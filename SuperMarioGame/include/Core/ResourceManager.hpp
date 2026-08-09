#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

class ResourceManager {
public:
    // Delete copy/move semantics for Singleton
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // Singleton Instance
    static ResourceManager& getInstance();

    // Texture Methods
    bool loadTexture(const std::string& id, const std::string& path);
    sf::Texture& getTexture(const std::string& id);

    // Font Methods
    bool loadFont(const std::string& id, const std::string& path);
    sf::Font& getFont(const std::string& id);

    // Sound Buffer Methods
    bool loadSoundBuffer(const std::string& id, const std::string& path);
    sf::SoundBuffer& getSoundBuffer(const std::string& id);

    // Cleanup Methods
    void clear();

private:
    // Private constructor/destructor for Singleton
    ResourceManager() = default;
    ~ResourceManager() = default;

    // Internal maps
    std::unordered_map<std::string, sf::Texture> m_textures;
    std::unordered_map<std::string, sf::Font> m_fonts;
    std::unordered_map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::unordered_set<std::string> m_reportedMissing;
};
