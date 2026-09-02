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

    // This map is a middle-of-nowhere slice of Endless Mode, not a level.
    //
    // A level has a beginning and an end and reserves room for both: a 22-tile
    // run-up before any prefab may appear and a 23-tile exit apron holding the
    // victory staircase, the flagpole and the castle. Those are fixed tile
    // counts, so on a 200-wide level they cost 22.5% of the map and on a
    // 100-tile Endless chunk they cost 45% — and the right-hand half of that
    // dead zone is exactly where the player ENTERS the chunk, looking for
    // something to play. A chunk has no beginning and no end, so it skips the
    // player spawn, the midpoint checkpoint, the victory staircase, the
    // flagpole and the castle, and fills [2, width-2) with content instead.
    bool isChunk = false;

    // Build a real Bowser encounter — arena, lava trench, bridge, posts and the
    // axe — instead of the bare Bowser this used to drop next to the exit with
    // no room, no floor of his own and no arena to lock the camera to.
    bool bossArena = false;
};

class MapGenerator {
public:
    static void generate(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const MapGeneratorConfig& config = MapGeneratorConfig());
    static void generateSubLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, MapTheme theme, MapDifficulty difficulty, const std::string& returnLevelPath, sf::Vector2f returnPosition, unsigned int seed = 0);

    // generate(), but checked: retries with a different seed (up to
    // maxAttempts times) until Utils/LevelSolvability confirms the spawn area
    // can actually reach the flagpole's approach on foot and by jump alone,
    // rather than trusting the placement-time pit/platform guardrails on
    // faith. Returns false if every attempt was rejected — the last attempt's
    // result is still left in tileMap/entities (a level is more useful to
    // look at than none), but the caller should log that it is unverified.
    static bool generateSolvable(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities,
                                  const MapGeneratorConfig& config, int maxAttempts = 3);

    // How wide a generated boss arena is, in tiles, and how much clear landing
    // ground it needs to its right. Public because the callers that decide
    // WHERE a boss chunk goes (PlayingState's Endless cadence) and the tests
    // that check one have to reserve the same room the generator will use.
    static constexpr int BOSS_ARENA_TILES   = 13;   // mirrors level_3.json's arenaW
    static constexpr int BOSS_LANDING_TILES = 10;   // apron right of the bridge

    // Minimum tiles between two ground enemies. Pacing, not a budget: without
    // it a run of eligible columns can stack three Goombas on top of each other.
    static constexpr int ENEMY_SPACING_TILES = 5;

private:
    // Stamps a Bowser encounter into [leftX, leftX + BOSS_ARENA_TILES) and its
    // landing apron to the right of that, and pushes the Bowser — arena already
    // assigned — into `entities`.
    //
    // Modelled tile-for-tile on level_3.json, which is the only hand-authored
    // Bowser fight in the game and therefore the only description of what the
    // encounter is supposed to be: an approach ledge, a lava trench, a solid
    // bridge one tile above the approach, a one-tile post at each end, Bowser
    // pacing the bridge, and the axe on the far side.
    static void buildBossArena(TileMap& tileMap,
                                std::vector<std::unique_ptr<Entity>>& entities,
                                const MapGeneratorConfig& config,
                                int leftX, TileType groundTile, TileType bridgeTile);
};


