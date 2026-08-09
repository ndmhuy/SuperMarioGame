#pragma once

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

    sf::Sprite getSprite(sf::IntRect frameRect) const;
    sf::Sprite getSprite(int x,int y,int w,int h) const;
    sf::Sprite getSprite(const std::string& spriteName) const;
    std::vector<std::string> getFrameNames() const;

private:
    const sf::Texture* m_spriteSheet = nullptr;
    std::unordered_map<std::string, sf::IntRect> m_frames;

    bool loadFrameData(const std::string& dataFilepath);
};

