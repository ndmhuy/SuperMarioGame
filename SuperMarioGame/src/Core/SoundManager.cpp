#include "Core/SoundManager.hpp"
#include "Core/ResourceManager.hpp"

#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <SFML/Audio/Sound.hpp>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    static sf::SoundBuffer dummyBuffer;
    m_soundPool.assign(SFX_POOL_SIZE, sf::Sound(dummyBuffer));
    
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
    m_fallbackBuffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate, channelMap);
}

void SoundManager::playSound(const std::string& id) {
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

void SoundManager::shutdown() {
    m_music.stop();
    for (auto& sound : m_soundPool) {
        sound.stop();
    }
    m_soundPool.clear();
}