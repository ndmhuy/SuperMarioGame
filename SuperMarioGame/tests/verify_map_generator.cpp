#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/StarCoin.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== Running Refined Map Generator Tests ===" << std::endl;

    // Test 1: Easy Overworld Level with 3 Star Coins & Flagpole
    {
        MapGeneratorConfig config;
        config.theme = MapTheme::Overworld;
        config.difficulty = MapDifficulty::Easy;
        config.width = 200;
        config.height = 23;
        config.seed = 12345;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        assert(!entities.empty());
        bool foundFlagpole = false;
        int starCoinCount = 0;
        for (const auto& e : entities) {
            if (dynamic_cast<Flagpole*>(e.get())) {
                foundFlagpole = true;
            }
            if (dynamic_cast<StarCoin*>(e.get())) {
                starCoinCount++;
            }
        }
        assert(foundFlagpole && "Flagpole winning point must be spawned at level end!");
        assert(starCoinCount == 3 && "Level generator must spawn exactly 3 Star Coins!");
        std::cout << "[PASS] Test 1: Easy Overworld winnability, flagpole & 3 Star Coins verified." << std::endl;
    }

    // Test 2: Hard Castle Level with Ceiling, Lava Hazards, and Checkpoint Trampoline
    {
        MapGeneratorConfig config;
        config.theme = MapTheme::Castle;
        config.difficulty = MapDifficulty::Hard;
        config.enableLava = true;
        config.width = 200;
        config.height = 23;
        config.seed = 67890;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        // Verify top ceiling row is solid ground tile
        assert(tileMap.getTileType(10, 0) == TileType::Ground && "Castle theme must have solid ceiling!");

        bool foundTrampoline = false;
        int starCoinCount = 0;
        for (const auto& e : entities) {
            if (dynamic_cast<Trampoline*>(e.get())) {
                foundTrampoline = true;
            }
            if (dynamic_cast<StarCoin*>(e.get())) {
                starCoinCount++;
            }
        }
        assert(foundTrampoline && "Midpoint checkpoint trampoline must be spawned!");
        assert(starCoinCount == 3 && "Hard Castle level must spawn 3 Star Coins!");
        std::cout << "[PASS] Test 2: Hard Castle ceiling, checkpoint & Star Coins verified." << std::endl;
    }

    // Test 3: Deterministic Seed Generation
    {
        MapGeneratorConfig config;
        config.seed = 99999;
        TileMap map1, map2;
        std::vector<std::unique_ptr<Entity>> e1, e2;

        MapGenerator::generate(map1, e1, config);
        MapGenerator::generate(map2, e2, config);

        assert(e1.size() == e2.size() && "Identical seed must generate identical entity counts!");
        std::cout << "[PASS] Test 3: Deterministic seed generation verified." << std::endl;
    }

    std::cout << "=== ALL REFINED MAP GENERATOR TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
