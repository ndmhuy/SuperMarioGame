// verify_r21_entity_guards.cpp — R21 defects 5, 8 and 10.
//
// All three are the same shape: an entity moving without ever asking what it is
// moving into, or a limit counted against the wrong thing.
//
//   D5   moving platforms drove into walls (no tilemap query in the class at
//        all) and, because PhysicsEngine also integrated them, carried nobody.
//   D8   Lakitu's three-Spiny limit was a LIFETIME cap, so a Lakitu that threw
//        its eggs off-camera before the player arrived never threw again.
//   D10  nothing but PatrolStrategy had a ledge check, and Bowser had no arena
//        clamp — so a phase-2 leap put him outside the room while the player
//        stayed locked inside it, which is a softlock, not a stray sprite.
//
// Window-free and cheap, so CI can run it.
//
// Run via:  ctest -R r21_entity_guards --output-on-failure

#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Mario.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/Spiny.hpp"
#include "Core/GameSnapshot.hpp"
#include "Physics/AABB.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"
#include "TestSaveSandbox.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    if (condition) {
        std::cout << "  [ ok ] " << what << "\n";
    } else {
        std::cout << "  [FAIL] " << what << "\n";
        ++g_failures;
    }
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

const float DT = 1.0f / 60.0f;

// Deliberately re-derived here rather than reused from TerrainProbe: this is the
// property under test, and a test that asks the code under test whether it is
// right proves nothing.
bool solidAt(const TileMap& map, float x, float y) {
    return TileMap::getInfo(map.getTileAt(x, y)).isSolid;
}

bool boxOverlapsSolid(const TileMap& map, const AABB& box) {
    for (float y = box.y; y < box.y + box.height; y += 4.0f) {
        for (float x = box.x; x < box.x + box.width; x += 4.0f) {
            if (solidAt(map, x, y)) return true;
        }
    }
    return false;
}

// PlayingState updates entities and then runs physics, in that order. Anything
// testing motion has to use the same order: reversing it is what hid D5's dead
// player-carry behind a passing standalone harness for months.
void stepWorld(PhysicsEngine& physics,
               std::vector<std::unique_ptr<Entity>>& entities,
               TileMap& map) {
    for (auto& e : entities) {
        if (e && e->isActive()) e->update(DT);
    }
    physics.update(entities, map, DT);
}

// ---------------------------------------------------------------------------
// D5 — a moving platform reverses at terrain instead of driving into it.
// ---------------------------------------------------------------------------
void testMovingPlatformStopsAtAWall() {
    section("D5  a moving platform whose sweep runs into a wall reverses");

    TileMap map;
    map.initialize(40, 20);
    for (int x = 0; x < 40; ++x) map.setTile(x, 18, TileType::Ground);
    // A single block standing in the second half of the platform's sweep.
    // World x 384..416, y 480..512 — the row the platform travels along.
    map.setTile(12, 15, TileType::Brick);
    Game::getInstance().setTileMap(&map);

    // Start at x=256 (right edge 320) with EntityFactory's stock 4-tile
    // rightward sweep, which would end at x=384 — inside the block.
    MovingPlatform platform({256.0f, 480.0f},
                            {4.0f * Constants::TILE_SIZE, 0.0f}, 50.0f);

    float minX = platform.getPosition().x;
    float maxX = platform.getPosition().x;
    bool everInsideTerrain = false;

    for (int frame = 0; frame < 1200; ++frame) {
        platform.update(DT);
        const float x = platform.getPosition().x;
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        if (boxOverlapsSolid(map, platform.getBoundingBox())) everInsideTerrain = true;
    }

    check(!everInsideTerrain, "it never occupies a solid tile");
    check(maxX < 384.0f,
          "it stops short of the configured travel end, which is inside the block (reached x=" +
          std::to_string(maxX) + ")");
    check(maxX > 300.0f,
          "but it does travel the part of the sweep that is clear (reached x=" +
          std::to_string(maxX) + ")");
    check(std::abs(minX - 256.0f) < 1.0f,
          "and it patrols back to its start rather than stalling against the wall");

    // A platform placed inside terrain has nowhere to go and must hold still
    // rather than flip every frame.
    MovingPlatform buried({384.0f, 480.0f},
                          {4.0f * Constants::TILE_SIZE, 0.0f}, 50.0f);
    for (int frame = 0; frame < 120; ++frame) buried.update(DT);
    check(std::abs(buried.getPosition().x - 384.0f) < 0.001f,
          "a platform whose start is already inside terrain does not oscillate in place");

    Game::getInstance().setTileMap(nullptr);
}

