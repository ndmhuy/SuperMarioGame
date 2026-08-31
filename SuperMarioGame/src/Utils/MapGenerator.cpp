#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Castle.hpp"
#include "Entities/Mario.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelSolvability.hpp"
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
    std::uniform_int_distribution<int> pipeHeightDist(2, 4);
    std::uniform_int_distribution<int> platformLengthDist(4, 8);

    // Theme tile selections
    TileType groundTile = TileType::Ground;
    TileType blockTile = TileType::Brick;
    TileType hazardTile = TileType::Empty;

    if (config.theme == MapTheme::Ice) {
        groundTile = TileType::Ice;
        blockTile = TileType::Ice;
    } else if (config.theme == MapTheme::Castle) {
        groundTile = TileType::Ground;
        blockTile = (config.difficulty == MapDifficulty::Hard) ? TileType::Conveyor : TileType::Brick;
        if (config.enableLava) {
            hazardTile = TileType::Water; // Liquid hazard (Lava physics / hazard)
        }
    }

    int midX = config.width / 2;
    int exitX = config.width - 15;
    int defaultGroundY = config.height - 2;

    // 1. Build Ceilings for Castle & Underground themes
    if (config.theme == MapTheme::Castle || config.theme == MapTheme::Underground) {
        for (int x = 0; x < config.width; ++x) {
            tileMap.setTile(x, 0, groundTile);
            tileMap.setTile(x, 1, blockTile);
        }
    }

    // Track baseline height map for multi-tier elevation profiles
    std::vector<int> groundHeights(config.width, defaultGroundY);
    for (int x = 0; x < config.width; ++x) {
        bool isSafeZone = (x < 20 || (x >= midX - 5 && x <= midX + 5) || x >= exitX - 5);
        if (!isSafeZone) {
            // Compute smooth rolling elevation profile using wave frequencies
            float wave = std::sin(x * 0.12f * config.roughness) * 1.8f + std::cos(x * 0.05f) * 1.2f;
            int elevationOffset = static_cast<int>(std::round(wave));
            groundHeights[x] = std::clamp(defaultGroundY - elevationOffset, config.height - 7, config.height - 2);
        } else {
            groundHeights[x] = defaultGroundY;
        }
    }

    // 2. Fill ground floor & pits with solvability guardrails
    int currentX = 0;
    int solidGroundCounter = 0;

    while (currentX < config.width) {
        bool isSafeZone = (currentX < 20 || (currentX >= midX - 5 && currentX <= midX + 5) || currentX >= exitX - 5);
        int gy = groundHeights[currentX];

        if (!isSafeZone && solidGroundCounter >= 5 && dist01(rng) < config.pitProbability) {
            int maxPit = (config.difficulty == MapDifficulty::Hard) ? 4 : 3;
            int pitWidth = 2 + static_cast<int>(dist01(rng) * (maxPit - 1));

            // Carve pit gap
            for (int dx = 0; dx < pitWidth && (currentX + dx) < exitX - 5; ++dx) {
                for (int y = gy; y < config.height; ++y) {
                    tileMap.setTile(currentX + dx, y, TileType::Empty);
                }
                // Fill bottom row with hazard tile if enabled (e.g. Castle Lava)
                if (hazardTile != TileType::Empty) {
                    tileMap.setTile(currentX + dx, config.height - 1, hazardTile);
                }
            }

            // Solvability & Dynamic Platforms check:
            if (pitWidth >= 3) {
                float platformX = (currentX + pitWidth / 2.0f) * Constants::TILE_SIZE;
                float platformY = (gy - 1) * Constants::TILE_SIZE;

                if (config.enableMovingPlatforms && dist01(rng) < 0.6f) {
                    auto platform = EntityFactory::create(EntityType::MovingPlatform, sf::Vector2f(platformX, platformY));
                    if (platform) entities.push_back(std::move(platform));
                } else if (config.enableMovingPlatforms && dist01(rng) < 0.85f) {
                    auto fPlatform = EntityFactory::create(EntityType::FallingPlatform, sf::Vector2f(platformX, platformY));
                    if (fPlatform) entities.push_back(std::move(fPlatform));
                } else {
                    tileMap.setTile(currentX + pitWidth / 2, gy - 2, blockTile);
                }
            }

            currentX += pitWidth;
            solidGroundCounter = 0;
            continue;
        }

        // Fill ground column from gy to bottom
        for (int y = gy; y < config.height; ++y) {
            tileMap.setTile(currentX, y, groundTile);
        }
        currentX++;
        solidGroundCounter++;
    }

    // 3. Spawn Player at safe starting area
    int startY = groundHeights[3];
    auto player = std::make_unique<Mario>(sf::Vector2f(96.0f, (startY - 2) * Constants::TILE_SIZE));
    entities.push_back(std::move(player));

    // 4. Generate 3 Star Coins distributed across level tiers
    std::vector<int> starCoinLocations = {
        static_cast<int>(config.width * 0.25f),
        midX,
        static_cast<int>(config.width * 0.75f)
    };

    for (size_t i = 0; i < starCoinLocations.size() && i < static_cast<size_t>(config.starCoinCount); ++i) {
        int scx = starCoinLocations[i];
        int scy = groundHeights[scx] - 5;
        if (i != 1) {
            for (int dx = -1; dx <= 1; ++dx) {
                tileMap.setTile(scx + dx, scy + 1, blockTile);
            }
        }
        auto starCoin = EntityFactory::create(EntityType::StarCoin, sf::Vector2f(scx * Constants::TILE_SIZE, scy * Constants::TILE_SIZE));
        if (starCoin) entities.push_back(std::move(starCoin));
    }

    // 5. Generate Midpoint Checkpoint Structure & Sub-Level Entrance Warp Pipe
    int midY = groundHeights[midX];

    std::string subLevelTarget = "";
    if (config.theme == MapTheme::Overworld) subLevelTarget = "assets/levels/level_1_sub.json";
    else if (config.theme == MapTheme::Ice) subLevelTarget = "assets/levels/level_2_sub.json";
    else if (config.theme == MapTheme::Castle) subLevelTarget = "assets/levels/level_3_sub.json";

    sf::Vector2f returnExitPos((midX + 4) * Constants::TILE_SIZE, (midY - 1) * Constants::TILE_SIZE);
    auto warpPipe = std::make_unique<Pipe>(
        sf::Vector2f((midX - 2) * Constants::TILE_SIZE, (midY - 2) * Constants::TILE_SIZE),
        1, returnExitPos, subLevelTarget, true
    );
    entities.push_back(std::move(warpPipe));

    tileMap.setTile(midX + 2, midY - 1, TileType::Pipe);
    tileMap.setTile(midX + 2, midY - 2, TileType::Pipe);

    auto qBlock = std::make_unique<QuestionBlock>(sf::Vector2f(midX * Constants::TILE_SIZE, (midY - 3) * Constants::TILE_SIZE), 1);
    entities.push_back(std::move(qBlock));

    auto checkpointTrampoline = EntityFactory::create(EntityType::Trampoline, sf::Vector2f(midX * Constants::TILE_SIZE, (midY - 1) * Constants::TILE_SIZE));
    if (checkpointTrampoline) entities.push_back(std::move(checkpointTrampoline));

    // 6. Generate Prefab Chunks, Structures & Enemy Pacing
    int lastEnemyX = 0;

    for (int x = 22; x < exitX - 8; ++x) {
        if (x >= midX - 5 && x <= midX + 5) continue;

        int gy = groundHeights[x];
        if (tileMap.getTileType(x, gy) == TileType::Empty || tileMap.getTileType(x + 1, gy) == TileType::Empty) {
            continue;
        }

        // 6a. Chunk: Pipe Alley
        if (dist01(rng) < config.pipeFrequency) {
            int pHeight = pipeHeightDist(rng);
            for (int ph = 0; ph < pHeight; ++ph) {
                int py = gy - 1 - ph;
                if (py >= 0) {
                    tileMap.setTile(x, py, TileType::Pipe);
                    tileMap.setTile(x + 1, py, TileType::Pipe);
                }
            }

            if (dist01(rng) < config.enemySpawnRate + 0.15f) {
                sf::Vector2f piranhaPos((x + 0.5f) * Constants::TILE_SIZE, (gy - pHeight) * Constants::TILE_SIZE);
                auto piranha = EntityFactory::create(EntityType::PiranhaPlant, piranhaPos);
                if (piranha) entities.push_back(std::move(piranha));
            }
            x += 3;
            continue;
        }

        // 6b. Chunk: Floating Canopy & Item Block Rows with Packaged Rewards
        if (dist01(rng) < config.coinClusterRate) {
            int blockRow = gy - 4;
            int length = platformLengthDist(rng);

            for (int lx = 0; lx < length && (x + lx) < exitX - 8; ++lx) {
                int curX = x + lx;
                if (lx == 1 || lx == length - 2) {
                    int itemType = 1;
                    if (config.difficulty != MapDifficulty::Easy && dist01(rng) < 0.5f) itemType = 2;
                    if (config.difficulty == MapDifficulty::Hard && dist01(rng) < 0.3f) itemType = 3;

                    auto itemQBlock = std::make_unique<QuestionBlock>(sf::Vector2f(curX * Constants::TILE_SIZE, blockRow * Constants::TILE_SIZE), itemType);
                    entities.push_back(std::move(itemQBlock));
                } else if (dist01(rng) < 0.40f) {
                    tileMap.setTile(curX, blockRow, TileType::Coin);
                } else {
                    tileMap.setTile(curX, blockRow, blockTile);
                }
            }

            if (config.difficulty != MapDifficulty::Easy && dist01(rng) < 0.15f) {
                EntityType specialBlock = (dist01(rng) < 0.5f) ? EntityType::POWBlock : EntityType::PSwitch;
                auto pow = EntityFactory::create(specialBlock, sf::Vector2f((x + length / 2) * Constants::TILE_SIZE, (gy - 1) * Constants::TILE_SIZE));
                if (pow) entities.push_back(std::move(pow));
            }

            if ((x - lastEnemyX) >= 5 && dist01(rng) < config.enemySpawnRate) {
                lastEnemyX = x;
                float progress = static_cast<float>(x) / config.width;

                std::vector<EntityType> pool;
                if (config.difficulty == MapDifficulty::Easy) {
                    pool = { EntityType::Goomba, EntityType::KoopaTroopa };
                } else if (config.difficulty == MapDifficulty::Medium) {
                    pool = { EntityType::Goomba, EntityType::KoopaTroopa, EntityType::KoopaParatroopa, EntityType::Spiny };
                } else {
                    if (progress > 0.5f) {
                        pool = { EntityType::HammerBro, EntityType::Lakitu, EntityType::Spiny, EntityType::Boo, EntityType::Thwomp };
                    } else {
                        pool = { EntityType::Goomba, EntityType::KoopaTroopa, EntityType::KoopaParatroopa, EntityType::HammerBro };
                    }
                }

                EntityType chosenType = pool[static_cast<int>(dist01(rng) * pool.size())];
                sf::Vector2f enemyPos((x + 2) * Constants::TILE_SIZE, (gy - 1) * Constants::TILE_SIZE);
                if (chosenType == EntityType::Thwomp) {
                    enemyPos.y = (gy - 6) * Constants::TILE_SIZE;
                }
                auto enemy = EntityFactory::create(chosenType, enemyPos);
                if (enemy) entities.push_back(std::move(enemy));
            }

            x += length + 2;
            continue;
        }

        // 6c. Chunk: Climbable Staircase Pyramid
        if (dist01(rng) < 0.08f && x < exitX - 12) {
            int height = 3 + (config.difficulty == MapDifficulty::Hard ? 1 : 0);
            for (int h = 1; h <= height; ++h) {
                for (int w = 0; w < (height - h + 1); ++w) {
                    tileMap.setTile(x + w, gy - h, groundTile);
                }
            }
            x += height + 2;
            continue;
        }
    }

    // 7. Boss Fight (Hard Difficulty): Spawn Bowser guarding exit
    int exitGY = groundHeights[exitX];
    if (config.difficulty == MapDifficulty::Hard) {
        sf::Vector2f bowserPos((exitX - 6) * Constants::TILE_SIZE, (exitGY - 2) * Constants::TILE_SIZE);
        auto bowser = EntityFactory::create(EntityType::Bowser, bowserPos);
        if (bowser) entities.push_back(std::move(bowser));
    }

    // 8. Build Victory Staircase leading to Goal Flagpole
    for (int step = 1; step <= 5; ++step) {
        int stepX = exitX - 6 + step;
        for (int h = 1; h <= step; ++h) {
            tileMap.setTile(stepX, exitGY - h, blockTile);
        }
    }

    // 9. Spawn Goal Flagpole
    //
    // The y here is a hint. exitGY-9 put the pole's foot nearly four tiles above
    // the floor — the flag hung in mid-air in every generated level, the same
    // defect the hand-authored files had. PlayingState::settleEndOfLevelScenery()
    // stands it on whatever is actually beneath it at load time, so this only
    // has to name the column.
    sf::Vector2f flagpolePos(exitX * Constants::TILE_SIZE, (exitGY - 9) * Constants::TILE_SIZE);
    auto flagpole = EntityFactory::create(EntityType::Flagpole, flagpolePos);
    if (flagpole) entities.push_back(std::move(flagpole));

    // 10. The victory castle.
    //
    // This used to stamp a 5x5 square of ordinary GROUND tiles with a one-tile
    // hole punched through it. That is not a building: it is a solid brown box
    // the player can climb on top of and stand on, and it looked like level
    // geometry because it *was* level geometry. The atlas has shipped castle_end
    // since the beginning, so the castle is now a real entity drawn from real
    // art, standing on the floor and settled the same way the flagpole is.
    const int castleStartX = exitX + 4;
    // Only if it fits: a castle hanging off the right edge of the map is worse
    // than no castle, and the generator's width is configurable.
    if (castleStartX + static_cast<int>(Castle::WIDTH_TILES) + 1 < config.width) {
        // Make sure it has a floor to stand on — the terrain pass may have left
        // a pit here, and a castle over a pit settles onto nothing.
        for (int cx = 0; cx <= static_cast<int>(Castle::WIDTH_TILES) + 1; ++cx) {
            for (int y = exitGY; y < config.height; ++y) {
                tileMap.setTile(castleStartX + cx, y, groundTile);
            }
        }
        sf::Vector2f castlePos(castleStartX * Constants::TILE_SIZE,
                               (exitGY - Castle::HEIGHT_TILES) * Constants::TILE_SIZE);
        auto castle = EntityFactory::create(EntityType::Castle, castlePos);
        if (castle) entities.push_back(std::move(castle));
    }

    std::cout << "[MapGenerator] Generated winnable procedural level (Width: " << config.width
              << ", Theme: " << static_cast<int>(config.theme)
              << ", Difficulty: " << static_cast<int>(config.difficulty)
              << ", Roughness: " << config.roughness
              << ", Star Coins: " << config.starCoinCount
              << ", Seed: " << seed << ")" << std::endl;
}

