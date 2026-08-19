#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

class SpriteSheet {
public:
    // Directly load into the ResourceManager
    SpriteSheet(const std::string& sheetFolderPath);
    // Load from ResourceManager
    SpriteSheet(const std::string& textureId, const std::string& dataFilepath);

    // Loads assets/spriteSheet/<folderName>, resolving the path for whatever
    // working directory the binary was launched from, and returns null when the
    // atlas is missing rather than throwing.
    //
    // This exists because the same "try three candidate paths" lambda had been
    // copied into PlayingState, MenuState, CharacterSelectState and
    // WorldMapState — audit A-13 counted four files guessing asset paths, and
    // three of those four were added while closing earlier tiers. One helper,
    // built on ResourceManager::resolvePath, so the next screen that needs an
    // atlas cannot add a fifth copy.
    static std::unique_ptr<SpriteSheet> loadAtlas(const std::string& folderName);

    sf::Sprite getSprite(sf::IntRect frameRect) const;
    sf::Sprite getSprite(int x,int y,int w,int h) const;
    sf::Sprite getSprite(const std::string& spriteName) const;
    std::vector<std::string> getFrameNames() const;
    bool hasFrame(const std::string& frameName) const;

private:
    const sf::Texture* m_spriteSheet = nullptr;
    std::unordered_map<std::string, sf::IntRect> m_frames;

    bool loadFrameData(const std::string& dataFilepath);
};

