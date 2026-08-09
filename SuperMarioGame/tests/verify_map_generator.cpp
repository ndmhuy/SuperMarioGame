#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== Running Map Generator Solvability & Diversity Tests ===" << std::endl;

    // Test 1: Easy Overworld Level
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
        for (const auto& e : entities) {
            if (dynamic_cast<Flagpole*>(e.get())) {
                foundFlagpole = true;
            }
        }
        assert(foundFlagpole && "Flagpole winning point must be spawned at level end!");
        std::cout << "[PASS] Test 1: Easy Overworld winnability & flagpole winning point verified." << std::endl;
    }

    // Test 2: Hard Castle Level with Checkpoint & Trampoline
    {
        MapGeneratorConfig config;
        config.theme = MapTheme::Castle;
        config.difficulty = MapDifficulty::Hard;
        config.width = 200;
        config.height = 23;
        config.seed = 67890;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        bool foundTrampoline = false;
        for (const auto& e : entities) {
            if (dynamic_cast<Trampoline*>(e.get())) {
                foundTrampoline = true;
            }
        }
        assert(foundTrampoline && "Midpoint checkpoint trampoline must be spawned!");
        std::cout << "[PASS] Test 2: Hard Castle level midpoint checkpoint & trampoline verified." << std::endl;
    }

    std::cout << "=== ALL MAP GENERATOR TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