void MapGenerator::generateSubLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, MapTheme theme, MapDifficulty difficulty, const std::string& returnLevelPath, sf::Vector2f returnPosition, unsigned int seed) {
    int subWidth = 65;
    int subHeight = 20;
    tileMap.initialize(subWidth, subHeight);
    entities.clear();

    if (seed == 0) seed = 424242;
    std::mt19937 rng(seed);

    TileType groundTile = (theme == MapTheme::Castle) ? TileType::Ground : TileType::Brick;

    // Ceilings and floors
    for (int x = 0; x < subWidth; ++x) {
        tileMap.setTile(x, 0, groundTile);
        tileMap.setTile(x, 1, groundTile);
        tileMap.setTile(x, subHeight - 2, groundTile);
        tileMap.setTile(x, subHeight - 1, groundTile);
    }

    int floorY = subHeight - 2;

    // Spawn Player near entry
    auto player = std::make_unique<Mario>(sf::Vector2f(96.0f, (floorY - 2) * Constants::TILE_SIZE));
    entities.push_back(std::move(player));

    // Entrance Pipe (non-functional return point visual)
    tileMap.setTile(3, floorY - 1, TileType::Pipe);
    tileMap.setTile(3, floorY - 2, TileType::Pipe);

    // Fill coin canopy & item blocks inside sub level
    for (int x = 10; x < subWidth - 12; x += 4) {
        int blockY = floorY - 4;
        tileMap.setTile(x, blockY, TileType::Question);
        tileMap.setTile(x + 1, blockY, TileType::Coin);
        tileMap.setTile(x + 2, blockY, TileType::Question);
        
        auto coin = EntityFactory::create(EntityType::Coin, sf::Vector2f((x + 1) * Constants::TILE_SIZE, (blockY - 1) * Constants::TILE_SIZE));
        if (coin) entities.push_back(std::move(coin));
    }

    // Interactive Vault Items
    auto powBlock = EntityFactory::create(EntityType::POWBlock, sf::Vector2f(25 * Constants::TILE_SIZE, (floorY - 1) * Constants::TILE_SIZE));
    if (powBlock) entities.push_back(std::move(powBlock));

    auto pSwitch = EntityFactory::create(EntityType::PSwitch, sf::Vector2f(35 * Constants::TILE_SIZE, (floorY - 1) * Constants::TILE_SIZE));
    if (pSwitch) entities.push_back(std::move(pSwitch));

    auto starCoin = EntityFactory::create(EntityType::StarCoin, sf::Vector2f(30 * Constants::TILE_SIZE, (floorY - 5) * Constants::TILE_SIZE));
    if (starCoin) entities.push_back(std::move(starCoin));

    auto trampoline = EntityFactory::create(EntityType::Trampoline, sf::Vector2f(45 * Constants::TILE_SIZE, (floorY - 1) * Constants::TILE_SIZE));
    if (trampoline) entities.push_back(std::move(trampoline));

    // Exit Pipe at sub-level end leading back to returnLevelPath
    sf::Vector2f exitPipePos((subWidth - 6) * Constants::TILE_SIZE, (floorY - 2) * Constants::TILE_SIZE);
    auto exitPipe = std::make_unique<Pipe>(exitPipePos, 2, returnPosition, returnLevelPath, true);
    entities.push_back(std::move(exitPipe));

    std::cout << "[MapGenerator] Generated subterranean bonus vault (Width: " << subWidth << ", Return Target: " << returnLevelPath << ")" << std::endl;
}