// ---------------------------------------------------------------------------
// D5 — and it actually carries the player, through the real engine.
//
// verify_blocks_new.cpp already asserts the carry, and passed throughout: it
// runs the platform standalone. With PhysicsEngine in the loop the platform was
// integrated twice and update()'s own displacement measured zero, so nobody was
// ever carried in the real game.
// ---------------------------------------------------------------------------
void testMovingPlatformCarriesThePlayer() {
    section("D5  a moving platform carries the player, with the engine running");

    TileMap map;
    map.initialize(40, 20);
    for (int x = 0; x < 40; ++x) map.setTile(x, 18, TileType::Ground);
    Game::getInstance().setTileMap(&map);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<MovingPlatform>(
        sf::Vector2f{256.0f, 400.0f}, sf::Vector2f{4.0f * Constants::TILE_SIZE, 0.0f}, 50.0f));
    entities.push_back(std::make_unique<Mario>(sf::Vector2f{0.0f, 0.0f}));

    auto* platform = static_cast<MovingPlatform*>(entities[0].get());
    auto* mario    = static_cast<Mario*>(entities[1].get());

    // Stand him on the platform's top edge, roughly centred.
    mario->setPosition({280.0f, 400.0f - mario->getBoundingBox().height});
    mario->setVelocity({0.0f, 0.0f});
    Game::getInstance().setPlayer(mario);

    const float platformStartX = platform->getPosition().x;
    const float playerStartX   = mario->getPosition().x;

    PhysicsEngine physics;
    for (int frame = 0; frame < 60; ++frame) stepWorld(physics, entities, map);

    const float platformMoved = platform->getPosition().x - platformStartX;
    const float playerMoved   = mario->getPosition().x - playerStartX;

    check(platformMoved > 20.0f,
          "the platform itself moved (" + std::to_string(platformMoved) + "px in one second)");
    check(playerMoved > 0.5f * platformMoved,
          "and the player riding it moved with it (" + std::to_string(playerMoved) +
          "px), rather than the zero displacement the double-integration produced");

    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
}

