#pragma once

#include <string>
#include <vector>
#include <memory>

class Entity;
class TileMap;

struct LevelData {
    std::string name;
    std::string theme;
    int width = 0;
    int height = 0;
    std::vector<std::unique_ptr<Entity>> entities;
};

class LevelLoader {
public:
    LevelLoader() = default;
    ~LevelLoader() = default;

    // Load file to build TileMap and spawn entities
    bool loadLevel(const std::string& jsonPath, TileMap& tileMap, LevelData& levelData);

    // Save active levels back to JSON files
    bool saveLevel(const std::string& jsonPath, const TileMap& tileMap, 
                   const std::vector<std::unique_ptr<Entity>>& entities, const std::string& levelName);
};
