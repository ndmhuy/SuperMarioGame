#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
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
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

// Helper: map TileType to string
static std::string getTileTypeName(TileType type) {
    switch (type) {
        case TileType::Ground: return "ground";
        case TileType::Brick: return "brick";
        case TileType::Question: return "question_block";
        case TileType::Pipe: return "pipe";
        case TileType::Ice: return "ice";
        case TileType::Conveyor: return "conveyor";
        case TileType::Water: return "water";
        case TileType::Coin: return "coin_tile";
        default: return "empty";
    }
}

// Helper: map string to TileType
static TileType parseTileTypeName(const std::string& name) {
    if (name == "ground") return TileType::Ground;
    if (name == "brick") return TileType::Brick;
    if (name == "question_block") return TileType::Question;
    if (name == "pipe") return TileType::Pipe;
    if (name == "ice") return TileType::Ice;
    if (name == "conveyor") return TileType::Conveyor;
    if (name == "water") return TileType::Water;
    if (name == "coin_tile" || name == "coin") return TileType::Coin;
    return TileType::Empty;
}

// Helper: map Entity to string type
static std::string getEntityTypeName(const Entity& entity) {
    if (dynamic_cast<const Mario*>(&entity)) return "mario";
    if (dynamic_cast<const Luigi*>(&entity)) return "luigi";
    if (dynamic_cast<const Toad*>(&entity)) return "toad";
    if (dynamic_cast<const Peach*>(&entity)) return "peach";
    if (dynamic_cast<const Mushroom*>(&entity)) return "mushroom";
    if (dynamic_cast<const OneUpMushroom*>(&entity)) return "oneup_mushroom";
    if (dynamic_cast<const MiniMushroom*>(&entity)) return "mini_mushroom";
    if (dynamic_cast<const MegaMushroom*>(&entity)) return "mega_mushroom";
    if (dynamic_cast<const CapeFeather*>(&entity)) return "cape_feather";
    if (dynamic_cast<const FireFlower*>(&entity)) return "fire_flower";
    if (dynamic_cast<const Star*>(&entity)) return "star";
    if (dynamic_cast<const Coin*>(&entity)) return "coin";
    if (dynamic_cast<const StarCoin*>(&entity)) return "star_coin";
    if (dynamic_cast<const PSwitch*>(&entity)) return "pswitch";
    if (dynamic_cast<const POWBlock*>(&entity)) return "pow_block";
    if (dynamic_cast<const Trampoline*>(&entity)) return "trampoline";
    return "unknown";
}

bool LevelLoader::loadLevel(const std::string& jsonPath, TileMap& tileMap, LevelData& levelData) {
    try {
        if (!std::filesystem::exists(jsonPath)) return false;

        std::ifstream file(jsonPath);
        if (!file.is_open()) return false;

        nlohmann::json j;
        file >> j;

        levelData.name = j.value("name", "Custom Level");
        levelData.theme = j.value("theme", "overworld");
        levelData.width = j.value("width", 40);
        levelData.height = j.value("height", 22);

        // 1. Initialize TileMap
        tileMap.initialize(levelData.width, levelData.height);

        // 2. Parse Tiles
        if (j.contains("tiles") && j["tiles"].is_array()) {
            for (const auto& tileObj : j["tiles"]) {
                std::string typeStr = tileObj.value("type", "empty");
                TileType type = parseTileTypeName(typeStr);
                int tx = tileObj.value("x", 0);
                int ty = tileObj.value("y", 0);
                int tw = tileObj.value("w", 1);
                
                for (int dx = 0; dx < tw; ++dx) {
                    tileMap.setTile(tx + dx, ty, type);
                }
            }
        }

        // 3. Parse Entities
        levelData.entities.clear();
        if (j.contains("entities") && j["entities"].is_array()) {
            for (const auto& entObj : j["entities"]) {
                std::string typeStr = entObj.value("type", "unknown");
                int gx = entObj.value("x", 0);
                int gy = entObj.value("y", 0);
                sf::Vector2f pos(gx * Constants::TILE_SIZE, gy * Constants::TILE_SIZE);

                std::unique_ptr<Entity> entity;
                if (typeStr == "mario") entity = std::make_unique<Mario>(pos);
                else if (typeStr == "luigi") entity = std::make_unique<Luigi>(pos);
                else if (typeStr == "toad") entity = std::make_unique<Toad>(pos);
                else if (typeStr == "peach") entity = std::make_unique<Peach>(pos);
                else if (typeStr == "mushroom") entity = std::make_unique<Mushroom>(pos);
                else if (typeStr == "oneup_mushroom") entity = std::make_unique<OneUpMushroom>(pos);
                else if (typeStr == "mini_mushroom") entity = std::make_unique<MiniMushroom>(pos);
                else if (typeStr == "mega_mushroom") entity = std::make_unique<MegaMushroom>(pos);
                else if (typeStr == "cape_feather") entity = std::make_unique<CapeFeather>(pos);
                else if (typeStr == "fire_flower") entity = std::make_unique<FireFlower>(pos);
                else if (typeStr == "star") entity = std::make_unique<Star>(pos);
                else if (typeStr == "coin") entity = std::make_unique<Coin>(pos);
                else if (typeStr == "star_coin") entity = std::make_unique<StarCoin>(pos);
                else if (typeStr == "pswitch") entity = std::make_unique<PSwitch>(pos);
                else if (typeStr == "pow_block") entity = std::make_unique<POWBlock>(pos);
                else if (typeStr == "trampoline") entity = std::make_unique<Trampoline>(pos);

                if (entity) {
                    levelData.entities.push_back(std::move(entity));
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "LevelLoader::loadLevel failed: " << e.what() << std::endl;
        return false;
    }
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

        // Default spawn and flagpole locations (or scan entities list)
        j["spawnPoint"] = { {"x", 2}, {"y", 18} };
        j["flagpole"] = { {"x", tileMap.getWidth() - 2}, {"y", 18} };

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
                tileObj["type"] = getTileTypeName(type);
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
            std::string typeStr = getEntityTypeName(*entity);
            if (typeStr == "unknown" || typeStr == "mario" || typeStr == "luigi" || typeStr == "toad" || typeStr == "peach") {
                // Skip player characters from standard entity save list if they represent spawn point
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
