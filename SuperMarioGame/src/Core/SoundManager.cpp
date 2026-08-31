#include "Core/SoundManager.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/EventBus.hpp"

#include <algorithm>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    // Probed once, here, rather than scattered null-checks at every call site
    // (audit D6): a machine with no audio device used to abort at startup
    // instead of degrading to silent play. Buffer decoding below needs no
    // device — only the pool of live sf::Sound objects and m_music's actual
    // playback do, so those are what the guard skips.
    m_audioAvailable = sf::PlaybackDevice::getDefaultDevice().has_value();
    if (!m_audioAvailable) {
        std::cerr << "[SoundManager] No audio playback device detected — "
                     "running silent." << std::endl;
    }

    static sf::SoundBuffer dummyBuffer;
    if (m_audioAvailable) {
        m_soundPool.assign(SFX_POOL_SIZE, sf::Sound(dummyBuffer));
    }

    // Generate fallback synth beep
    std::vector<std::int16_t> samples;
    unsigned int sampleRate = 44100;
    double frequency = 440.0;
    double duration = 0.5;
    size_t sampleCount = static_cast<size_t>(sampleRate * duration);
    samples.reserve(sampleCount);

    for (size_t i = 0; i < sampleCount; ++i) {
        double time = static_cast<double>(i) / sampleRate;
        double value = std::sin(time * frequency * 2.0 * 3.14159265);
        samples.push_back(static_cast<std::int16_t>(value * 32767.0));
    }
    std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};
    // The result is [[nodiscard]]. A failed fallback tone is not fatal — it is
    // the substitute for a missing sound file — but it should not be silent
    // about failing.
    if (!m_fallbackBuffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate, channelMap)) {
        std::cerr << "[SoundManager] Could not build the fallback tone buffer." << std::endl;
    }
}

void SoundManager::loadAllSounds() {
    if (m_soundsLoaded) return;
    m_soundsLoaded = true;

    ResourceManager& rm = ResourceManager::getInstance();
    std::vector<std::string> sfxFiles = {
        "boing", "bowserfall", "break_brick_block", "bubble", "bump",
        "coin", "damage", "enter_level", "fireball", "flagpole",
        "footstep_floor", "footstep_grass", "footstep_metalcap", "game_over",
        "jump_small", "jump_super", "kick", "lost_life",
        "mushroom_fireflower_appears", "one_up", "pause", "pipe",
        "power_up", "stage_clear", "stomp", "thwomp", "time_warning",
        "vine_grow", "world_clear"
    };

    for (const auto& name : sfxFiles) {
        std::string pathWav = "assets/sfx/" + name + ".wav";
        std::string pathWAV = "assets/sfx/" + name + ".WAV";
        std::string resolved = ResourceManager::resolvePath(pathWav);
        if (!std::filesystem::exists(resolved)) {
            resolved = ResourceManager::resolvePath(pathWAV);
        }
        rm.loadSoundBuffer(name, resolved);
    }

    // Alias registration for exact file names and legacy code identifiers
    if (rm.hasSoundBuffer("break_brick_block")) {
        rm.loadSoundBuffer("break_block", ResourceManager::resolvePath("assets/sfx/break_brick_block.wav"));
    }
    if (rm.hasSoundBuffer("mushroom_fireflower_appears")) {
        rm.loadSoundBuffer("powerup_appears", ResourceManager::resolvePath("assets/sfx/mushroom_fireflower_appears.wav"));
    }
    if (rm.hasSoundBuffer("jump_small")) {
        rm.loadSoundBuffer("jump", ResourceManager::resolvePath("assets/sfx/jump_small.wav"));
    }
    if (rm.hasSoundBuffer("power_up")) {
        rm.loadSoundBuffer("powerup", ResourceManager::resolvePath("assets/sfx/power_up.wav"));
    }

    setupEventSubscriptions();
}

