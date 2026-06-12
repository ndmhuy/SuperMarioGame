#pragma once

#include <string>
#include <vector>
#include <SFML/Audio/Music.hpp>

class SoundManager {
public:
    // Delete copy/move semantics for Singleton
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    SoundManager& operator=(SoundManager&&) = delete;

    // Singleton Instance
    static SoundManager& getInstance();

    // Play a sound effect from the pool
    void playSound(const std::string& id);

    // Music methods
    void playMusic(const std::string& path);
    void stopMusic();
    void pauseMusic();

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
};