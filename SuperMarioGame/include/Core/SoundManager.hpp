#pragma once

#include <string>
#include <vector>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include "Core/EventBus.hpp"


class SoundManager {
public:
    // Delete copy/move semantics for Singleton
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    SoundManager& operator=(SoundManager&&) = delete;

    // Singleton Instance
    static SoundManager& getInstance();

    // Load all SFX files into ResourceManager
    void loadAllSounds();

    // EventBus listener wiring
    void setupEventSubscriptions();

    // Play a sound effect from the pool. `pitch` defaults to unmodified for
    // every existing caller; the combo escalation below is the one caller that
    // passes something else.
    void playSound(const std::string& id, float pitch = 1.0f);

    // Music methods
    // `loop` false for one-shot jingles — level complete, castle complete, game
    // over. It used to be hardcoded true for everything, so the 3-second
    // level-clear cue restarted for as long as the celebration lasted and read as
    // the jingle playing over and over.
    void playMusic(const std::string& path, bool loop = true);

    // Whether the current track repeats. Exists so the regression suite can tell
    // a jingle from level music without opening an audio device to listen.
    bool isMusicLooping() const { return m_music.isLooping(); }

    // Resolved path of the track last handed to playMusic(). Same rationale as
    // isMusicLooping(): lets the regression suite assert which BGM a level
    // index selects (audit D1) without an audio device.
    const std::string& getCurrentMusicPath() const { return m_musicPath; }
    void playLevelBGM(int levelIndex);
    void playStarMusic();
    void restoreLevelBGM();
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

    // Per-frame poll (audit D26): a one-shot celebratory jingle (loop=false)
    // that is still playing when a second one is requested — e.g. a boss's
    // castle_complete fanfare followed moments later by the flagpole's
    // level_complete jingle in the same boss-level clear — used to be clobbered
    // mid-playback because playMusic() switched tracks unconditionally. The
    // second request is now held in m_pendingMusic* and started here once the
    // first one-shot track's sf::Music::Status stops being Playing. Wired into
    // Game::run()'s fixed-timestep loop next to AchievementManager::update().
    void update(float dt);

    // Lifecycle
    void shutdown();

    // Set volume methods
    void setSFXVolume(float volume);
    void setMusicVolume(float volume);

private:
    // Private constructor/destructor for Singleton
    SoundManager();
    ~SoundManager() = default;

    const int SFX_POOL_SIZE = 64;

    float m_SFXVolume = 100;
    float m_musicVolume = 100;

    std::vector<sf::Sound> m_soundPool;
    sf::Music m_music;
    std::string m_musicPath;
    std::string m_savedLevelMusicPath;
    bool m_soundsLoaded = false;
    bool m_eventsSubscribed = false;

    // ComboHit escalation (SPEC 11.1's combo_1..combo_4 SFX). Kept as a
    // ScopedSubscription (audit X-7 / R4) rather than the raw ids the dozen
    // subscriptions in setupEventSubscriptions() still use, since this one is
    // set up separately from that block.
    EventBus::ScopedSubscription m_comboHitSub;

    // A playMusic() request deferred because it arrived while the currently
    // playing track was a still-running one-shot jingle (see update()). Only
    // ever holds the single most recent deferred request — a later playMusic()
    // call while one is already pending simply overwrites it, same as it would
    // have overwritten m_music directly before this fix existed.
    bool m_hasPendingMusic = false;
    std::string m_pendingMusicPath;   // already fully resolved, not an alias/id
    bool m_pendingMusicLoop = false;

    // Shared by playMusic() (fresh request) and update() (deferred request) —
    // the actual sf::Music::openFromFile()/play() call, taking an
    // already-resolved path so update() never re-runs alias/path resolution.
    void startMusicNow(const std::string& resolvedPath, bool loop);

    // Probed once at construction via sf::PlaybackDevice::getDefaultDevice().
    // When false, playSound()/playMusic() are silent no-ops instead of driving
    // sf::Sound/sf::Music against hardware that was never there — a machine
    // with no audio device used to abort at startup instead of degrading.
    bool m_audioAvailable = true;

    sf::SoundBuffer m_fallbackBuffer;
};