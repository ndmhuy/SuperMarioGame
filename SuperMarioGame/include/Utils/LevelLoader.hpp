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

    // Save active levels back to JSON files.
    //
    // The theme is written as "overworld" and the spawn point is guessed at
    // (2, 18) clamped to the map, because this overload has nothing better to
    // go on. Prefer the LevelData form below wherever the caller knows them —
    // the editor does, and a level that came back from a round trip with a
    // different backdrop and a spawn point in mid-air is not the level that was
    // saved.
    bool saveLevel(const std::string& jsonPath, const TileMap& tileMap,
                   const std::vector<std::unique_ptr<Entity>>& entities, const std::string& levelName);

    // As above, but persisting `meta`'s name, theme and spawn point.
    // `meta.entities` is ignored; the entities written are the ones passed
    // separately, so a caller may save a live world rather than a parsed file.
    bool saveLevel(const std::string& jsonPath, const TileMap& tileMap,
                   const std::vector<std::unique_ptr<Entity>>& entities,
                   const LevelData& meta);
};
