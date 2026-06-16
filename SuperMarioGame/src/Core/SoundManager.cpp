#include "Core/SoundManager.hpp"
#include "Core/ResourceManager.hpp"

#include <iostream>
#include <SFML/Audio/Sound.hpp>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    static sf::SoundBuffer dummyBuffer;
    m_soundPool.assign(SFX_POOL_SIZE, sf::Sound(dummyBuffer));
}

void SoundManager::playSound(const std::string& id) {
    sf::SoundBuffer& sfxBuffer = ResourceManager::getInstance().getSoundBuffer(id);

    // Ignore the sound if the channels are full.
    for (sf::Sound& poolSFX : m_soundPool) {
        if (poolSFX.getStatus() == sf::SoundSource::Status::Stopped) {
            poolSFX.setBuffer(sfxBuffer);
            poolSFX.setVolume(m_SFXVolume);
            poolSFX.play();
            break;
        }
    }
}

void SoundManager::playMusic(const std::string& path) {
    if (path != m_musicPath) {
        m_musicPath = path;
        if (!m_music.openFromFile(path)) {
            std::cerr << "No music path found!" << std::endl;
            return;
        }
        m_music.setVolume(m_musicVolume);
        m_music.play();
    } else {
        if (m_music.getStatus() != sf::SoundSource::Status::Playing) {
            m_music.play();
        }
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