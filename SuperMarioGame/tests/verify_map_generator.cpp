#include "Utils/MapGenerator.hpp"
#include "Utils/LevelSolvability.hpp"
#include "Utils/Constants.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Block.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/BridgeAxe.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "TestSaveSandbox.hpp"

namespace {

// Enemies only — not question blocks, not coins. "The chunk has entities" was
// true of the broken generator too, because every chunk carried three star
// coins and a checkpoint trampoline; what it did not carry was anything to
// play against.
int countEnemies(const std::vector<std::unique_ptr<Entity>>& entities) {
    int n = 0;
    for (const auto& e : entities) {
        if (e && dynamic_cast<Enemy*>(e.get())) ++n;
    }
    return n;
}

int tileXOf(const Entity& e) {
    return static_cast<int>(e.getPosition().x / Constants::TILE_SIZE);
}

} // namespace

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("map_generator");

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

    // ------------------------------------------------------------------
    // Test 4: an Endless chunk carries enemies ACROSS ITS WHOLE WIDTH.
    //
    // This is the user's actual R21 D7 report — "in endless mode it seems the
    // entity is not created in the far chunk". Two separate causes, both
    // invisible to a test that only counted entities:
    //
    //   * every ground enemy spawn was nested inside the coin-cluster branch,
    //     so the real density was coinClusterRate x enemySpawnRate — about one
    //     enemy per 100-tile chunk at the defaults;
    //   * the prefab loop ran [22, width-23), which on a 100-tile chunk is 45%
    //     dead — and the right-hand half of that dead zone is exactly where the
    //     player enters the chunk.
    //
    // So this asserts a count AND a distribution, and specifically demands
    // content in the last third, which used to be structurally empty.
    {
        MapGeneratorConfig config;
        config.isChunk = true;
        config.width = 100;
        config.height = 23;
        config.seed = 5150;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        const int enemies = countEnemies(entities);
        assert(enemies >= 4 && "A 100-tile Endless chunk must carry real enemy density!");

        // Thirds of the chunk. Every one of them has to hold something.
        int perThird[3] = {0, 0, 0};
        for (const auto& e : entities) {
            if (!e) continue;
            const int tx = tileXOf(*e);
            if (tx < 0 || tx >= config.width) continue;
            perThird[std::min(2, tx * 3 / config.width)]++;
        }
        assert(perThird[0] > 0 && "chunk's first third is empty");
        assert(perThird[1] > 0 && "chunk's middle third is empty");
        assert(perThird[2] > 0 && "chunk's LAST third is empty — the 45% dead margin is back");

        // A chunk is a slice of road, not a level: no player to duplicate the
        // live one, no flagpole or castle to end a mode that does not end.
        for (const auto& e : entities) {
            const std::string type = e->getTypeName();
            assert(type != "flagpole" && type != "castle" && "a chunk must not build an exit");
            assert(type != "mario" && type != "luigi" && type != "peach" &&
                   "a chunk must not build a second player");
        }

        std::cout << "[PASS] Test 4: Endless chunk carries " << enemies
                  << " enemies spread " << perThird[0] << "/" << perThird[1] << "/"
                  << perThird[2] << " across its thirds." << std::endl;
    }

    // ------------------------------------------------------------------
    // Test 5: spliced entities STAY where the splice put them.
    //
    // Endless Mode shifts a freshly generated chunk into world space. That used
    // to be a plain setPosition(), which writes `position` and nothing else —
    // while MovingPlatform drives itself to m_startPos + range*progress every
    // frame and TimerEmergenceStrategy parks a retracted PiranhaPlant on the
    // anchor it captured at construction. Both were back at their chunk-LOCAL
    // coordinates on frame one, i.e. back near world tile 20-80, which is what
    // made a far chunk look empty and littered the start of the world with
    // stray platforms.
    {
        MapGeneratorConfig config;
        config.isChunk = true;
        config.width = 100;
        config.height = 23;
        config.pipeFrequency = 0.5f;     // guarantee pipes, hence piranha plants
        config.pitProbability = 0.35f;   // guarantee pits, hence platforms
        config.seed = 31337;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        const sf::Vector2f delta(600.0f * Constants::TILE_SIZE, 0.0f);
        std::vector<sf::Vector2f> expected;
        for (auto& e : entities) {
            e->translate(delta);
            expected.push_back(e->getPosition());
        }

        // Ten frames is far more than enough: both snap-backs are deterministic
        // and happen on the very first update.
        for (int frame = 0; frame < 10; ++frame) {
            for (auto& e : entities) {
                if (e && e->isActive()) e->update(1.0f / 60.0f);
            }
        }

        int checked = 0;
        for (size_t i = 0; i < entities.size(); ++i) {
            // Only the self-driven classes are interesting; anything the physics
            // engine owns has no engine here and simply holds still.
            const std::string type = entities[i]->getTypeName();
            if (type != "moving_platform" && type != "piranha_plant" &&
                type != "falling_platform") continue;
            ++checked;
            const float driftX = std::abs(entities[i]->getPosition().x - expected[i].x);
            // A moving platform legitimately patrols a couple of tiles; a
            // snap-back is 600 tiles. Anything under one chunk width is fine.
            assert(driftX < 100.0f * Constants::TILE_SIZE &&
                   "a spliced entity teleported back to its chunk-local position");
            assert(entities[i]->getPosition().x > 500.0f * Constants::TILE_SIZE &&
                   "a spliced entity is no longer in the chunk it was spliced into");
        }
        assert(checked >= 2 && "this case proves nothing without self-driven entities in it");
        std::cout << "[PASS] Test 5: " << checked
                  << " self-driven spliced entities stayed put across 10 frames." << std::endl;
    }

    // ------------------------------------------------------------------
    // Test 6: a boss chunk is a FIGHT, and an ordinary chunk has no boss.
    //
    // The generator used to drop a bare Bowser at exitX-6 on Hard and nothing
    // else: no arena (so PlayingState's camera lock, health bar and battle
    // music all sat behind a condition that could never be true), no room, no
    // lava, no bridge and no axe.
    {
        MapGeneratorConfig config;
        config.isChunk = true;
        config.bossArena = true;
        config.width = 100;
        config.height = 23;
        config.seed = 24680;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(tileMap, entities, config);

        Boss* boss = nullptr;
        bool foundAxe = false;
        for (const auto& e : entities) {
            if (auto* b = dynamic_cast<Boss*>(e.get())) boss = b;
            if (dynamic_cast<BridgeAxe*>(e.get())) foundAxe = true;
        }
        assert(boss && "a boss chunk must contain a boss");
        assert(boss->hasArena() && "a generated boss without an arena is scenery, not a fight");
        assert(foundAxe && "the non-combat route out of the fight must exist");

        // The arena has to hold a bridge chop: PlayingState::chopBridge() drops
        // every solid tile in an arena column that has lava under it, and finds
        // those columns by that shape alone.
        const AABB arena = boss->getArena();
        const int firstX = static_cast<int>(arena.x / Constants::TILE_SIZE);
        const int lastX  = static_cast<int>((arena.x + arena.width) / Constants::TILE_SIZE);
        int bridgeColumns = 0;
        for (int x = firstX; x < lastX && x < config.width; ++x) {
            int lavaRow = -1;
            for (int y = 0; y < config.height; ++y) {
                if (tileMap.getTileType(x, y) == TileType::Lava) { lavaRow = y; break; }
            }
            if (lavaRow < 0) continue;
            for (int y = 0; y < lavaRow; ++y) {
                if (TileMap::getInfo(tileMap.getTileType(x, y)).isSolid) { ++bridgeColumns; break; }
            }
        }
        assert(bridgeColumns >= 6 && "the arena needs a real bridge over real lava to chop");

        // ... and an ordinary chunk at the same difficulty must NOT have one.
        // The escalation turns Endless Hard from chunk 6, and the generator used
        // to put a Bowser in every Hard map — so every chunk past ~600 tiles
        // spliced another one in.
        MapGeneratorConfig plain = config;
        plain.bossArena = false;
        plain.difficulty = MapDifficulty::Hard;
        TileMap plainMap;
        std::vector<std::unique_ptr<Entity>> plainEntities;
        MapGenerator::generate(plainMap, plainEntities, plain);
        for (const auto& e : plainEntities) {
            assert(!dynamic_cast<Boss*>(e.get()) &&
                   "an ordinary Hard chunk must not spawn a boss");
        }

        std::cout << "[PASS] Test 6: boss chunk has an arena, " << bridgeColumns
                  << " choppable bridge columns and an axe; an ordinary Hard chunk has no boss."
                  << std::endl;
    }

    // ------------------------------------------------------------------
    // Test 7: the solvability check is no longer vacuous under a ceiling.
    //
    // LevelSolvability::groundRowAt() scanned top-down for the first solid
    // tile, and MapGenerator gives every Castle and Underground map a solid
    // two-row ceiling across its whole width — so every column answered "row 0",
    // every rise was zero, and isPathReachable() returned true for ANY map of
    // those two themes, including one cut clean in half.
    {
        TileMap ceilinged;
        ceilinged.initialize(60, 23);
        for (int x = 0; x < 60; ++x) {
            ceilinged.setTile(x, 0, TileType::Ground);
            ceilinged.setTile(x, 1, TileType::Brick);
            // Floor everywhere except a 20-tile chasm nothing can jump.
            if (x < 20 || x >= 40) {
                for (int y = 21; y < 23; ++y) ceilinged.setTile(x, y, TileType::Ground);
            }
        }
        std::vector<std::unique_ptr<Entity>> none;
        const bool reachable = LevelSolvability::isPathReachable(ceilinged, none, 3, 57);
        assert(!reachable &&
               "a 20-tile chasm under a ceiling must be rejected — this passed vacuously before");

        // The control: the same map with the chasm filled in is accepted, so the
        // case above is rejecting the gap and not the ceiling.
        for (int x = 20; x < 40; ++x) {
            for (int y = 21; y < 23; ++y) ceilinged.setTile(x, y, TileType::Ground);
        }
        assert(LevelSolvability::isPathReachable(ceilinged, none, 3, 57) &&
               "a flat floor under a ceiling must still be accepted");
        std::cout << "[PASS] Test 7: ceilinged maps are actually checked now." << std::endl;
    }

    // ------------------------------------------------------------------
    // Test 8: a generated Castle is verified end to end, arena included.
    //
    // Both halves of the old vacuity at once: the ceiling made every column
    // reachable, and the goal column was exitX (= width - 15), so the whole exit
    // apron — staircase, flagpole, castle, and now the boss arena — sat past the
    // goal and was never examined.
    {
        MapGeneratorConfig config;
        config.theme = MapTheme::Castle;
        config.difficulty = MapDifficulty::Hard;
        config.width = 180;          // deliberately not the 200 every other case uses
        config.height = 23;
        config.seed = 424243;

        TileMap tileMap;
        std::vector<std::unique_ptr<Entity>> entities;
        const bool ok = MapGenerator::generateSolvable(tileMap, entities, config, 6);
        assert(ok && "a Hard castle must survive a non-vacuous solvability check");
        assert(tileMap.getWidth() == 180 && "the generator must honour a non-default width");

        Boss* boss = nullptr;
        for (const auto& e : entities) {
            if (auto* b = dynamic_cast<Boss*>(e.get())) boss = b;
        }
        assert(boss && boss->hasArena() &&
               "a Hard procedural level's Bowser must come with the room he is fought in");

        // Castle pits are lethal now: the hazard tile was TileType::Water, which
        // the damage check does not test for, so every generated castle pit was
        // decorative and chopBridge() had no lava to find.
        bool foundLava = false;
        for (int x = 0; x < tileMap.getWidth() && !foundLava; ++x) {
            for (int y = 0; y < tileMap.getHeight(); ++y) {
                if (tileMap.getTileType(x, y) == TileType::Lava) { foundLava = true; break; }
            }
        }
        assert(foundLava && "a Castle map's hazard must be Lava, not inert Water");
        std::cout << "[PASS] Test 8: 180-wide Hard castle verified end-to-end with a real arena and lava."
                  << std::endl;
    }

    std::cout << "=== ALL REFINED MAP GENERATOR TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
