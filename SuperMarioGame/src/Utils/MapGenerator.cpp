#include "Utils/MapGenerator.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Castle.hpp"
#include "Entities/Mario.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Bowser.hpp"
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
            // Lava, not Water. The comment always said "Lava physics / hazard"
            // and the tile was Water, which is inert: the damage check tests
            // TileType::Lava and nothing else, so every generated castle pit was
            // decorative. It also made a generated castle's bridge chop
            // unimplementable, because PlayingState::chopBridge() identifies
            // bridge columns by the lava underneath them and there was none.
            //
            // This IS a difficulty change: pits in a generated castle that were
            // survivable scenery are now lethal.
            hazardTile = TileType::Lava;
        }
    }

    int midX = config.width / 2;
    int exitX = config.width - 15;
    int defaultGroundY = config.height - 2;

    // Where the boss arena goes, or -1 for no arena. Placed at the right-hand
    // end of the playable run in both modes: a chunk's arena is the last thing
    // in it, and a level's sits immediately before the victory staircase so the
    // flagpole is the reward for getting past Bowser rather than a detour
    // around him.
    const bool wantsBossArena = config.bossArena ||
                                (!config.isChunk && config.difficulty == MapDifficulty::Hard);
    const int bossSpan = BOSS_ARENA_TILES + BOSS_LANDING_TILES;
    int bossLeftX = -1;
    if (wantsBossArena) {
        const int candidate = config.isChunk ? (config.width - bossSpan)
                                              : (exitX - 6 - BOSS_ARENA_TILES);
        // Only if the whole encounter fits with a run-up in front of it; a
        // half-built arena is worse than the flat exit apron it replaces.
        if (candidate > 24) bossLeftX = candidate;
    }

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
                    // The sweep has to fit the pit. EntityFactory gives every
                    // platform a fixed four tiles to the right, but a pit here
                    // is only two to four tiles wide and the bank on the far
                    // side can stand up to five tiles higher than this one (see
                    // the elevation profile above), so a platform launched from
                    // the pit's CENTRE routinely finished its travel inside
                    // solid ground (D5). Anchored at the pit's left edge, and
                    // travelling only what a 2-tile-wide platform has left of
                    // the pit, it cannot leave the gap it was placed to bridge.
                    const int carvedWidth = std::min(pitWidth, std::max(0, (exitX - 5) - currentX));
                    const float sweepTiles = static_cast<float>(std::max(0, carvedWidth - 2));
                    auto platform = std::make_unique<MovingPlatform>(
                        sf::Vector2f(currentX * Constants::TILE_SIZE, platformY),
                        sf::Vector2f(sweepTiles * Constants::TILE_SIZE, 0.0f));
                    entities.push_back(std::move(platform));
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

    // 3. Spawn Player at safe starting area. A chunk is spliced into a level the
    // player is already alive in, so it must not build a second one.
    if (!config.isChunk) {
        int startY = groundHeights[3];
        auto player = std::make_unique<Mario>(sf::Vector2f(96.0f, (startY - 2) * Constants::TILE_SIZE));
        entities.push_back(std::move(player));
    }

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

    // 5. Generate Midpoint Checkpoint Structure & Sub-Level Entrance Warp Pipe.
    //
    // Skipped for a chunk: a checkpoint halfway to nowhere is meaningless, and
    // the warp pipe would drop the player out of an Endless run into a
    // hand-authored bonus vault whose exit pipe returns to a level path the run
    // does not have. The splice already discards the Pipe entity; without this
    // the pipe's TILES were spliced in anyway, leaving a half pipe and a
    // free-standing question block in the middle of every chunk.
    if (!config.isChunk) {
        int midY = groundHeights[midX];

        std::string subLevelTarget = "";
        if (config.theme == MapTheme::Overworld) subLevelTarget = "assets/levels/level_1_sub.json";
        else if (config.theme == MapTheme::Ice) subLevelTarget = "assets/levels/level_2_sub.json";
        else if (config.theme == MapTheme::Castle) subLevelTarget = "assets/levels/level_3_sub.json";

        sf::Vector2f returnExitPos((midX + 4) * Constants::TILE_SIZE, (midY - 1) * Constants::TILE_SIZE);
        // Three rows above the surface, not four.
        //
        // A Pipe is placed by its top-left corner and is Pipe::HEIGHT_PX (4 tiles)
        // tall, so seating its foot exactly on row midY would put its rim 128px up
        // — Constants::JUMP_HEIGHT to the pixel, i.e. the apex of the arc, for a
        // solid block the player has to get on top of to use. Set one row into the
        // ground instead: the rim is three tiles up and comfortably reachable, and
        // the buried row is hidden by the terrain drawn over it. Every authored
        // level places its pipes the same way.
        auto warpPipe = std::make_unique<Pipe>(
            sf::Vector2f((midX - 2) * Constants::TILE_SIZE, (midY - 3) * Constants::TILE_SIZE),
            1, returnExitPos, subLevelTarget, true
        );
        entities.push_back(std::move(warpPipe));

        // Two columns wide, like every other pipe in the game. cc6a32d fixed the
        // identical one-column bug in generateSubLevel but missed this call site, so
        // every procedurally generated overworld still emitted a half pipe at its
        // checkpoint (D22/D23, still live at R21).
        for (int pipeCol = midX + 2; pipeCol <= midX + 3; ++pipeCol) {
            tileMap.setTile(pipeCol, midY - 1, TileType::Pipe);
            tileMap.setTile(pipeCol, midY - 2, TileType::Pipe);
        }

        auto qBlock = std::make_unique<QuestionBlock>(sf::Vector2f(midX * Constants::TILE_SIZE, (midY - 3) * Constants::TILE_SIZE), 1);
        entities.push_back(std::move(qBlock));

        auto checkpointTrampoline = EntityFactory::create(EntityType::Trampoline, sf::Vector2f(midX * Constants::TILE_SIZE, (midY - 1) * Constants::TILE_SIZE));
        if (checkpointTrampoline) entities.push_back(std::move(checkpointTrampoline));
    }

    // 6. Generate Prefab Chunks, Structures & Enemy Pacing.
    //
    // The bounds used to be the literals 22 and exitX-8 whatever the map was.
    // See MapGeneratorConfig::isChunk for why that is a 45% dead zone on a
    // 100-tile Endless chunk and why the right-hand half of it is precisely
    // where the player arrives. Derived from the width now, and a chunk gets
    // essentially all of itself.
    const int prefabFirstX = config.isChunk ? 2
                                             : std::min(22, std::max(4, config.width / 9));
    int prefabLastX = config.isChunk ? config.width - 2 : exitX - 8;
    // Nothing may be built into the room the arena is about to claim.
    if (bossLeftX > 0) prefabLastX = std::min(prefabLastX, bossLeftX - 2);

    int lastEnemyX = 0;

    for (int x = prefabFirstX; x < prefabLastX; ++x) {
        if (!config.isChunk && x >= midX - 5 && x <= midX + 5) continue;

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

            for (int lx = 0; lx < length && (x + lx) < prefabLastX; ++lx) {
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

            x += length + 2;
            continue;
        }

        // 6c. Chunk: Climbable Staircase Pyramid
        if (dist01(rng) < 0.08f && x < prefabLastX - 4) {
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

    // 6d. Enemy pacing, as a pass of its own over every column.
    //
    // This used to be nested INSIDE the floating-canopy branch above, so a
    // ground enemy could only appear on a column that had already won the
    // coin-cluster roll: the effective density was coinClusterRate x
    // enemySpawnRate — about 0.6-1.3 enemies per 100-tile chunk at the default
    // 0.2 x 0.15, which is what "there are no entities in the far chunks"
    // actually was. It also has to be its own LOOP and not merely its own
    // branch, because the prefab loop skips x forward by 4-10 columns whenever
    // it builds something, so an enemy roll taken inside it is starved in
    // exactly the structure-rich stretches that most need pacing.
    for (int x = prefabFirstX; x < prefabLastX; ++x) {
        if (!config.isChunk && x >= midX - 5 && x <= midX + 5) continue;
        if ((x - lastEnemyX) < ENEMY_SPACING_TILES) continue;

        const int gy = groundHeights[x];
        // Two columns of floor to stand and walk on, and headroom above it: an
        // enemy dropped on a pipe alley or under a canopy row starts the level
        // already inside solid tiles.
        if (tileMap.getTileType(x, gy) == TileType::Empty ||
            tileMap.getTileType(x + 1, gy) == TileType::Empty) continue;
        if (TileMap::getInfo(tileMap.getTileType(x, gy - 1)).isSolid ||
            TileMap::getInfo(tileMap.getTileType(x, gy - 2)).isSolid) continue;
        if (dist01(rng) >= config.enemySpawnRate) continue;

        lastEnemyX = x;
        const float progress = static_cast<float>(x) / config.width;

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

        const EntityType chosenType = pool[static_cast<int>(dist01(rng) * pool.size())];
        sf::Vector2f enemyPos(x * Constants::TILE_SIZE, (gy - 1) * Constants::TILE_SIZE);
        if (chosenType == EntityType::Thwomp) {
            enemyPos.y = (gy - 6) * Constants::TILE_SIZE;
        }
        auto enemy = EntityFactory::create(chosenType, enemyPos);
        if (enemy) entities.push_back(std::move(enemy));
    }

    // 7. Boss Fight.
    //
    // This used to be four lines: drop a Bowser at exitX-6 on Hard and hope.
    // Nothing else about it was a boss FIGHT — no arena, so hasArena() was
    // false and PlayingState's camera lock, HUD health bar, battle music and
    // "no escape" clamp all sat behind a condition that could never be true; no
    // room, because he landed on the flat exit apron with the victory staircase
    // immediately to his right, so the player walked straight past him; no
    // bridge and no axe, so the non-combat solution the fight is balanced
    // around did not exist.
    int exitGY = groundHeights[exitX];
    if (bossLeftX > 0) {
        buildBossArena(tileMap, entities, config, bossLeftX, groundTile, TileType::Brick);
    }

    // 8. Build Victory Staircase leading to Goal Flagpole. A chunk has no exit.
    if (!config.isChunk) {
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
    }

    std::cout << "[MapGenerator] Generated winnable procedural level (Width: " << config.width
              << ", Theme: " << static_cast<int>(config.theme)
              << ", Difficulty: " << static_cast<int>(config.difficulty)
              << ", Roughness: " << config.roughness
              << ", Star Coins: " << config.starCoinCount
              << ", Seed: " << seed << ")" << std::endl;
}

void MapGenerator::buildBossArena(TileMap& tileMap,
                                   std::vector<std::unique_ptr<Entity>>& entities,
                                   const MapGeneratorConfig& config,
                                   int leftX, TileType groundTile, TileType bridgeTile) {
    const float TS = Constants::TILE_SIZE;
    const int floorRow  = config.height - 2;   // the map's default walking surface
    const int bridgeRow = floorRow - 1;        // one step up, as in level_3.json
    const int postRow   = floorRow - 2;
    const int trenchFirst = leftX + 1;
    const int trenchLast  = leftX + 8;         // 8 bridge tiles, as in level_3.json
    const int rightPostX  = leftX + 9;
    const int lastX = std::min(tileMap.getWidth() - 1,
                               leftX + BOSS_ARENA_TILES + BOSS_LANDING_TILES - 1);

    // The arena replaces whatever the terrain and prefab passes left here, so
    // clear it first — everything below the themed ceiling, which stays.
    const int clearFromRow = (config.theme == MapTheme::Castle ||
                              config.theme == MapTheme::Underground) ? 2 : 0;
    for (int x = leftX; x <= lastX; ++x) {
        for (int y = clearFromRow; y < config.height; ++y) {
            tileMap.setTile(x, y, TileType::Empty);
        }
    }

    // Two flat approach columns in front of it, so the run-up cannot arrive at
    // the arena over a pit the terrain pass happened to carve at its lip.
    for (int x = std::max(0, leftX - 2); x < leftX; ++x) {
        for (int y = floorRow; y < config.height; ++y) {
            tileMap.setTile(x, y, groundTile);
        }
    }

    // An entity the earlier passes put inside the arena is now standing in a
    // room that was built around it — a moving platform patrolling the lava, a
    // question block floating over the bridge. They go with the tiles.
    const float spanLeftPx  = leftX * TS;
    const float spanRightPx = (lastX + 1) * TS;
    entities.erase(std::remove_if(entities.begin(), entities.end(),
        [spanLeftPx, spanRightPx](const std::unique_ptr<Entity>& e) {
            if (!e) return false;
            const float x = e->getPosition().x;
            return x >= spanLeftPx && x < spanRightPx;
        }), entities.end());

    // The two posts and the ledges they stand on.
    for (int x : {leftX, rightPostX}) {
        tileMap.setTile(x, postRow, bridgeTile);
        for (int y = bridgeRow; y < config.height; ++y) {
            tileMap.setTile(x, y, groundTile);
        }
    }

    // The trench: lava all the way to the floor, spanned by a solid bridge one
    // row up. chopBridge() finds these columns by exactly this shape — a solid
    // tile with lava underneath it, inside the arena — so the bridge does not
    // have to be declared anywhere.
    for (int x = trenchFirst; x <= trenchLast; ++x) {
        for (int y = floorRow; y < config.height; ++y) {
            tileMap.setTile(x, y, TileType::Lava);
        }
        tileMap.setTile(x, bridgeRow, bridgeTile);
    }

    // The far ledge stays at bridge level for two more columns — that is what
    // the player lands on once the bridge is gone, and what the axe stands on —
    // then the apron steps back down to the map's own floor so the run
    // continues normally after the fight.
    const int ledgeLastX = rightPostX + 2;
    for (int x = rightPostX + 1; x <= std::min(ledgeLastX, lastX); ++x) {
        for (int y = bridgeRow; y < config.height; ++y) {
            tileMap.setTile(x, y, groundTile);
        }
    }
    for (int x = ledgeLastX + 1; x <= lastX; ++x) {
        for (int y = floorRow; y < config.height; ++y) {
            tileMap.setTile(x, y, groundTile);
        }
    }

    // Constructed directly rather than through EntityFactory::create, which
    // returns a unique_ptr<Entity> and would need a dynamic_cast back to Boss
    // just to hand it the arena it cannot be a fight without.
    auto bowser = std::make_unique<Bowser>(sf::Vector2f((leftX + 4) * TS, (bridgeRow - 2) * TS));
    bowser->setArena(AABB{leftX * TS, 0.0f,
                          BOSS_ARENA_TILES * TS,
                          config.height * TS});
    entities.push_back(std::move(bowser));

    // Three axes: the left end of the bridge, the far end of the bridge, and
    // the far ledge past both posts. All three on solid tiles, because a
    // BridgeAxe is an Item with the default gravity multiplier and one placed
    // in mid-air over the trench would simply fall into the lava.
    //
    // The generator always lays out all three and PlayingState::configureBridgeAxes()
    // prunes them to the run's difficulty (EASY 1, NORMAL 2, HARD 3) — the same
    // rule and the same code path level_3.json goes through, so a procedural
    // Bowser is not an easier or harder fight than the authored one. Placing a
    // difficulty-dependent count here instead would have put the rule in two
    // places, and the generator does not know the tier the run was started on.
    //
    // Deliberately clear of Bowser's own spawn at (leftX + 4, bridgeRow - 2),
    // which covers columns leftX+4..+5: an axe standing inside him on frame one
    // is a landmark the player cannot see.
    const int midBridgeAxeX = trenchLast - 2;
    for (int axeX : {trenchFirst, midBridgeAxeX, rightPostX + 1}) {
        auto axe = EntityFactory::create(EntityType::BridgeAxe,
                                         sf::Vector2f(axeX * TS, postRow * TS));
        if (axe) entities.push_back(std::move(axe));
    }

    std::cout << "[MapGenerator] Boss arena at tiles " << leftX << "-"
              << (leftX + BOSS_ARENA_TILES - 1) << " (bridge " << trenchFirst << "-"
              << trenchLast << " over lava, axes at " << trenchFirst << ", "
              << midBridgeAxeX << ", " << (rightPostX + 1) << ")" << std::endl;
}

void MapGenerator::generateSubLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, MapTheme theme, MapDifficulty difficulty, const std::string& returnLevelPath, sf::Vector2f returnPosition, unsigned int seed) {
    int subWidth = 65;
    int subHeight = 20;
    tileMap.initialize(subWidth, subHeight);
    entities.clear();

    if (seed == 0) seed = 424242;
    std::mt19937 rng(seed);

    // Always Ground, never Brick: this is the level's floor/ceiling boundary,
    // and the P-Switch swaps every Brick tile in the level to a Coin (see
    // PlayingState::beginPSwitch). Theming it as Brick for every non-Castle
    // sub-level used to mean pressing the switch turned the entire floor and
    // ceiling into coins, stranding the player with no ground to stand on
    // (D21) — Ground renders with the same themed sprites (PlayingState::
    // render) but is immune to the swap.
    TileType groundTile = TileType::Ground;

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

    // Entrance Pipe (non-functional return point visual). Two columns wide,
    // matching every other pipe in the game (the head/body sprite pair needs
    // both a left and a right column to compose a full pipe) — one column
    // used to render as a half pipe, using only the head_left/body_left
    // quarter sprites (D22/D23).
    tileMap.setTile(3, floorY - 1, TileType::Pipe);
    tileMap.setTile(3, floorY - 2, TileType::Pipe);
    tileMap.setTile(4, floorY - 1, TileType::Pipe);
    tileMap.setTile(4, floorY - 2, TileType::Pipe);

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
    // floorY - 3, not - 2: a Pipe is Pipe::HEIGHT_PX (4 tiles) tall and placed
    // by its top-left corner, so this seats it one row into the floor and puts
    // its rim three tiles up — see the checkpoint pipe in generate() for why
    // three and not four. This pipe is the ONLY way out of the vault, so a rim
    // at the exact top of the jump arc would be a soft lock.
    sf::Vector2f exitPipePos((subWidth - 6) * Constants::TILE_SIZE, (floorY - 3) * Constants::TILE_SIZE);
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
        // Three tiles short of the right edge, not `width - 15`.
        //
        // `width - 15` is exitX — the column the flagpole stands in — and the
        // BFS returns as soon as it reaches its goal, so everything from there
        // on was never examined at all: the victory staircase, the flagpole
        // approach, the castle apron, and (since R21) the boss arena, its lava
        // trench and its bridge. The whole point of the check is that the
        // player can reach the END, and the end is the right-hand edge.
        const int endTileX = std::max(startTileX + 1, attemptConfig.width - 3);
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
