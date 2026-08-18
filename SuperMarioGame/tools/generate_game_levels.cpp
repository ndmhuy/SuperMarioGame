#include "Utils/MapGenerator.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/Constants.hpp"
#include <iostream>
#include <vector>
#include <memory>

int main() {
    std::cout << "=== Generating Super Mario 3-World Campaign Levels & Sub-Levels ===" << std::endl;

    LevelLoader loader;

    auto saveBoth = [&](const std::string& relPath, TileMap& map, std::vector<std::unique_ptr<Entity>>& ents, const std::string& name) {
        loader.saveLevel(relPath, map, ents, name);
        loader.saveLevel("../" + relPath, map, ents, name);
        std::cout << "[SUCCESS] Generated " << relPath << std::endl;
    };


    // --- WORLD 1: Overworld & Underground Vault ---
    {
        MapGeneratorConfig config1;
        config1.theme = MapTheme::Overworld;
        config1.difficulty = MapDifficulty::Easy;
        config1.width = 200;
        config1.height = 23;
        config1.seed = 10101;

        TileMap map1;
        std::vector<std::unique_ptr<Entity>> entities1;
        MapGenerator::generate(map1, entities1, config1);
        saveBoth("assets/levels/level_1.json", map1, entities1, "World 1-1: Grassland Overworld");

        TileMap subMap1;
        std::vector<std::unique_ptr<Entity>> subEntities1;
        sf::Vector2f returnPos1(92.0f * Constants::TILE_SIZE, 19.0f * Constants::TILE_SIZE);
        MapGenerator::generateSubLevel(subMap1, subEntities1, MapTheme::Underground, MapDifficulty::Easy, "assets/levels/level_1.json", returnPos1, 10102);
        saveBoth("assets/levels/level_1_sub.json", subMap1, subEntities1, "World 1-1 Sub: Underground Coin Vault");
    }

    // --- WORLD 2: Ice / Cavern & Sky Canopy Vault ---
    {
        MapGeneratorConfig config2;
        config2.theme = MapTheme::Ice;
        config2.difficulty = MapDifficulty::Medium;
        config2.width = 200;
        config2.height = 23;
        config2.seed = 20202;

        TileMap map2;
        std::vector<std::unique_ptr<Entity>> entities2;
        MapGenerator::generate(map2, entities2, config2);
        saveBoth("assets/levels/level_2.json", map2, entities2, "World 1-2: Ice & Cavern Path");

        TileMap subMap2;
        std::vector<std::unique_ptr<Entity>> subEntities2;
        sf::Vector2f returnPos2(92.0f * Constants::TILE_SIZE, 19.0f * Constants::TILE_SIZE);
        MapGenerator::generateSubLevel(subMap2, subEntities2, MapTheme::Overworld, MapDifficulty::Medium, "assets/levels/level_2.json", returnPos2, 20203);
        saveBoth("assets/levels/level_2_sub.json", subMap2, subEntities2, "World 1-2 Sub: Sky Platform Canopy");
    }

    // --- WORLD 3: Castle Fortress & Secret Lava Vault ---
    {
        MapGeneratorConfig config3;
        config3.theme = MapTheme::Castle;
        config3.difficulty = MapDifficulty::Hard;
        config3.enableLava = true;
        config3.width = 200;
        config3.height = 23;
        config3.seed = 30303;

        TileMap map3;
        std::vector<std::unique_ptr<Entity>> entities3;
        MapGenerator::generate(map3, entities3, config3);
        saveBoth("assets/levels/level_3.json", map3, entities3, "World 1-3: Bowser's Castle Fortress");

        TileMap subMap3;
        std::vector<std::unique_ptr<Entity>> subEntities3;
        sf::Vector2f returnPos3(92.0f * Constants::TILE_SIZE, 19.0f * Constants::TILE_SIZE);
        MapGenerator::generateSubLevel(subMap3, subEntities3, MapTheme::Castle, MapDifficulty::Hard, "assets/levels/level_3.json", returnPos3, 30304);
        saveBoth("assets/levels/level_3_sub.json", subMap3, subEntities3, "World 1-3 Sub: Castle Lava Secret Vault");
    }

    // --- BONUS STAGE 1 ---
    {
        MapGeneratorConfig configBonus;
        configBonus.theme = MapTheme::Overworld;
        configBonus.difficulty = MapDifficulty::Easy;
        configBonus.coinClusterRate = 0.5f;
        configBonus.width = 120;
        configBonus.height = 23;
        configBonus.seed = 40404;

        TileMap bonusMap;
        std::vector<std::unique_ptr<Entity>> bonusEntities;
        MapGenerator::generate(bonusMap, bonusEntities, configBonus);
        saveBoth("assets/levels/bonus_1.json", bonusMap, bonusEntities, "Bonus Stage 1: Coin Paradise");
    }


    std::cout << "=== ALL 3 CAMPAIGN LEVELS & SUB-LEVELS SUCCESSFULLY GENERATED! ===" << std::endl;
    return 0;
}