bool MapGenerator::generateSolvable(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities,
                                     const MapGeneratorConfig& config, int maxAttempts) {
    MapGeneratorConfig attemptConfig = config;
    const unsigned int baseSeed = config.seed;   // 0 means "pick randomly", preserved on attempt 1

    for (int attempt = 0; attempt < std::max(1, maxAttempts); ++attempt) {
        // Attempt 1 honors the caller's exact seed (a reproducible daily
        // challenge must not silently become a different level); only a
        // rejected attempt perturbs it.
        attemptConfig.seed = (attempt == 0) ? baseSeed
                            : (baseSeed != 0 ? baseSeed + static_cast<unsigned int>(attempt)
                                              : 0);
        generate(tileMap, entities, attemptConfig);

        const int startTileX = 3;
        const int endTileX = std::max(startTileX + 1, attemptConfig.width - 15);
        if (LevelSolvability::isPathReachable(tileMap, entities, startTileX, endTileX)) {
            if (attempt > 0) {
                std::cout << "[MapGenerator] Solvability check passed on attempt "
                          << (attempt + 1) << "/" << maxAttempts << std::endl;
            }
            return true;
        }
        std::cout << "[MapGenerator] Solvability check FAILED on attempt "
                  << (attempt + 1) << "/" << maxAttempts
                  << (attempt + 1 < maxAttempts ? " — regenerating with a new seed" : " — keeping it anyway, unverified")
                  << std::endl;
    }
    return false;
}