// ---------------------------------------------------------------------------
// D8 — Lakitu's drop limit is concurrent and its clock is gated on engagement.
// ---------------------------------------------------------------------------
void testLakituKeepsItsDropsForTheEncounter() {
    section("D8  Lakitu holds its eggs until the player is there, and refills them");

    // Stand in for PlayingState's spawn handler: the drop leaves as an event and
    // the Spiny it asks for is what the concurrent cap counts.
    std::vector<std::unique_ptr<Spiny>> spinies;
    int spawnRequests = 0;
    EventBus::ScopedSubscription spawner(
        EventType::EntitySpawnRequested,
        [&spawnRequests, &spinies](const GameEvent& ev) {
            if (!ev.data.has_value() || ev.data.type() != typeid(EntitySpawnRequest)) return;
            const auto request = std::any_cast<EntitySpawnRequest>(ev.data);
            if (request.type != static_cast<int>(EntityType::Spiny)) return;
            ++spawnRequests;
            spinies.push_back(std::make_unique<Spiny>(request.position));
        });

    Mario mario({0.0f, 500.0f});
    Game::getInstance().setPlayer(&mario);

    Lakitu lakitu({5000.0f, 200.0f});
    check(!lakitu.isEngaged(), "a Lakitu 5000px away is not engaged");

    // A minute of the player being elsewhere — the whole approach to the Lakitu
    // in level_1, which sits ~5500px from the player's spawn.
    for (int frame = 0; frame < 60 * 60; ++frame) lakitu.update(DT);
    check(spawnRequests == 0,
          "it throws nothing while the player is far away (threw " +
          std::to_string(spawnRequests) + ")");
    check(lakitu.getSpawnCount() == 0, "so its allowance is still intact when the player arrives");

    // The player walks into the encounter.
    mario.setPosition({5000.0f, 500.0f});
    check(lakitu.isEngaged(), "and is engaged once the player is on screen");

    for (int frame = 0; frame < 60 * 30 && spawnRequests < Lakitu::MAX_SPINIES; ++frame) {
        lakitu.update(DT);
    }
    check(spawnRequests == Lakitu::MAX_SPINIES,
          "it then drops its full complement (" + std::to_string(spawnRequests) + ")");

    // Cap reached: no more, however long the fight runs.
    for (int frame = 0; frame < 60 * 60; ++frame) lakitu.update(DT);
    check(spawnRequests == Lakitu::MAX_SPINIES,
          "and stops there while they are all still alive (" +
          std::to_string(spawnRequests) + "), so it is not a fountain");
    check(Spiny::liveCount() == Lakitu::MAX_SPINIES,
          "the census agrees on how many are in play");

    // The player clears the field. This is the half the lifetime cap could never
    // do: with it, the encounter was over for good.
    spinies.clear();
    check(Spiny::liveCount() == 0, "clearing them empties the census");

    for (int frame = 0; frame < 60 * 30 && spawnRequests == Lakitu::MAX_SPINIES; ++frame) {
        lakitu.update(DT);
    }
    check(spawnRequests > Lakitu::MAX_SPINIES,
          "and Lakitu can throw again once its Spinies are dead (" +
          std::to_string(spawnRequests) + " total)");

    spinies.clear();
    Game::getInstance().setPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// D10 — Bowser stays in his arena across a phase-2 leap.
// ---------------------------------------------------------------------------
void testBowserCannotLeapOutOfHisArena() {
    section("D10  Bowser stays inside his arena, leap or no leap");

    TileMap map;
    map.initialize(60, 20);
    for (int x = 0; x < 60; ++x) map.setTile(x, 18, TileType::Ground);
    Game::getInstance().setTileMap(&map);

    const AABB arena{320.0f, 0.0f, 640.0f, 608.0f};
    // The same derivation Bowser::updateBehaviour uses. Written out rather than
    // exposed through a getter, so the bound this test enforces is stated here
    // in the test and not taken on the class's word.
    const float patrolLeft  = arena.x + Constants::TILE_SIZE;
    const float patrolRight = arena.x + arena.width - Constants::TILE_SIZE - 64.0f;

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Bowser>(sf::Vector2f{600.0f, 512.0f}));
    auto* bowser = static_cast<Bowser*>(entities[0].get());
    bowser->setArena(arena);

    // Parked well outside the arena, to the right: this is what used to drag him
    // through the wall, because the player-facing override ran after the
    // patrol-bound turnaround and overwrote it.
    Mario mario({1600.0f, 512.0f});
    Game::getInstance().setPlayer(&mario);

    // Three stomps take him to phase 2, where he leaps every 2.6s.
    for (int hit = 0; hit < 3; ++hit) {
        bowser->onStomped();
        for (int i = 0; i < 70; ++i) bowser->update(DT);
    }
    check(bowser->getPhase() == 2, "he is in phase 2, where the leap happens");

    PhysicsEngine physics;
    float minX = bowser->getPosition().x;
    float maxX = bowser->getPosition().x;
    bool leapt = false;
    const float groundY = bowser->getPosition().y;

    for (int frame = 0; frame < 900; ++frame) {
        stepWorld(physics, entities, map);
        const float x = bowser->getPosition().x;
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        if (bowser->getPosition().y < groundY - 30.0f) leapt = true;
    }

    // The clamp lives in updateBehaviour, which runs BEFORE physics integrates
    // the frame's velocity — so one frame's walk can sit outside the bound
    // before being pulled back, exactly as it does for BoomBoom, which has
    // clamped this way since it was written. What must never happen, and is
    // what the softlock was, is him leaving and STAYING out.
    const float ONE_FRAME_OF_WALK = Constants::BOSS_BOWSER_SPEED * 1.6f * DT + 0.01f;

    check(leapt, "he does leap (otherwise this test proves nothing)");
    check(minX >= patrolLeft - ONE_FRAME_OF_WALK,
          "and never crosses the left arena bound (min x=" + std::to_string(minX) +
          ", bound " + std::to_string(patrolLeft) + ")");
    check(maxX <= patrolRight + ONE_FRAME_OF_WALK,
          "nor the right one, with the player parked outside it (max x=" +
          std::to_string(maxX) + ", bound " + std::to_string(patrolRight) + ")");
    check(bowser->getPosition().x >= patrolLeft && bowser->getPosition().x <= patrolRight,
          "and he is inside the arena when the dust settles (x=" +
          std::to_string(bowser->getPosition().x) + ")");

    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
}

