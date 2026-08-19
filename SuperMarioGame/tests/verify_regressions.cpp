// verify_regressions.cpp — headless regression suite for the August 2026 audit.
//
// Every case here corresponds to a defect that shipped, was marked complete in
// TASK_DIVISION.md, and survived because nothing ran automatically. Each one is
// deliberately cheap and window-free so CI can run it.
//
// Run via:  ctest -R regressions --output-on-failure
//
// Add a case here whenever a bug escapes to the audit stage again.

#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Utils/Constants.hpp"
#include "Entities/Mario.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Goomba.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Graphics/Camera.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>

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

// Levels live relative to the source tree; the test binary may run from build/.
std::string levelPath(const std::string& name) {
    const std::vector<std::string> roots = {
        "assets/levels/", "../assets/levels/", "../../assets/levels/",
        "SuperMarioGame/assets/levels/"
    };
    for (const auto& r : roots) {
        if (std::filesystem::exists(r + name)) return r + name;
    }
    return "assets/levels/" + name;
}

int countTiles(const TileMap& map, TileType type) {
    int n = 0;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getTileType(x, y) == type) ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// A-1 — the loader dropped every "coin_tile" because it re-implemented the
// string->TileType mapping instead of using SerializationUtils.
// ---------------------------------------------------------------------------
void testCoinTilesLoad() {
    section("A-1  coin tiles survive loading");

    const std::vector<std::string> levels = {
        "level_1.json", "level_2.json", "level_3.json", "bonus_1.json",
        "level_1_sub.json", "level_2_sub.json", "level_3_sub.json"
    };

    for (const auto& name : levels) {
        const std::string path = levelPath(name);
        if (!std::filesystem::exists(path)) {
            check(false, name + " exists on disk");
            continue;
        }
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(path, map, data)) {
            check(false, name + " loads");
            continue;
        }
        const int coins = countTiles(map, TileType::Coin);
        check(coins > 0, name + " has coin tiles after load (found " + std::to_string(coins) + ")");
    }
}

// The round-trip is the actual invariant: whatever getTileTypeName emits,
// parseTileTypeName must accept. A drift here is what caused A-1.
void testTileNameRoundTrip() {
    section("A-1  tile name round-trip is total");

    const TileType all[] = {
        TileType::Empty, TileType::Ground, TileType::Brick, TileType::Question,
        TileType::Pipe,  TileType::Ice,    TileType::Conveyor,
        TileType::Water, TileType::Coin,   TileType::Used
    };
    for (TileType t : all) {
        const std::string name = SerializationUtils::getTileTypeName(t);
        const TileType back = SerializationUtils::parseTileTypeName(name);
        check(back == t, "round-trip \"" + name + "\" -> " +
                         std::to_string(static_cast<int>(t)));
    }

    // Legacy aliases must keep loading older hand-written level JSON.
    check(SerializationUtils::parseTileTypeName("coin")     == TileType::Coin,     "alias \"coin\"");
    check(SerializationUtils::parseTileTypeName("question") == TileType::Question, "alias \"question\"");
    check(SerializationUtils::parseTileTypeName("nonsense") == TileType::Empty,    "unknown -> Empty");
}

// ---------------------------------------------------------------------------
// A-7 — Star/Mega decorators called player.changeState() from inside their own
// update(), freeing themselves mid-call. Run under ASan this crashed.
// ---------------------------------------------------------------------------
void testStarDecoratorExpiry() {
    section("A-7  Star decorator expires without destroying itself mid-update");

    Mario mario({100.0f, 100.0f});
    check(dynamic_cast<SmallState*>(mario.getCurrentState()) != nullptr,
          "starts in SmallState");

    mario.powerUp(4); // Star
    check(dynamic_cast<StarDecorator*>(mario.getCurrentState()) != nullptr,
          "Star wraps the base state");

    // Step past STAR_DURATION at the fixed timestep.
    const float dt = Constants::FIXED_TIMESTEP;
    const int steps = static_cast<int>(Constants::STAR_DURATION / dt) + 10;
    for (int i = 0; i < steps; ++i) {
        mario.update(dt);
    }

    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "decorator is gone after the timer lapses");
    check(dynamic_cast<SmallState*>(mario.getCurrentState()) != nullptr,
          "the wrapped SmallState survived the unwrap");
}

