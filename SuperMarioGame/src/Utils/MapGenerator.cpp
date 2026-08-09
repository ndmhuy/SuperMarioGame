#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Mario.hpp"
#include "Utils/Constants.hpp"
#include <random>
#include <algorithm>
#include <iostream>

void MapGenerator::generate(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const MapGeneratorConfig& config) {
    tileMap.initialize(config.width, config.height);
    entities.clear();

    unsigned int seed = (config.seed != 0) ? config.seed : static_cast<unsigned int>(std::random_device{}());
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_int_distribution<int> pipeHeightDist(2, 4);
    std::uniform_int_distribution<int> platformLengthDist(3, 7);

    TileType groundTile = (config.theme == MapTheme::Ice) ? TileType::Ice : TileType::Ground;
    TileType blockTile = (config.theme == MapTheme::Ice) ? TileType::Ice : TileType::Brick;

    // 1. Fill base ground floor with occasional pits
    int groundRow1 = config.height - 2;
    int groundRow2 = config.height - 1;

    int currentX = 0;
    while (currentX < config.width) {
        // Guarantee solid ground for initial spawn (0..15) and level exit (width-15..width)
        bool isSafeZone = (currentX < 15 || currentX > config.width - 15);

        if (!isSafeZone && dist01(rng) < config.pitProbability) {
            // Generate a pit (gap in ground) 2..4 tiles wide
            int pitWidth = 2 + static_cast<int>(dist01(rng) * 3.0f);
            for (int dx = 0; dx < pitWidth && (currentX + dx) < config.width - 15; ++dx) {
                tileMap.setTile(currentX + dx, groundRow1, TileType::Empty);
                tileMap.setTile(currentX + dx, groundRow2, TileType::Empty);
            }
            currentX += pitWidth + 1;
            continue;
        }

        tileMap.setTile(currentX, groundRow1, groundTile);
        tileMap.setTile(currentX, groundRow2, groundTile);
        currentX++;
    }

    // 2. Spawn Player at start position
    auto player = std::make_unique<Mario>(sf::Vector2f(96.0f, (groundRow1 - 2) * Constants::TILE_SIZE));
    entities.push_back(std::move(player));

    // 3. Generate Level Structures (Pipes, Floating Platforms, Question Blocks, Staircases)
    for (int x = 16; x < config.width - 20; ++x) {
        // Ensure ground exists under structure
        if (tileMap.getTileType(x, groundRow1) == TileType::Empty) continue;

        // 3a. Pipes
        if (dist01(rng) < config.pipeFrequency) {
            int pHeight = pipeHeightDist(rng);
            for (int ph = 0; ph < pHeight; ++ph) {
                int py = groundRow1 - 1 - ph;
                if (py >= 0) {
                    tileMap.setTile(x, py, TileType::Pipe);
                    tileMap.setTile(x + 1, py, TileType::Pipe);
                }
            }
            // Spawn Piranha Plant in pipe if enemy spawn rate allows
            if (dist01(rng) < config.enemySpawnRate) {
                sf::Vector2f piranhaPos((x + 0.5f) * Constants::TILE_SIZE, (groundRow1 - pHeight) * Constants::TILE_SIZE);
                auto piranha = EntityFactory::create(EntityType::PiranhaPlant, piranhaPos);
                if (piranha) entities.push_back(std::move(piranha));
            }
            x += 3; // Space out next structure
            continue;
        }

        // 3b. Floating Brick & Question Block Rows
        if (dist01(rng) < config.coinClusterRate) {
            int blockRow = groundRow1 - 4;
            int length = platformLengthDist(rng);

            for (int lx = 0; lx < length && (x + lx) < config.width - 20; ++lx) {
                if (lx == length / 2 || lx == 1) {
                    tileMap.setTile(x + lx, blockRow, TileType::Question);
                } else if (dist01(rng) < 0.3f) {
                    tileMap.setTile(x + lx, blockRow, TileType::Coin);
                } else {
                    tileMap.setTile(x + lx, blockRow, blockTile);
                }
            }

            // Spawn Enemy underneath or on top of platform
            if (dist01(rng) < config.enemySpawnRate) {
                EntityType enemyTypes[] = { EntityType::Goomba, EntityType::KoopaTroopa, EntityType::KoopaParatroopa, EntityType::Spiny };
                EntityType chosenEnemy = enemyTypes[static_cast<int>(dist01(rng) * 4)];
                sf::Vector2f ePos((x + 2) * Constants::TILE_SIZE, (groundRow1 - 1) * Constants::TILE_SIZE);
                auto enemy = EntityFactory::create(chosenEnemy, ePos);
                if (enemy) entities.push_back(std::move(enemy));
            }

            x += length + 2;
            continue;
        }

        // 3c. Staircase Pyramids
        if (dist01(rng) < 0.04f && x < config.width - 30) {
            int height = 4;
            for (int h = 1; h <= height; ++h) {
                for (int w = 0; w < (height - h + 1); ++w) {
                    tileMap.setTile(x + w, groundRow1 - h, groundTile);
                }
            }
            x += height + 3;
            continue;
        }
    }

    // 4. Spawn Goal Flagpole near end of level
    int exitX = config.width - 12;
    sf::Vector2f flagpolePos(exitX * Constants::TILE_SIZE, (groundRow1 - 9) * Constants::TILE_SIZE);
    auto flagpole = EntityFactory::create(EntityType::Flagpole, flagpolePos);
    if (flagpole) entities.push_back(std::move(flagpole));

    // Spawn bonus collectibles (POWBlock, Trampoline)
    sf::Vector2f powPos((exitX - 8) * Constants::TILE_SIZE, (groundRow1 - 1) * Constants::TILE_SIZE);
    auto powBlock = EntityFactory::create(EntityType::POWBlock, powPos);
    if (powBlock) entities.push_back(std::move(powBlock));

    sf::Vector2f trampPos(35 * Constants::TILE_SIZE, (groundRow1 - 1) * Constants::TILE_SIZE);
    auto tramp = EntityFactory::create(EntityType::Trampoline, trampPos);
    if (tramp) entities.push_back(std::move(tramp));

    std::cout << "[MapGenerator] Generated " << config.width << "x" << config.height << " level with seed " << seed << std::endl;
}
