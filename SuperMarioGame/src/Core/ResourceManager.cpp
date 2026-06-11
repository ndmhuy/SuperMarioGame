#include "Core/ResourceManager.hpp"

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

bool ResourceManager::loadTexture(const std::string& id, const std::string& path) {
    // TODO: Implement by hand
    return false;
}

sf::Texture& ResourceManager::getTexture(const std::string& id) {
    // TODO: Implement by hand
    static sf::Texture dummy;
    return dummy;
}

bool ResourceManager::loadFont(const std::string& id, const std::string& path) {
    // TODO: Implement by hand
    return false;
}

sf::Font& ResourceManager::getFont(const std::string& id) {
    // TODO: Implement by hand
    static sf::Font dummy;
    return dummy;
}

bool ResourceManager::loadSoundBuffer(const std::string& id, const std::string& path) {
    // TODO: Implement by hand
    return false;
}

sf::SoundBuffer& ResourceManager::getSoundBuffer(const std::string& id) {
    // TODO: Implement by hand
    static sf::SoundBuffer dummy;
    return dummy;
}

void ResourceManager::clear() {
    // TODO: Implement by hand
}
