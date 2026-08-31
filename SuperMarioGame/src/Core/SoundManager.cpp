#include "Core/SoundManager.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/EventBus.hpp"

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
}

void SoundManager::playSound(const std::string& id) {
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

void SoundManager::playMusic(const std::string& path, bool loop) {
    if (!m_audioAvailable) return;

    std::string bgmPath = resolveBGMIdentifier(path);
    std::string resolved = ResourceManager::resolvePath(bgmPath);
    if (resolved != m_musicPath) {
        m_musicPath = resolved;
        if (!m_music.openFromFile(resolved)) {
            std::cerr << "[SoundManager] Could not open music file: " << resolved << std::endl;
            return;
        }
        m_music.setLooping(loop);
        m_music.setVolume(m_musicVolume);
        m_music.play();
    } else {
        if (m_music.getStatus() != sf::SoundSource::Status::Playing) {
            m_music.play();
        }
    }
}

void SoundManager::playLevelBGM(int levelIndex) {
    std::string bgmKey = "overworld";
    if (levelIndex == 1) bgmKey = "underworld";
    else if (levelIndex == 2) bgmKey = "underwater";
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
    for (auto& sound : m_soundPool) {
        sound.stop();
    }
    m_soundPool.clear();
}