// ---------------------------------------------------------------------------
// D10 — a Hammer Bro on a ledge stays on it.
// ---------------------------------------------------------------------------
void testHammerBroStaysOnItsLedge() {
    section("D10  a Hammer Bro on a platform does not shuffle off the edge");

    TileMap map;
    map.initialize(40, 20);
    // Floor only from x=0..9: world x 0..320, with void either side of it.
    for (int x = 0; x <= 9; ++x) map.setTile(x, 18, TileType::Ground);
    Game::getInstance().setTileMap(&map);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<HammerBro>(sf::Vector2f{256.0f, 528.0f}));
    auto* bro = static_cast<HammerBro*>(entities[0].get());

    Mario mario({160.0f, 528.0f});
    Game::getInstance().setPlayer(&mario);

    PhysicsEngine physics;
    // His centre, not his bounding box: an enemy standing at the lip of a
    // platform legitimately overhangs it, and his hop carries him a further
    // ~12px (0.39s of airtime at the 30px/s shuffle) past wherever the ledge
    // check last stopped him. Falling is the defect; overhanging is not.
    float maxCentre = bro->getPosition().x + bro->getBoundingBox().width / 2.0f;
    float minCentre = maxCentre;
    float maxY = bro->getPosition().y;

    // 30 seconds is ten of his 3s hops, so the airborne case is exercised too.
    for (int frame = 0; frame < 1800; ++frame) {
        stepWorld(physics, entities, map);
        const float centre = bro->getPosition().x + bro->getBoundingBox().width / 2.0f;
        maxCentre = std::max(maxCentre, centre);
        minCentre = std::min(minCentre, centre);
        maxY = std::max(maxY, bro->getPosition().y);
    }

    check(bro->isActive(), "he is still in the world after thirty seconds");
    check(maxCentre <= 320.0f,
          "he never shuffles past the right edge of his platform (centre reached " +
          std::to_string(maxCentre) + ", edge 320)");
    check(minCentre >= 0.0f,
          "nor past the left one (centre reached " + std::to_string(minCentre) + ", edge 0)");
    check(maxY <= 528.0f + 1.0f,
          "and he never starts falling past it (lowest y=" + std::to_string(maxY) + ")");

    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
}

}  // namespace

int main() {
    // Every save path in this process points at a throwaway directory, so
    // nothing here can read or delete real save data. See TestSaveSandbox.hpp.
    TestSaveSandbox sandbox("r21_entity_guards");

    testMovingPlatformStopsAtAWall();
    testMovingPlatformCarriesThePlayer();
    testLakituKeepsItsDropsForTheEncounter();
    testBowserCannotLeapOutOfHisArena();
    testHammerBroStaysOnItsLedge();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
