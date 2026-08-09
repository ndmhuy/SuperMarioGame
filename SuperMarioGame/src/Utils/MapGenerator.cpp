#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Mario.hpp"
#include "Utils/Constants.hpp"
#include <random>
#include <algorithm>
#include <iostream>
#include <cmath>

void MapGenerator::generate(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const MapGeneratorConfig& config) {
    tileMap.initialize(config.width, config.height);
    entities.clear();

    unsigned int seed = (config.seed != 0) ? config.seed : static_cast<unsigned int>(std::random_device{}());
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_int_distribution<int> pipeHeightDist(2, 3);
    std::uniform_int_distribution<int> platformLengthDist(4, 7);

    TileType groundTile = TileType::Ground;
    TileType blockTile = TileType::Brick;

    if (config.theme == MapTheme::Ice) {
        groundTile = TileType::Ice;
        blockTile = TileType::Ice;
    } else if (config.theme == MapTheme::Castle) {
        groundTile = TileType::Ground;
        blockTile = (config.difficulty == MapDifficulty::Hard) ? TileType::Conveyor : TileType::Brick;
    }

    int groundRow1 = config.height - 2;
    int groundRow2 = config.height - 1;
    int midX = config.width / 2;
    int exitX = config.width - 15;

    // 1. Fill base ground floor with guaranteed solvability rules
    int currentX = 0;
    int solidGroundCounter = 0;

    while (currentX < config.width) {
        // Guaranteed solid ground zones: Start area (0..20), Midpoint Checkpoint (midX-4..midX+4), End area (exitX-5..width)
        bool isSafeZone = (currentX < 20 || (currentX >= midX - 4 && currentX <= midX + 4) || currentX >= exitX - 5);

        if (!isSafeZone && solidGroundCounter >= 4 && dist01(rng) < config.pitProbability) {
            // Determine max pit width based on difficulty (max 3 tiles = 96px, easily jumpable by player)
            int maxPit = (config.difficulty == MapDifficulty::Hard) ? 3 : 2;
            int pitWidth = 2 + static_cast<int>(dist01(rng) * (maxPit - 1));

            // Solvability check: if pit width == 3, add a floating stepping block in the center so jump is 100% winnable
            for (int dx = 0; dx < pitWidth && (currentX + dx) < exitX - 5; ++dx) {
                tileMap.setTile(currentX + dx, groundRow1, TileType::Empty);
                tileMap.setTile(currentX + dx, groundRow2, TileType::Empty);
            }

            if (pitWidth >= 3) {
                tileMap.setTile(currentX + 1, groundRow1 - 2, blockTile);
            }

            currentX += pitWidth;
            solidGroundCounter = 0;
            continue;
        }

        tileMap.setTile(currentX, groundRow1, groundTile);
        tileMap.setTile(currentX, groundRow2, groundTile);
        currentX++;
        solidGroundCounter++;
    }

    // 2. Spawn Player at safe starting area
    auto player = std::make_unique<Mario>(sf::Vector2f(96.0f, (groundRow1 - 2) * Constants::TILE_SIZE));
    entities.push_back(std::move(player));

    // 3. Generate Midpoint Checkpoint Structure
    tileMap.setTile(midX - 2, groundRow1 - 1, TileType::Pipe);
    tileMap.setTile(midX - 2, groundRow1 - 2, TileType::Pipe);
    tileMap.setTile(midX + 2, groundRow1 - 1, TileType::Pipe);
    tileMap.setTile(midX + 2, groundRow1 - 2, TileType::Pipe);

    tileMap.setTile(midX, groundRow1 - 3, TileType::Question);
    auto checkpointTrampoline = EntityFactory::create(EntityType::Trampoline, sf::Vector2f(midX * Constants::TILE_SIZE, (groundRow1 - 1) * Constants::TILE_SIZE));
    if (checkpointTrampoline) entities.push_back(std::move(checkpointTrampoline));

    auto starCoin = EntityFactory::create(EntityType::StarCoin, sf::Vector2f(midX * Constants::TILE_SIZE, (groundRow1 - 5) * Constants::TILE_SIZE));
    if (starCoin) entities.push_back(std::move(starCoin));

    // 4. Generate Structures & Diverse Difficulty Entities
    for (int x = 22; x < exitX - 8; ++x) {
        // Skip midpoint checkpoint zone
        if (x >= midX - 5 && x <= midX + 5) continue;

        // Skip if ground is a pit gap
        if (tileMap.getTileType(x, groundRow1) == TileType::Empty || tileMap.getTileType(x + 1, groundRow1) == TileType::Empty) {
            continue;
        }

        // 4a. Pipes
        if (dist01(rng) < config.pipeFrequency) {
            int pHeight = pipeHeightDist(rng);
            for (int ph = 0; ph < pHeight; ++ph) {
                int py = groundRow1 - 1 - ph;
                if (py >= 0) {
                    tileMap.setTile(x, py, TileType::Pipe);
                    tileMap.setTile(x + 1, py, TileType::Pipe);
                }
            }

            // Piranha Plant in pipe
            if (dist01(rng) < config.enemySpawnRate + 0.1f) {
                sf::Vector2f piranhaPos((x + 0.5f) * Constants::TILE_SIZE, (groundRow1 - pHeight) * Constants::TILE_SIZE);
                auto piranha = EntityFactory::create(EntityType::PiranhaPlant, piranhaPos);
                if (piranha) entities.push_back(std::move(piranha));
            }
            x += 3;
            continue;
        }

        // 4b. Floating Platforms & Item Block Rows
        if (dist01(rng) < config.coinClusterRate) {
            int blockRow = groundRow1 - 4;
            int length = platformLengthDist(rng);

            for (int lx = 0; lx < length && (x + lx) < exitX - 8; ++lx) {
                if (lx == 1 || lx == length - 2) {
                    tileMap.setTile(x + lx, blockRow, TileType::Question);
                } else if (dist01(rng) < 0.35f) {
                    tileMap.setTile(x + lx, blockRow, TileType::Coin);
                } else {
                    tileMap.setTile(x + lx, blockRow, blockTile);
                }
            }

            // Spawn Diverse Powerups & Items
            sf::Vector2f itemPos((x + 1) * Constants::TILE_SIZE, (blockRow - 1) * Constants::TILE_SIZE);
            if (dist01(rng) < 0.25f) {
                auto mushroom = EntityFactory::create(EntityType::Mushroom, itemPos);
                if (mushroom) entities.push_back(std::move(mushroom));
            } else if (dist01(rng) < 0.45f && config.difficulty != MapDifficulty::Easy) {
                auto flower = EntityFactory::create(EntityType::FireFlower, itemPos);
                if (flower) entities.push_back(std::move(flower));
            } else if (dist01(rng) < 0.65f && config.difficulty == MapDifficulty::Hard) {
                auto feather = EntityFactory::create(EntityType::CapeFeather, itemPos);
                if (feather) entities.push_back(std::move(feather));
            }

            // Spawn Diverse Enemies based on Theme & Difficulty
            if (dist01(rng) < config.enemySpawnRate) {
                std::vector<EntityType> pool;
                if (config.difficulty == MapDifficulty::Easy) {
                    pool = { EntityType::Goomba, EntityType::KoopaTroopa };
                } else if (config.difficulty == MapDifficulty::Medium) {
                    pool = { EntityType::Goomba, EntityType::KoopaTroopa, EntityType::KoopaParatroopa, EntityType::Spiny };
                } else { // Hard
                    pool = { EntityType::HammerBro, EntityType::Lakitu, EntityType::Spiny, EntityType::Boo, EntityType::Thwomp };
                }

                EntityType chosenType = pool[static_cast<int>(dist01(rng) * pool.size())];
                sf::Vector2f enemyPos((x + 2) * Constants::TILE_SIZE, (groundRow1 - 1) * Constants::TILE_SIZE);
                if (chosenType == EntityType::Thwomp) {
                    enemyPos.y = (groundRow1 - 6) * Constants::TILE_SIZE; // Ceiling trap position for Thwomp
                }
                auto enemy = EntityFactory::create(chosenType, enemyPos);
                if (enemy) entities.push_back(std::move(enemy));
            }

            x += length + 2;
            continue;
        }

        // 4c. Climbable Staircase Pyramids
        if (dist01(rng) < 0.05f && x < exitX - 12) {
            int height = 3;
            for (int h = 1; h <= height; ++h) {
                for (int w = 0; w < (height - h + 1); ++w) {
                    tileMap.setTile(x + w, groundRow1 - h, groundTile);
                }
            }
            x += height + 2;
            continue;
        }
    }

    // 5. Boss Fight (Hard Difficulty): Spawn Bowser guarding exit
    if (config.difficulty == MapDifficulty::Hard) {
        sf::Vector2f bowserPos((exitX - 6) * Constants::TILE_SIZE, (groundRow1 - 2) * Constants::TILE_SIZE);
        auto bowser = EntityFactory::create(EntityType::Bowser, bowserPos);
        if (bowser) entities.push_back(std::move(bowser));
    }

    // 6. Build Victory Staircase leading to Goal Flagpole (Winning Point)
    for (int step = 1; step <= 5; ++step) {
        int stepX = exitX - 6 + step;
        for (int h = 1; h <= step; ++h) {
            tileMap.setTile(stepX, groundRow1 - h, blockTile);
        }
    }

    // 7. Spawn Goal Flagpole (Winning Point)
    sf::Vector2f flagpolePos(exitX * Constants::TILE_SIZE, (groundRow1 - 9) * Constants::TILE_SIZE);
    auto flagpole = EntityFactory::create(EntityType::Flagpole, flagpolePos);
    if (flagpole) entities.push_back(std::move(flagpole));

    // 8. Build Victory Castle Structure after Flagpole
    int castleStartX = exitX + 4;
    for (int cx = 0; cx < 5; ++cx) {
        for (int cy = 1; cy <= 5; ++cy) {
            tileMap.setTile(castleStartX + cx, groundRow1 - cy, groundTile);
        }
    }
    // Castle Door Entrance (Empty cutout)
    tileMap.setTile(castleStartX + 2, groundRow1 - 1, TileType::Empty);
    tileMap.setTile(castleStartX + 2, groundRow2 - 2, TileType::Empty);

    std::cout << "[MapGenerator] Generated winnable level (Width: " << config.width << ", Theme: " << static_cast<int>(config.theme)
              << ", Difficulty: " << static_cast<int>(config.difficulty) << ", Midpoint Checkpoint: x=" << midX
              << ", Goal Flagpole: x=" << exitX << ", Seed: " << seed << ")" << std::endl;
}