void testMegaDecoratorExpiry() {
    section("A-7  Mega decorator expires cleanly too");

    Mario mario({100.0f, 100.0f});
    mario.powerUp(0); // Super
    check(dynamic_cast<SuperState*>(mario.getCurrentState()) != nullptr, "reached SuperState");

    mario.powerUp(5); // Mega
    check(dynamic_cast<MegaDecorator*>(mario.getCurrentState()) != nullptr, "Mega wraps SuperState");

    const float dt = Constants::FIXED_TIMESTEP;
    for (int i = 0; i < static_cast<int>(12.0f / dt); ++i) {
        mario.update(dt);
    }

    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "Mega decorator retired");
    check(dynamic_cast<SuperState*>(mario.getCurrentState()) != nullptr,
          "SuperState survived");
}

// ---------------------------------------------------------------------------
// A-3 — every path that swaps the player must keep the observers consistent.
// PlayingState::adoptPlayer needs a window, so the headless half of the
// invariant is checked here: carrying stats across a swap must not fire events.
// ---------------------------------------------------------------------------
void testRestoreStatsIsSilent() {
    section("A-3  restoreStats carries stats without side effects");

    Mario mario({0.0f, 0.0f});
    mario.restoreStats(5, 250, 12345);

    check(mario.getLives() == 5,     "lives restored exactly (no gainLife loop)");
    check(mario.getCoins() == 250,   "coins restored exactly, not wrapped by the 100-coin 1-UP rule");
    check(mario.getScore() == 12345, "score restored exactly");

    // addCoins(250) would have granted two extra lives and left coins at 50.
    check(mario.getLives() != 7, "restoreStats did not trigger the 1-UP rule");
}

// ---------------------------------------------------------------------------
// A-5 — rewind snapshots were paired to entities by vector index. Pruning an
// entity between record and restore shifted every later index by one, so
// rewinding silently teleported entities into each other's positions.
// ---------------------------------------------------------------------------
void testRewindRestoresByIdNotIndex() {
    section("A-5  rewind survives an entity being pruned mid-sequence");

    // Three entities with known, distinct positions.
    std::vector<std::unique_ptr<Mario>> world;
    world.push_back(std::make_unique<Mario>(sf::Vector2f{100.0f, 0.0f}));
    world.push_back(std::make_unique<Mario>(sf::Vector2f{200.0f, 0.0f}));
    world.push_back(std::make_unique<Mario>(sf::Vector2f{300.0f, 0.0f}));

    check(world[0]->getId() != world[1]->getId() &&
          world[1]->getId() != world[2]->getId(),
          "entity ids are unique");

    // Record.
    GameSnapshot snap;
    for (const auto& e : world) {
        snap.entityStates.push_back({e->getId(), e->getPosition(), e->getVelocity(), e->isActive()});
    }

    // The middle entity is pruned and everything moves — exactly what happens
    // between two frames of real gameplay.
    world.erase(world.begin() + 1);
    for (auto& e : world) e->setPosition({-999.0f, -999.0f});

    // Restore by id.
    std::unordered_map<std::uint32_t, const EntitySnapshot*> byId;
    for (const auto& es : snap.entityStates) byId.emplace(es.id, &es);
    for (const auto& e : world) {
        auto it = byId.find(e->getId());
        if (it != byId.end()) e->setPosition(it->second->position);
    }

    // Index-based restore would have given the survivor at index 1 the snapshot
    // of the deleted entity — 200 instead of 300.
    check(world[0]->getPosition().x == 100.0f, "first entity restored to 100");
    check(world[1]->getPosition().x == 300.0f,
          "surviving third entity restored to 300, not 200 (index drift would give 200)");
}

