#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Player.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Entities/Item.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/OneUpMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/Coin.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/Pipe.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <algorithm>

bool LevelLoader::loadLevel(const std::string& jsonPath, TileMap& tileMap, LevelData& levelData) {
    std::string filename = std::filesystem::path(jsonPath).filename().string();
    std::vector<std::string> fallbacks = {
        jsonPath,
        "SuperMarioGame/" + jsonPath,
        "../" + jsonPath,
        "build/" + jsonPath,
        "../build/" + jsonPath,
        "assets/levels/" + filename,
        "../assets/levels/" + filename,
        "SuperMarioGame/assets/levels/" + filename,
        "build/assets/levels/" + filename,
        "../build/assets/levels/" + filename,
        "SuperMarioGame/build/assets/levels/" + filename
    };

    std::ifstream file;
    for (const auto& altPath : fallbacks) {
        file.clear();
        file.open(altPath);
        if (file.is_open()) break;
    }

    if (!file.is_open()) {
        std::cerr << "[LevelLoader] Failed to open level JSON file: " << jsonPath << std::endl;
        return false;
    }


    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::cerr << "[LevelLoader] JSON parsing error in " << jsonPath << ": " << e.what() << std::endl;
        return false;
    }

    // 1. Parse Metadata
    levelData.name = j.value("name", "Unnamed Level");
    levelData.theme = j.value("theme", "overworld");
    int width = j.value("width", 40);
    int height = j.value("height", 22);
    
    // Initialize TileMap grid
    tileMap.initialize(width, height);
    levelData.width = width;
    levelData.height = height;

    // 2. Parse Spawn Point
    if (j.contains("spawnPoint") && j["spawnPoint"].is_object()) {
        float spawnX = j["spawnPoint"].value("x", 2.0f);
        float spawnY = j["spawnPoint"].value("y", 20.0f);
        levelData.spawnPoint = sf::Vector2f(spawnX * Constants::TILE_SIZE, spawnY * Constants::TILE_SIZE);
    } else {
        levelData.spawnPoint = sf::Vector2f(2.0f * Constants::TILE_SIZE, 20.0f * Constants::TILE_SIZE);
    }

    // 3. Parse Tiles list
    if (j.contains("tiles") && j["tiles"].is_array()) {
        for (const auto& tileJson : j["tiles"]) {
            std::string typeStr = tileJson.value("type", "empty");
            int tx = tileJson.value("x", 0);
            int ty = tileJson.value("y", 0);
            int tw = tileJson.value("w", 1); // horizontal span width
            
            TileType tileType = TileType::Empty;
            if (typeStr == "ground") tileType = TileType::Ground;
            else if (typeStr == "brick") tileType = TileType::Brick;
            else if (typeStr == "question_block" || typeStr == "question") tileType = TileType::Question;
            else if (typeStr == "pipe") tileType = TileType::Pipe;
            else if (typeStr == "ice") tileType = TileType::Ice;
            else if (typeStr == "conveyor") tileType = TileType::Conveyor;
            else if (typeStr == "water") tileType = TileType::Water;
            else if (typeStr == "coin") tileType = TileType::Coin;

            if (tileType != TileType::Empty) {
                for (int dx = 0; dx < tw; ++dx) {
                    tileMap.setTile(tx + dx, ty, tileType);
                }
            }
        }
    }

    // 4. Parse Entities list
    if (j.contains("entities") && j["entities"].is_array()) {
        for (const auto& entityJson : j["entities"]) {
            std::string typeStr = entityJson.value("type", "");
            float tx = entityJson.value("x", 0.0f);
            float ty = entityJson.value("y", 0.0f);
            
            sf::Vector2f position(tx * Constants::TILE_SIZE, ty * Constants::TILE_SIZE);
            std::unique_ptr<Entity> entity = nullptr;

            if (typeStr == "pipe") {
                int pipeId = entityJson.value("pipeId", 0);
                bool isEntrance = entityJson.value("isEntrance", false);
                std::string targetLevel = entityJson.value("targetLevel", "");
                float exitX = entityJson.value("exitX", tx + 2.0f);
                float exitY = entityJson.value("exitY", ty);
                entity = std::make_unique<Pipe>(position, pipeId, sf::Vector2f(exitX * Constants::TILE_SIZE, exitY * Constants::TILE_SIZE), targetLevel, isEntrance);
            } else {
                EntityType eType = SerializationUtils::parseEntityTypeName(typeStr);
                entity = EntityFactory::create(eType, position);
            }

            if (entity) {
                levelData.entities.push_back(std::move(entity));
            }
        }
    }

    std::cout << "[LevelLoader] Successfully loaded '" << levelData.name << "' (" 
              << width << "x" << height << ") from: " << jsonPath << std::endl;
    return true;
}

