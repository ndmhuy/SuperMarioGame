#include "Graphics/SpriteSheet.hpp"

#include "nlohmann/json.hpp"
#include "Core/ResourceManager.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

using namespace nlohmann;

SpriteSheet::SpriteSheet(const std::string& sheetFolderPath) {
    std::filesystem::path path(sheetFolderPath);

    std::string filename = path.filename().string();

    std::string textureFilepath = (path / (filename + ".png")).string(),
                dataFilepath = (path / (filename + ".json")).string();

    ResourceManager& rm = ResourceManager::getInstance();
    if (!rm.loadTexture(textureFilepath, textureFilepath)) {
        std::cerr << "[SpriteSheet] Failed to load texture" << std::endl;
        m_spriteSheet = &rm.getTexture("");
        return;
    }
    m_spriteSheet = &rm.getTexture(textureFilepath);

    loadFrameData(dataFilepath);
}

SpriteSheet::SpriteSheet(const std::string& textureId, const std::string& dataFilepath) {
    m_spriteSheet = &(ResourceManager::getInstance().getTexture(textureId));

    loadFrameData(dataFilepath);
}

bool SpriteSheet::loadFrameData(const std::string& dataFilepath) {
    try {
        json data = json::parse(std::ifstream(dataFilepath));
        for (json::iterator it = data["frames"].begin(); it != data["frames"].end(); it++) {
            sf::IntRect spriteRect(
                sf::Vector2i(it.value()["frame"]["x"].get<int>(), it.value()["frame"]["y"].get<int>()),
                sf::Vector2i(it.value()["frame"]["w"].get<int>(), it.value()["frame"]["h"].get<int>())
            );

            m_frames[it.key()] = spriteRect;
        }
        return true;
    }
    catch (std::exception &e) {
        std::cerr << "[SpriteSheet] Frame data load failed: " << e.what() << std::endl;
        return false;
    }
}

sf::Sprite SpriteSheet::getSprite(sf::IntRect frameRect) const {
    return sf::Sprite(*m_spriteSheet, frameRect);
}

sf::Sprite SpriteSheet::getSprite(int x,int y,int w,int h) const {
    return sf::Sprite(
        *m_spriteSheet,
        sf::IntRect(sf::Vector2i(x,y),sf::Vector2i(w,h))
    );
}

sf::Sprite SpriteSheet::getSprite(const std::string& spriteName) const {
    auto it = m_frames.find(spriteName);
    if (it == m_frames.end()) 
        return sf::Sprite(*m_spriteSheet, sf::IntRect());

    return sf::Sprite(*m_spriteSheet, it->second);
}

std::vector<std::string> SpriteSheet::getFrameNames() const {
    std::vector<std::string> names;
    names.reserve(m_frames.size());
    for (const auto& pair : m_frames) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}
