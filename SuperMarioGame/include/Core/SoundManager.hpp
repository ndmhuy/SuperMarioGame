#pragma once

#include <string>
#include <vector>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>


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

    // Play a sound effect from the pool
    void playSound(const std::string& id);

    // Music methods
    void playMusic(const std::string& path);
    void playLevelBGM(int levelIndex);
    void playStarMusic();
    void restoreLevelBGM();
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

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
    
    sf::SoundBuffer m_fallbackBuffer;
};