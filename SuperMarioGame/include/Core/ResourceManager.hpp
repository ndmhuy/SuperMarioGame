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

    // Helper to resolve relative asset paths across working directories
    static std::string resolvePath(const std::string& relativePath);

    // Texture Methods
    bool loadTexture(const std::string& id, const std::string& path);
    bool hasTexture(const std::string& id) const;
    sf::Texture& getTexture(const std::string& id);

    // The texture to hand to an sf::Sprite when there is genuinely nothing to
    // draw.
    //
    // sf::Sprite cannot be constructed without a texture, so every "no sprite
    // yet" path needs *some* texture to point at. Callers used to ask
    // getTexture("") for it, which printed a spurious "Texture not found:" with
    // an empty id once per run — a warning about nothing, in a log where the
    // real missing-asset warnings look identical.
    //
    // Deliberately left zero-sized: Entity::drawSprite() skips a zero-size
    // sprite, so this draws nothing, which is the behaviour these paths already
    // had. hasArtwork() is the honest way to ask whether an entity will draw.
    // Lives in the texture map like every other placeholder, so clear() releases
    // it while the graphics context is still alive.
    sf::Texture& placeholderTexture();

    // Font Methods
    bool loadFont(const std::string& id, const std::string& path);
    bool hasFont(const std::string& id) const;
    sf::Font& getFont(const std::string& id);

    // Sound Buffer Methods
    bool loadSoundBuffer(const std::string& id, const std::string& path);
    bool hasSoundBuffer(const std::string& id) const;
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