// ---------------------------------------------------------------------------
// A-4 — coyote time and jump buffering were counted but never read by jump().
// ---------------------------------------------------------------------------
void testCoyoteTime() {
    section("A-4  coyote time lets a jump land just after leaving a ledge");

    Mario mario({0.0f, 0.0f});
    mario.setGrounded(true);
    mario.update(Constants::FIXED_TIMESTEP);          // charges the grace window
    check(mario.getCoyoteFramesLeft() > 0, "grace window charged while grounded");

    mario.setGrounded(false);                          // walked off the ledge
    mario.update(Constants::FIXED_TIMESTEP);
    check(mario.getCoyoteFramesLeft() > 0, "grace window still open one frame later");

    mario.setVelocity({0.0f, 0.0f});
    mario.jump();
    check(mario.getVelocity().y < 0.0f, "jump fires during the coyote window");
}

void testCoyoteWindowExpires() {
    section("A-4  coyote window does expire");

    Mario mario({0.0f, 0.0f});
    mario.setGrounded(true);
    mario.update(Constants::FIXED_TIMESTEP);
    mario.setGrounded(false);
    for (int i = 0; i < Constants::COYOTE_FRAMES + 4; ++i) {
        mario.update(Constants::FIXED_TIMESTEP);
    }
    check(mario.getCoyoteFramesLeft() == 0, "window closed after COYOTE_FRAMES");

    mario.setVelocity({0.0f, 0.0f});
    mario.jump();
    check(mario.getVelocity().y == 0.0f, "a late jump does not fire mid-air");
    check(mario.getJumpBufferFramesLeft() > 0, "it is buffered instead of dropped");
}

void testJumpBufferFiresOnLanding() {
    section("A-4  a jump pressed just before landing fires on touchdown");

    Mario mario({0.0f, 0.0f});
    mario.setGrounded(false);
    mario.jump();                                      // pressed in mid-air
    check(mario.getJumpBufferFramesLeft() > 0, "request buffered");

    mario.setVelocity({0.0f, 0.0f});
    mario.setGrounded(true);                           // touchdown
    mario.update(Constants::FIXED_TIMESTEP);

    check(mario.getVelocity().y < 0.0f, "buffered jump fired on landing");
    check(mario.getJumpBufferFramesLeft() == 0, "buffer consumed");
}

// ---------------------------------------------------------------------------
// A-8 — base-form power-ups called changeState(), discarding an active Star.
// ---------------------------------------------------------------------------
void testPowerUpPreservesStar() {
    section("A-8  collecting a Fire Flower does not cancel Star");

    Mario mario({0.0f, 0.0f});
    mario.powerUp(4); // Star
    check(dynamic_cast<StarDecorator*>(mario.getCurrentState()) != nullptr, "Star active");

    mario.powerUp(1); // Fire Flower
    check(dynamic_cast<StarDecorator*>(mario.getCurrentState()) != nullptr,
          "Star survived the power-up");
    check(dynamic_cast<FireState*>(mario.getBaseState()) != nullptr,
          "base form advanced to FireState underneath the decorator");
}

// ---------------------------------------------------------------------------
// A-10 — getEntityTypeName was a 30-deep dynamic_cast chain that tested
// KoopaTroopa before its own subclass KoopaParatroopa, so every Paratroopa was
// serialised as a plain koopa_troopa and reloaded without wings.
// ---------------------------------------------------------------------------
void testEntityTypeNameIsVirtual() {
    section("A-10  entity type names are not shadowed by base classes");

    Goomba goomba({0.0f, 0.0f});
    KoopaTroopa koopa({0.0f, 0.0f});
    KoopaParatroopa para({0.0f, 0.0f});

    check(SerializationUtils::getEntityTypeName(goomba) == "goomba", "Goomba -> \"goomba\"");
    check(SerializationUtils::getEntityTypeName(koopa) == "koopa_troopa", "KoopaTroopa -> \"koopa_troopa\"");
    check(SerializationUtils::getEntityTypeName(para) == "koopa_paratroopa",
          "KoopaParatroopa -> \"koopa_paratroopa\", not shadowed by its base");

    // The category is what the collision resolver dispatches on.
    check(goomba.getCategory() == EntityCategory::Enemy, "Goomba reports Enemy category");
    check(para.getCategory()   == EntityCategory::Enemy, "Paratroopa reports Enemy category");
}