bool LevelLoader::saveLevel(const std::string& jsonPath, const TileMap& tileMap, 
                             const std::vector<std::unique_ptr<Entity>>& entities, const std::string& levelName) {
    try {
        std::filesystem::path fsPath(jsonPath);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["name"] = levelName;
        j["theme"] = "overworld";
        j["width"] = tileMap.getWidth();
        j["height"] = tileMap.getHeight();
        j["tileSize"] = Constants::TILE_SIZE;

        // Default spawn and flagpole locations clamped to map size
        int defaultSpawnX = std::clamp(2, 0, tileMap.getWidth() - 1);
        int defaultSpawnY = std::clamp(18, 0, tileMap.getHeight() - 1);
        int defaultFlagX = std::clamp(tileMap.getWidth() - 2, 0, tileMap.getWidth() - 1);
        int defaultFlagY = std::clamp(18, 0, tileMap.getHeight() - 1);

        j["spawnPoint"] = { {"x", defaultSpawnX}, {"y", defaultSpawnY} };
        j["flagpole"] = { {"x", defaultFlagX}, {"y", defaultFlagY} };

        // 1. Serialize Tiles with run-length horizontal optimization
        for (int y = 0; y < tileMap.getHeight(); ++y) {
            for (int x = 0; x < tileMap.getWidth(); ) {
                TileType type = tileMap.getTileType(x, y);
                if (type == TileType::Empty) {
                    x++;
                    continue;
                }
                int startX = x;
                while (x < tileMap.getWidth() && tileMap.getTileType(x, y) == type) {
                    x++;
                }
                int w = x - startX;
                nlohmann::json tileObj;
                tileObj["type"] = SerializationUtils::getTileTypeName(type);
                tileObj["x"] = startX;
                tileObj["y"] = y;
                if (w > 1) {
                    tileObj["w"] = w;
                }
                j["tiles"].push_back(tileObj);
            }
        }

        // 2. Serialize Entities list
        j["entities"] = nlohmann::json::array();
        for (const auto& entity : entities) {
            if (!entity) continue;
            std::string typeStr = SerializationUtils::getEntityTypeName(*entity);
            if (typeStr == "unknown" || typeStr == "mario" || typeStr == "luigi" || typeStr == "toad" || typeStr == "peach") {
                if (typeStr == "mario" || typeStr == "luigi" || typeStr == "toad" || typeStr == "peach") {
                    j["spawnPoint"]["x"] = static_cast<int>(entity->getPosition().x / Constants::TILE_SIZE);
                    j["spawnPoint"]["y"] = static_cast<int>(entity->getPosition().y / Constants::TILE_SIZE);
                }
                continue;
            }
            nlohmann::json entObj;
            entObj["type"] = typeStr;
            entObj["x"] = static_cast<int>(entity->getPosition().x / Constants::TILE_SIZE);
            entObj["y"] = static_cast<int>(entity->getPosition().y / Constants::TILE_SIZE);

            if (auto pipe = dynamic_cast<const Pipe*>(entity.get())) {
                entObj["pipeId"] = pipe->getPipeId();
                entObj["isEntrance"] = pipe->isEntrance();
                entObj["targetLevel"] = pipe->getTargetLevel();
                entObj["exitX"] = static_cast<int>(pipe->getExitPosition().x / Constants::TILE_SIZE);
                entObj["exitY"] = static_cast<int>(pipe->getExitPosition().y / Constants::TILE_SIZE);
            }

            j["entities"].push_back(entObj);
        }


        std::ofstream file(jsonPath);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "LevelLoader::saveLevel failed: " << e.what() << std::endl;
        return false;
    }
}
