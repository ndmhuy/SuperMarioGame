#include "Core/ResourceManager.hpp"
#include <iostream>

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

bool ResourceManager::loadTexture(const std::string& id, const std::string& path) {
    // SFML 3.0 API texture loading stub
    sf::Texture texture;
    if (texture.loadFromFile(path)) {
        m_textures[id] = std::move(texture);
        return true;
    }
    std::cerr << "Failed to load texture: " << path << std::endl;
    return false;
}

sf::Texture& ResourceManager::getTexture(const std::string& id) {
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        return it->second;
    }
    // Fallback stub: return a static empty texture to avoid crash
    static sf::Texture dummy;
    return dummy;
}

bool ResourceManager::loadFont(const std::string& id, const std::string& path) {
    sf::Font font;
    if (font.openFromFile(path)) {
        m_fonts[id] = std::move(font);
        return true;
    }
    std::cerr << "Failed to load font: " << path << std::endl;
    return false;
}

sf::Font& ResourceManager::getFont(const std::string& id) {
    auto it = m_fonts.find(id);
    if (it != m_fonts.end()) {
        return it->second;
    }
    static sf::Font dummy;
    return dummy;
}

bool ResourceManager::loadSoundBuffer(const std::string& id, const std::string& path) {
    sf::SoundBuffer soundBuffer;
    if (soundBuffer.loadFromFile(path)) {
        m_soundBuffers[id] = std::move(soundBuffer);
        return true;
    }
    std::cerr << "Failed to load sound buffer: " << path << std::endl;
    return false;
}

sf::SoundBuffer& ResourceManager::getSoundBuffer(const std::string& id) {
    auto it = m_soundBuffers.find(id);
    if (it != m_soundBuffers.end()) {
        return it->second;
    }
    static sf::SoundBuffer dummy;
    return dummy;
}

void ResourceManager::clear() {
    m_textures.clear();
    m_fonts.clear();
    m_soundBuffers.clear();
}