// ---------------------------------------------------------------------------
// C-1/C-2 — the camera showed void outside the map. Four causes; these pin the
// two that are pure geometry.
// ---------------------------------------------------------------------------
void testCameraClampsToMap() {
    section("C-1  camera never shows outside a map taller than the view");

    Camera cam;
    const float VW = 1280.0f, VH = 720.0f;
    cam.setBounds(AABB{0.0f, 0.0f, 6400.0f, 736.0f});   // level_1 dimensions

    // Far off the left/top edge.
    sf::Vector2f c = cam.clampToBounds({-5000.0f, -5000.0f});
    check(c.x - VW/2 >= -0.01f, "left edge held: view.left >= 0");
    check(c.y - VH/2 >= -0.01f, "top edge held: view.top >= 0");

    // Far off the right/bottom edge.
    c = cam.clampToBounds({99999.0f, 99999.0f});
    check(c.x + VW/2 <= 6400.0f + 0.01f, "right edge held: view.right <= map width");
    check(c.y + VH/2 <= 736.0f + 0.01f,  "bottom edge held: view.bottom <= map height");
}

void testCameraOnShortMapKeepsGroundVisible() {
    section("C-1  map shorter than the view anchors to the ground, not the centre");

    Camera cam;
    const float VH = 720.0f, mapH = 640.0f;             // level_1_sub dimensions
    cam.setBounds(AABB{0.0f, 0.0f, 2080.0f, mapH});

    const sf::Vector2f c = cam.clampToBounds({1000.0f, 0.0f});
    const float viewBottom = c.y + VH/2;

    // Centring gave view.bottom = 680, i.e. 40px of void below the ground.
    check(std::abs(viewBottom - mapH) < 0.01f,
          "view bottom sits exactly on the map bottom (no void below the ground)");
    check(c.y - VH/2 < 0.0f, "the leftover space is above the map, which reads as sky");
}

void testShakeCannotEscapeBounds() {
    section("C-2  screen shake cannot push the view outside the map");

    Camera cam;
    cam.setBounds(AABB{0.0f, 0.0f, 6400.0f, 736.0f});
    cam.setPosition({640.0f, 368.0f});
    cam.triggerScreenShake(ShakePreset::Heavy);         // 6px, 0.30s

    const float VW = 1280.0f, VH = 720.0f;
    bool everOutside = false;
    for (int i = 0; i < 40; ++i) {
        cam.setPosition({VW / 2.0f, VH / 2.0f});        // pinned to the top-left corner
        cam.update(Constants::FIXED_TIMESTEP);
        const AABB v = cam.getVisibleBounds();
        if (v.x < -0.01f || v.y < -0.01f) everOutside = true;
    }
    check(!everOutside, "40 frames of Heavy shake at the corner never left the map");
}

} // namespace

int main() {
    std::cout << "Audit regression suite\n";

    testCoinTilesLoad();
    testTileNameRoundTrip();
    testStarDecoratorExpiry();
    testMegaDecoratorExpiry();
    testRestoreStatsIsSilent();
    testRewindRestoresByIdNotIndex();
    testCoyoteTime();
    testCoyoteWindowExpires();
    testJumpBufferFiresOnLanding();
    testPowerUpPreservesStar();
    testEntityTypeNameIsVirtual();
    testCameraClampsToMap();
    testCameraOnShortMapKeepsGroundVisible();
    testShakeCannotEscapeBounds();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
