#include "Graphics/SpriteSheet.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/ResourceManager.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace nlohmann;

SpriteSheet::SpriteSheet(const std::string& sheetFolderPath) {
    std::string resolvedFolder = ResourceManager::resolvePath(sheetFolderPath);
    std::filesystem::path path(resolvedFolder);

    std::string filename = path.filename().string();

    std::string textureFilepath = (path / (filename + ".png")).string(),
                dataFilepath = (path / (filename + ".json")).string();

    ResourceManager& rm = ResourceManager::getInstance();
    if (!rm.loadTexture(textureFilepath, textureFilepath)) {
        std::cerr << "[SpriteSheet] Failed to load texture: " << textureFilepath << std::endl;
        // A named blank, not getTexture(""): the atlas failing to load is
        // already reported on the line above, and asking for an empty id only
        // added a second, less informative warning about it.
        m_spriteSheet = &rm.placeholderTexture();
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
        std::string actualPath = ResourceManager::resolvePath(dataFilepath);
        std::ifstream file(actualPath);
        if (!file.is_open()) {
            std::cerr << "[SpriteSheet] Could not open json file: " << actualPath << std::endl;
            return false;
        }
        json data = json::parse(file);

        // Standard TexturePacker "frames" format (player, enemy, item, scenery atlases)
        if (data.contains("frames")) {
            for (json::iterator it = data["frames"].begin(); it != data["frames"].end(); it++) {
                sf::IntRect spriteRect(
                    sf::Vector2i(it.value()["frame"]["x"].get<int>(), it.value()["frame"]["y"].get<int>()),
                    sf::Vector2i(it.value()["frame"]["w"].get<int>(), it.value()["frame"]["h"].get<int>())
                );
                m_frames[it.key()] = spriteRect;
            }
            return true;
        }

        // Custom "tiles" format (tileset_blocks.json): { "tiles": { "name": { x, y, w, h } } }
        if (data.contains("tiles")) {
            for (json::iterator it = data["tiles"].begin(); it != data["tiles"].end(); it++) {
                sf::IntRect spriteRect(
                    sf::Vector2i(it.value()["x"].get<int>(), it.value()["y"].get<int>()),
                    sf::Vector2i(it.value()["w"].get<int>(), it.value()["h"].get<int>())
                );
                m_frames[it.key()] = spriteRect;
            }
            return true;
        }

        std::cerr << "[SpriteSheet] Unknown JSON format in: " << dataFilepath << std::endl;
        return false;
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

bool SpriteSheet::hasFrame(const std::string& frameName) const {
    return m_frames.find(frameName) != m_frames.end();
}

std::unique_ptr<SpriteSheet> SpriteSheet::loadAtlas(const std::string& folderName) {
    const std::string resolved =
        ResourceManager::resolvePath("assets/spriteSheet/" + folderName);
    if (!std::filesystem::exists(resolved)) {
        std::cerr << "[SpriteSheet] Atlas not found: " << folderName << std::endl;
        return nullptr;
    }
    try {
        return std::make_unique<SpriteSheet>(resolved);
    } catch (const std::exception& e) {
        std::cerr << "[SpriteSheet] Could not load " << folderName << ": " << e.what() << std::endl;
        return nullptr;
    }
}