void SoundManager::setupEventSubscriptions() {
    if (m_eventsSubscribed) return;
    m_eventsSubscribed = true;

    EventBus& bus = EventBus::getInstance();

    bus.subscribe(EventType::CoinCollected, [this](const GameEvent&) { playSound("coin"); });
    // Task 11.4, audio cues: a star coin is one of three in a level and used to
    // sound exactly like an ordinary coin, so a player relying on sound could
    // not tell a major pickup from a trivial one. A checkpoint was silent
    // altogether — the only feedback was visual.
    bus.subscribe(EventType::StarCoinCollected, [this](const GameEvent&) { playSound("one_up"); });
    bus.subscribe(EventType::CheckpointActivated, [this](const GameEvent&) { playSound("vine_grow"); });
    // "boing" is the trampoline's cue, and the P-Switch was borrowing it — so
    // pressing the switch sounded exactly like bouncing off a spring and read as
    // a bug rather than an effect. "kick" is the short percussive thunk of the
    // switch going down, and is used by nothing else.
    bus.subscribe(EventType::PSwitchActivated, [this](const GameEvent&) { playSound("kick"); });
    bus.subscribe(EventType::EnemyDefeated, [this](const GameEvent&) { playSound("stomp"); });
    bus.subscribe(EventType::PlayerDied, [this](const GameEvent&) { stopMusic(); playSound("lost_life"); });
    bus.subscribe(EventType::PowerUpCollected, [this](const GameEvent&) { playSound("power_up"); });
    // One owner for the level-clear cue, and one play of it.
    //
    // This handler used to fire BOTH playMusic("level_complete") AND
    // playSound("stage_clear") — the same cue twice at once, since
    // assets/bgm/level_complete.mp3 and assets/sfx/stage_clear.wav are the same
    // fanfare. playMusic then looped it for the whole celebration, and
    // VictoryState::enter played it a third time three seconds later. Three
    // owners, no latch, hence "end level plays end level music multiple times".
    //
    // Now: the jingle is music, it plays once, and VictoryState leaves it alone.
    // The latch is the one in PlayingState's own LevelComplete handler, which
    // already ignores a repeat publish from a second flagpole.
    bus.subscribe(EventType::LevelComplete, [this](const GameEvent&) {
        playMusic("level_complete", /*loop=*/false);
    });
    bus.subscribe(EventType::BossDefeated, [this](const GameEvent&) {
        playMusic("castle_complete", /*loop=*/false);
    });
    bus.subscribe(EventType::GameOver, [this](const GameEvent&) {
        playMusic("game_over", /*loop=*/false);
    });
    bus.subscribe(EventType::PlayerDamaged, [this](const GameEvent&) { playSound("damage"); });
    bus.subscribe(EventType::BlockBroken, [this](const GameEvent&) { playSound("break_brick_block"); });
    bus.subscribe(EventType::PlayerShotFireball, [this](const GameEvent&) { playSound("fireball"); });
    bus.subscribe(EventType::POWBlockHit, [this](const GameEvent&) { playSound("thwomp"); });
    bus.subscribe(EventType::ThwompSlam, [this](const GameEvent&) { playSound("thwomp"); });
    bus.subscribe(EventType::GroundPoundSlam, [this](const GameEvent&) { playSound("stomp"); });
    bus.subscribe(EventType::TimeWarning, [this](const GameEvent&) { playSound("time_warning"); });
    bus.subscribe(EventType::PauseToggled, [this](const GameEvent&) { playSound("pause"); });
    // An achievement unlocking was completely silent; the only feedback was an
    // ImGui window that normal play never shows.
    bus.subscribe(EventType::AchievementUnlocked, [this](const GameEvent&) { playSound("one_up"); });

    // SPEC 11.1 names dedicated combo_1..combo_4 SFX files, but only the 29
    // names in loadAllSounds() above actually ship in assets/sfx — there is no
    // combo_*.wav to load. A rising pitch on an existing sound stands in
    // instead (R7 audit): each chained kill (Player::incrementCombo(),
    // Player.cpp) escalates the pitch a step further, capped so a long combo
    // does not end up an inaudible chipmunk squeak.
    m_comboHitSub = EventBus::ScopedSubscription(EventType::ComboHit, [this](const GameEvent& ev) {
        int combo = 1;
        if (const int* asInt = std::any_cast<int>(&ev.data)) combo = *asInt;
        const float pitch = 1.0f + 0.12f * static_cast<float>(std::min(combo - 1, 6));
        playSound("coin", pitch);
    });
}

