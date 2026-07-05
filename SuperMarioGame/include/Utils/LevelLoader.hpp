#pragma once

#include <string>
#include <vector>
#include <memory>
#include <SFML/System/Vector2.hpp>

class Entity;
class TileMap;

struct LevelData {
    std::string name;
    std::string theme;
    int width = 0;
    int height = 0;
    sf::Vector2f spawnPoint{64.0f, 640.0f};
    std::vector<std::unique_ptr<Entity>> entities;
};

class LevelLoader {
public:
    LevelLoader() = default;
    ~LevelLoader() = default;

    // Load file to build TileMap and spawn entities
    bool loadLevel(const std::string& jsonPath, TileMap& tileMap, LevelData& levelData);
};
