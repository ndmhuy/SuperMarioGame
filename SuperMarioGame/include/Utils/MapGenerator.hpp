#pragma once

#include "Utils/TileMap.hpp"
#include "Entities/Entity.hpp"
#include <vector>
#include <memory>
#include <string>

enum class MapTheme {
    Overworld,
    Underground,
    Castle,
    Ice
};

enum class MapDifficulty {
    Easy,
    Medium,
    Hard
};

struct MapGeneratorConfig {
    MapTheme theme = MapTheme::Overworld;
    MapDifficulty difficulty = MapDifficulty::Easy;
    int width = 200;
    int height = 23;
    float pitProbability = 0.1f;
    float pipeFrequency = 0.08f;
    float enemySpawnRate = 0.15f;
    float coinClusterRate = 0.2f;
    float roughness = 0.3f;             // Elevation profile noise/roughness
    bool enableLava = true;             // Castle theme pit hazards
    bool enableMovingPlatforms = true;  // Dynamic platforms across pits
    int starCoinCount = 3;              // Standard 3 Star Coins per level
    unsigned int seed = 0;              // 0 = random seed
};

class MapGenerator {
public:
    static void generate(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const MapGeneratorConfig& config = MapGeneratorConfig());
    static void generateSubLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, MapTheme theme, MapDifficulty difficulty, const std::string& returnLevelPath, sf::Vector2f returnPosition, unsigned int seed = 0);
};