void SoundManager::playSound(const std::string& id, float pitch) {
    if (!m_audioAvailable) return;

    if (!m_soundsLoaded) {
        loadAllSounds();
    }

    sf::SoundBuffer& sfxBuffer = ResourceManager::getInstance().getSoundBuffer(id);

    const sf::SoundBuffer* bufferToPlay = &sfxBuffer;
    if (sfxBuffer.getSampleCount() == 0) {
        bufferToPlay = &m_fallbackBuffer;
    }

    // Ignore the sound if the channels are full.
    for (sf::Sound& poolSFX : m_soundPool) {
        if (poolSFX.getStatus() == sf::SoundSource::Status::Stopped) {
            poolSFX.setBuffer(*bufferToPlay);
            poolSFX.setVolume(m_SFXVolume);
            // Always assigned, even at the default 1.0 — a pooled sf::Sound is
            // reused across calls, and a stale pitch from a previous combo hit
            // must not bleed into the next, unrelated sound this slot plays.
            poolSFX.setPitch(pitch);
            poolSFX.play();
            break;
        }
    }
}

static std::string resolveBGMIdentifier(const std::string& input) {
    static const std::unordered_map<std::string, std::string> bgmAliasMap = {
        {"main_menu", "assets/bgm/main_menu.mp3"},
        {"title_screen", "assets/bgm/main_menu.mp3"},
        {"overworld", "assets/bgm/overworld.mp3"},
        {"underworld", "assets/bgm/underworld.mp3"},
        {"underwater", "assets/bgm/underwater.mp3"},
        {"sub_space-bonus_room", "assets/bgm/sub_space-bonus_room.mp3"},
        {"athletic", "assets/bgm/sub_space-bonus_room.mp3"},
        {"bonus", "assets/bgm/sub_space-bonus_room.mp3"},
        {"castle", "assets/bgm/castle.mp3"},
        {"bowser_castle", "assets/bgm/castle.mp3"},
        {"bowser_boss_battle", "assets/bgm/bowser_boss_battle.mp3"},
        {"starman", "assets/bgm/starman.mp3"},
        {"invincible", "assets/bgm/starman.mp3"},
        {"world_map", "assets/bgm/world_map.mp3"},
        {"level_complete", "assets/bgm/level_complete.mp3"},
        {"castle_complete", "assets/bgm/castle_complete.mp3"},
        {"game_over", "assets/bgm/game_over.mp3"}
    };

    auto it = bgmAliasMap.find(input);
    if (it != bgmAliasMap.end()) {
        return it->second;
    }
    if (input.find("assets/bgm/") == std::string::npos && input.find(".mp3") == std::string::npos) {
        return "assets/bgm/" + input + ".mp3";
    }
    return input;
}

void SoundManager::startMusicNow(const std::string& resolvedPath, bool loop) {
    m_hasPendingMusic = false;
    m_musicPath = resolvedPath;
    if (!m_music.openFromFile(resolvedPath)) {
        std::cerr << "[SoundManager] Could not open music file: " << resolvedPath << std::endl;
        return;
    }
    m_music.setLooping(loop);
    m_music.setVolume(m_musicVolume);
    m_music.play();
}

void SoundManager::playMusic(const std::string& path, bool loop) {
    if (!m_audioAvailable) return;

    std::string bgmPath = resolveBGMIdentifier(path);
    std::string resolved = ResourceManager::resolvePath(bgmPath);

    if (resolved == m_musicPath) {
        // Re-requesting the already-current track cancels any deferred swap —
        // e.g. LevelComplete re-publishing (the flagpole can fire more than
        // once) should not resurrect a stale pending track.
        m_hasPendingMusic = false;
        if (m_music.getStatus() != sf::SoundSource::Status::Playing) {
            m_music.play();
        }
        return;
    }

    // Audit D26: a one-shot celebratory jingle (loop=false) that is still
    // playing must be allowed to finish rather than be clobbered mid-playback
    // by a SECOND one-shot jingle — the reproducible case is a boss level,
    // where Boss::update() fires BossDefeated -> playMusic("castle_complete",
    // false) the instant the defeat sequence starts, and shortly after
    // touching the now-clear flag fires LevelComplete -> playMusic
    // ("level_complete", false), which used to cut castle_complete off
    // mid-fanfare.
    //
    // The deferral requires the INCOMING request to also be a one-shot
    // (loop == false): ordinary BGM switching — a jingle handing off to real
    // level/level-select/starman music (loop == true) — must keep interrupting
    // immediately, exactly as before. testJinglesDoNotLoop
    // (tests/verify_regressions.cpp) depends on this: it plays a jingle then
    // immediately switches to looping "overworld" music and expects the switch
    // to take effect at once.
    if (!loop && !m_music.isLooping() &&
        m_music.getStatus() == sf::SoundSource::Status::Playing) {
        m_pendingMusicPath = resolved;
        m_pendingMusicLoop = loop;
        m_hasPendingMusic = true;
        return;
    }

    startMusicNow(resolved, loop);
}

void SoundManager::update(float /*dt*/) {
    if (!m_audioAvailable || !m_hasPendingMusic) return;
    // Still playing the current one-shot jingle — keep waiting.
    if (m_music.getStatus() == sf::SoundSource::Status::Playing) return;

    std::string resolved = m_pendingMusicPath;
    bool loop = m_pendingMusicLoop;
    startMusicNow(resolved, loop);
}

void SoundManager::playLevelBGM(int levelIndex) {
    // levelIndex is the LevelCatalog index (see LevelCatalog::levels()), not a
    // theme id: 0=1-1 Grassland, 1=1-2 Ice Cavern, 2=1-3 Bowser's Castle,
    // 3=Bonus Stage. A generated/endless level has no catalog index and falls
    // through to the overworld default, same as any unrecognised index.
    std::string bgmKey = "overworld";
    if (levelIndex == 1) bgmKey = "underworld";
    else if (levelIndex == 2) bgmKey = "castle";
    else if (levelIndex == 3) bgmKey = "sub_space-bonus_room";

    m_savedLevelMusicPath = resolveBGMIdentifier(bgmKey);
    playMusic(m_savedLevelMusicPath);
}

void SoundManager::playStarMusic() {
    std::string starKey = "starman";
    std::string resolvedStar = ResourceManager::resolvePath(resolveBGMIdentifier(starKey));
    if (m_musicPath != resolvedStar) {
        m_savedLevelMusicPath = m_musicPath;
    }
    playMusic(starKey);
}

void SoundManager::restoreLevelBGM() {
    if (!m_savedLevelMusicPath.empty()) {
        playMusic(m_savedLevelMusicPath);
    } else {
        playMusic("overworld");
    }
}

void SoundManager::stopMusic() {
    m_music.stop();
    // An explicit stop means "stop everything now" — do not let a deferred
    // swap (see update()) surprise-start a track right after it.
    m_hasPendingMusic = false;
}

void SoundManager::pauseMusic() {
    m_music.pause();
}

void SoundManager::resumeMusic() {
    if (m_music.getStatus() == sf::SoundSource::Status::Paused) {
        m_music.play();
    }
}

void SoundManager::setSFXVolume(float volume) {
    m_SFXVolume = volume;
    for (sf::Sound &poolSFX : m_soundPool) {
        poolSFX.setVolume(volume);
    }
}

void SoundManager::setMusicVolume(float volume) {
    m_musicVolume = volume;
    m_music.setVolume(volume);
}

void SoundManager::shutdown() {
    m_music.stop();
    m_hasPendingMusic = false;
    for (auto& sound : m_soundPool) {
        sound.stop();
    }
    m_soundPool.clear();
}