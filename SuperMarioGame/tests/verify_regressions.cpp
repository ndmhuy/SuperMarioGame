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
#include "Utils/MapGenerator.hpp"
#include "Utils/LevelSolvability.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Graphics/Camera.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/EntityFactory.hpp"
#include "Core/InputManager.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/IGameState.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Utils/CampaignProgress.hpp"
#include "Utils/ObjectPool.hpp"
#include "Utils/EntityConfig.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/ColorPalette.hpp"
#include "Utils/MetaGame.hpp"
#include "Core/DebugConsole.hpp"
#include "Core/SoundManager.hpp"
#include "Core/ReplayRecorder.hpp"
#include "Utils/Serializer.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/BoomBoom.hpp"
#include "Core/DifficultyStrategy.hpp"
#include "Core/Game.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/BossFireball.hpp"
#include "Entities/Hammer.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Castle.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Spiny.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Core/RunCommand.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/Item.hpp"
#include "Entities/Block.hpp"
#include "Entities/PiranhaPlant.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <set>
#include <SFML/Graphics/RenderTexture.hpp>
#include "TestSaveSandbox.hpp"

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

// TileMap answers "what tile is at this point"; solidity is a property of the
// tile's TileInfo. Water and lava are tiles but nothing stands on them.
bool solidAt(const TileMap& map, float x, float y) {
    return TileMap::getInfo(map.getTileAt(x, y)).isSolid;
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

// ---------------------------------------------------------------------------
// Content regression — the shipped levels once held 2-3 enemies each and every
// question block defaulted to a coin, so no power-up existed anywhere.
// ---------------------------------------------------------------------------
void testLevelsArePopulated() {
    section("content  levels carry enemies and reachable power-ups");

    const std::vector<std::string> mainLevels = {
        "level_1.json", "level_2.json", "level_3.json", "bonus_1.json"
    };

    int totalPowerUpBlocks = 0;
    for (const auto& name : mainLevels) {
        const std::string path = levelPath(name);
        if (!std::filesystem::exists(path)) { check(false, name + " exists"); continue; }

        TileMap map; LevelData data; LevelLoader loader;
        if (!loader.loadLevel(path, map, data)) { check(false, name + " loads"); continue; }

        int enemies = 0, powerUpBlocks = 0;
        for (const auto& e : data.entities) {
            if (!e) continue;
            if (e->getCategory() == EntityCategory::Enemy) ++enemies;
            if (auto* qb = dynamic_cast<QuestionBlock*>(e.get())) {
                if (qb->getContainedItemType() != QuestionBlock::Coin) ++powerUpBlocks;
            }
        }
        totalPowerUpBlocks += powerUpBlocks;

        const float perTile = static_cast<float>(map.getWidth()) / std::max(enemies, 1);
        check(enemies >= 6,
              name + " has a real enemy population (" + std::to_string(enemies) +
              ", 1 per " + std::to_string(static_cast<int>(perTile)) + " tiles)");
    }

    check(totalPowerUpBlocks > 0,
          "at least one question block in the campaign holds a power-up (" +
          std::to_string(totalPowerUpBlocks) + " do)");
}

// ---------------------------------------------------------------------------
// Member B findings.
// ---------------------------------------------------------------------------
void testHiddenBlockCanBeRevealed() {
    section("B-3  a hidden block is collidable, so it can be found at all");

    HiddenBlock block({100.0f, 100.0f});
    check(!block.isRevealed(), "starts hidden");
    check(block.isCollidable(),
          "still collidable while hidden — it returned a zero AABB before, which "
          "made onHitFromBelow unreachable and the block undiscoverable");

    Mario mario({100.0f, 140.0f});
    block.onHitFromBelow(mario);
    check(block.isRevealed(), "revealed once hit from below");
}

void testMovingPlatformActuallyMoves() {
    section("B-5  moving platforms have a travel range");

    auto entity = EntityFactory::create(EntityType::MovingPlatform, {200.0f, 200.0f});
    check(entity != nullptr, "factory builds a MovingPlatform");
    if (!entity) return;

    const sf::Vector2f start = entity->getPosition();
    for (int i = 0; i < 60; ++i) entity->update(Constants::FIXED_TIMESTEP);

    check(std::abs(entity->getPosition().x - start.x) > 1.0f,
          "it has moved after a second (factory passed a zero range before)");
}

void testNonCollidableEntitiesAreNotAtTheOrigin() {
    section("B-14 disabled entities report themselves, not a box at (0,0)");

    Goomba goomba({500.0f, 300.0f});
    check(goomba.isCollidable(), "a live goomba collides");

    goomba.onStomped();                       // squished
    check(!goomba.isCollidable(), "a squished goomba opts out of collision");

    const AABB box = goomba.getBoundingBox();
    check(box.x > 400.0f,
          "its bounding box still reports its real position, not (0,0) "
          "(the old zero-AABB idiom polluted spatial-hash cell 0,0)");
}

void testKeyBindingsApply() {
    section("B-11 persisted key bindings reach the InputManager");

    sf::Keyboard::Key parsed;
    check(InputManager::parseKeyName("W", parsed) && parsed == sf::Keyboard::Key::W,
          "key names parse");
    check(InputManager::keyName(sf::Keyboard::Key::LShift) == "LShift", "and round-trip");
    check(!InputManager::parseKeyName("NotAKey", parsed), "nonsense is rejected, not guessed");

    // Rebinding jump to J must not throw and must be accepted.
    InputManager::getInstance().applyBindings({{"jump", "J"}}, 0);
    check(true, "applyBindings accepts a valid rebind without throwing");
}


// --- Tier 1: front-end states ------------------------------------------------

// A state that only records what the manager did to it. Deliberately trivial:
// the thing under test is GameStateManager, not any real screen.
class ProbeState : public IGameState {
public:
    ProbeState(std::string name, std::vector<std::string>* log, bool overlay)
        : m_name(std::move(name)), m_log(log), m_overlay(overlay) {}

    void enter() override      { m_log->push_back(m_name + ":enter"); }
    void exit() override       { m_log->push_back(m_name + ":exit"); }
    void handleInput(const sf::Event&) override { m_log->push_back(m_name + ":input"); }
    void update(float) override { m_log->push_back(m_name + ":update"); }
    void render(sf::RenderTarget&) override { m_log->push_back(m_name + ":render"); }
    bool isOverlay() const override { return m_overlay; }
    void onSuspend() override  { m_log->push_back(m_name + ":suspend"); }
    void onResume() override   { m_log->push_back(m_name + ":resume"); }

private:
    std::string m_name;
    std::vector<std::string>* m_log;
    bool m_overlay;
};

// Pops itself from inside update(). Before push/pop were deferred this destroyed
// the object whose update() was still on the stack.
class SelfPoppingState : public IGameState {
public:
    SelfPoppingState(GameStateManager* gsm, std::vector<std::string>* log)
        : m_gsm(gsm), m_log(log) {}

    void enter() override { m_log->push_back("selfpop:enter"); }
    void exit() override  { m_log->push_back("selfpop:exit"); }
    void handleInput(const sf::Event&) override {}
    void update(float) override {
        m_log->push_back("selfpop:update");
        m_gsm->popState();
        // Touching a member after the pop request is the whole point: with an
        // immediate pop this line ran on a destroyed object.
        m_log->push_back("selfpop:still-alive");
    }
    void render(sf::RenderTarget&) override {}
    bool isOverlay() const override { return true; }

private:
    GameStateManager* m_gsm;
    std::vector<std::string>* m_log;
};

bool logContains(const std::vector<std::string>& log, const std::string& entry) {
    return std::find(log.begin(), log.end(), entry) != log.end();
}

int indexOf(const std::vector<std::string>& log, const std::string& entry) {
    auto it = std::find(log.begin(), log.end(), entry);
    return (it == log.end()) ? -1 : static_cast<int>(std::distance(log.begin(), it));
}

void testOverlayStatesRenderTheStack() {
    section("7.5 GameStateManager renders the whole stack, not just the top");

    std::vector<std::string> log;
    GameStateManager gsm;

    gsm.pushState(std::make_unique<ProbeState>("base", &log, false));
    gsm.update(0.016f);                    // applies the deferred push
    check(gsm.getStateCount() == 1, "a pushed state is applied at the frame boundary");

    log.clear();
    gsm.pushState(std::make_unique<ProbeState>("overlay", &log, true));
    gsm.update(0.016f);

    check(logContains(log, "base:suspend"), "the covered state is told it was suspended");
    check(indexOf(log, "base:suspend") < indexOf(log, "overlay:enter"),
          "and it is told before the new state enters");
    check(logContains(log, "overlay:update") && !logContains(log, "base:update"),
          "only the top of the stack updates");

    // render() needs a target. A RenderTexture needs a graphics context, which is
    // not guaranteed on a headless runner, so this half is skipped if it fails.
    log.clear();
    bool haveTarget = false;
    try {
        sf::RenderTexture texture({64u, 64u});
        haveTarget = true;
        gsm.render(texture);
    } catch (...) {
        haveTarget = false;
    }

    if (haveTarget) {
        check(logContains(log, "base:render") && logContains(log, "overlay:render"),
              "an overlay does not hide the state beneath it");
        check(indexOf(log, "base:render") < indexOf(log, "overlay:render"),
              "and the overlay draws on top, not underneath");
    } else {
        std::cout << "  [skip] render-order checks (no graphics context)\n";
    }

    log.clear();
    gsm.popState();
    gsm.update(0.016f);
    check(logContains(log, "overlay:exit") && logContains(log, "base:resume"),
          "popping an overlay resumes the state below it");
}

void testStateOperationsAreDeferred() {
    section("7.5 a state may pop itself from update() without self-destructing");

    std::vector<std::string> log;
    GameStateManager gsm;
    gsm.pushState(std::make_unique<ProbeState>("base", &log, false));
    gsm.update(0.016f);

    log.clear();
    gsm.pushState(std::make_unique<SelfPoppingState>(&gsm, &log));
    // One update() applies the push, runs the new state's update() — in which it
    // asks to be popped — and then applies that pop, all at frame boundaries.
    gsm.update(0.016f);

    check(logContains(log, "selfpop:still-alive"),
          "the state survives its own popState() call");
    check(logContains(log, "selfpop:exit"), "and the pop is applied afterwards");
    check(gsm.getStateCount() == 1, "leaving only the state underneath");
}

void testLevelCatalogCoversTheCampaign() {
    section("7.7 the campaign order is one list, and every level in it exists");

    // Four main-path levels. The three *_sub rooms used to be listed here as
    // ordinary campaign stages; they carry no flagpole, so the campaign advanced
    // into one and could never leave. They are reached through their pipes now,
    // which is what testCampaignPathContainsOnlyCompletableLevels() guards.
    check(LevelCatalog::count() == 4, "four main-path levels, matching advanceToNextLevel()");
    check(!LevelCatalog::isValidIndex(-1) && !LevelCatalog::isValidIndex(LevelCatalog::count()),
          "out-of-range indices are rejected, not clamped silently");

    int found = 0;
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        const std::string& rel = LevelCatalog::pathFor(i);
        const std::string name = rel.substr(rel.find_last_of('/') + 1);
        if (std::filesystem::exists(levelPath(name))) ++found;
    }
    check(found == LevelCatalog::count(), "every catalogued level file is on disk");
    check(LevelCatalog::nameFor(0) == "1-1", "display names are the ones the HUD uses");
}

void testHighScoreTableIsSortedAndBounded() {
    section("7.8 the high-score table sorts, truncates and rejects empty runs");

    // No backup dance any more. It used to rename "saves/highscores.json" aside
    // — a path relative to the WORKING DIRECTORY — while Serializer wrote to
    // saveDirectory(), which resolves independently and skips build trees. Run
    // from build/ the two were different files: the backup missed, the
    // assertions read the developer's real table, and "the best run is at the
    // top" failed against scores this test never wrote. Under ctest, whose
    // working directory is the source root, they happened to coincide and it
    // passed. TestSaveSandbox now guarantees an empty directory either way, so
    // this case starts from nothing however it is launched.
    check(Serializer::loadHighScores().empty(),
          "the table starts empty - the sandbox, not whatever is on this machine");

    for (int i = 1; i <= 14; ++i) {
        HighScoreEntry entry;
        entry.score = i * 1000;
        entry.coins = i;
        entry.levelName = "1-1";
        Serializer::recordHighScore(entry);
    }

    HighScoreEntry empty;
    empty.score = 0;
    check(!Serializer::recordHighScore(empty), "a zero score is not recorded");

    std::vector<HighScoreEntry> scores = Serializer::loadHighScores();
    check(static_cast<int>(scores.size()) == Serializer::MAX_HIGH_SCORES,
          "the table is capped at MAX_HIGH_SCORES");

    bool descending = true;
    for (std::size_t i = 1; i < scores.size(); ++i) {
        if (scores[i - 1].score < scores[i].score) descending = false;
    }
    check(descending, "and stays sorted highest first");
    check(!scores.empty() && scores.front().score == 14000, "the best run is at the top");

    // Leave the table empty for whatever runs next in this process.
    Serializer::clearHighScores();
}

// Atlas lookups the renderer performs by name. A missing frame is silent at
// runtime — the sprite simply does not draw — which is exactly how the flagpole
// shipped with no flag.
std::unique_ptr<SpriteSheet> loadAtlas(const std::string& folder) {
    const std::vector<std::string> roots = {
        "assets/spriteSheet/", "../assets/spriteSheet/", "../../assets/spriteSheet/",
        "SuperMarioGame/assets/spriteSheet/"
    };
    for (const auto& root : roots) {
        if (std::filesystem::exists(root + folder)) {
            try {
                return std::make_unique<SpriteSheet>(root + folder);
            } catch (...) {}
        }
    }
    return nullptr;
}

void testFlagpoleFramesExist() {
    section("Flagpole names frames that are actually in the atlas");

    auto sheet = loadAtlas("world_scenery_item");
    if (!sheet) {
        check(false, "world_scenery_item atlas loads");
        return;
    }

    check(sheet->hasFrame("pole_flag_green"), "the raised flag frame exists");
    bool descent = true;
    for (int i = 0; i < 5; ++i) {
        if (!sheet->hasFrame("full_flag_pole_" + std::to_string(i))) descent = false;
    }
    check(descent, "and all five descent frames exist");
    check(!sheet->hasFrame("flag_white") && !sheet->hasFrame("flag_reddish_white"),
          "the names the old code asked for are still absent, confirming why nothing drew");
}

void testPowerUpFramesExistForEveryCharacter() {
    section("Super / Fire / Cape have their own frames for all four characters");

    auto sheet = loadAtlas("player");
    if (!sheet) {
        check(false, "player atlas loads");
        return;
    }

    // The prefixes Player::refreshStateAnimations() asks for, per base state.
    const std::vector<std::string> forms = {"small", "super", "fire", "cape", "tiny"};
    const std::vector<std::string> characters = {"mario", "luigi", "toad", "peach"};

    int missing = 0;
    for (const auto& character : characters) {
        for (const auto& form : forms) {
            const std::string prefix = character + "_" + form;
            const bool ok = sheet->hasFrame(prefix + "_idle") || sheet->hasFrame(prefix + "_walk_0");
            if (!ok) {
                std::cout << "        missing: " << prefix << "\n";
                ++missing;
            }
        }
    }
    check(missing == 0, "every character has frames for every base player state");

    // The derived forms must be visibly different from _small, or the power-up
    // is invisible again in a new way.
    check(sheet->hasFrame("mario_super_idle") && sheet->hasFrame("mario_fire_idle")
          && sheet->hasFrame("mario_cape_idle"),
          "and the derived Super/Fire/Cape frames are named as the mapping expects");
}


// --- Tier 2: bosses ----------------------------------------------------------

void testProjectilesOnlyHurtWhatTheyShould() {
    section("9.x  every projectile answers for who it may damage");

    Fireball playerShot({0.0f, 0.0f}, {200.0f, 0.0f});
    Hammer hammer({0.0f, 0.0f}, {200.0f, -300.0f});
    BossFireball breath({0.0f, 0.0f}, {200.0f, 0.0f});

    check(playerShot.damagesEnemies() && !playerShot.damagesPlayer(),
          "the player's fireball kills enemies and not the player");
    check(!hammer.damagesEnemies() && hammer.damagesPlayer(),
          "a thrown hammer hurts the player only — it used to kill enemies, because "
          "the resolver static_cast every projectile to Fireball");
    check(!breath.damagesEnemies() && breath.damagesPlayer(),
          "Bowser's fire breath hurts the player only");

    // The resolver's static_cast<Projectile&> is only sound while this category
    // means exactly "is a Projectile".
    check(playerShot.getCategory() == EntityCategory::Projectile &&
          hammer.getCategory() == EntityCategory::Projectile &&
          breath.getCategory() == EntityCategory::Projectile,
          "and all three report the Projectile category the resolver dispatches on");
}

void testBowserTakesFiveHitsAndChangesPhase() {
    section("9.3  Bowser is a five-hit fight with a phase change at half health");

    Bowser bowser({100.0f, 100.0f});
    check(bowser.getMaxHealth() == 5, "five hits, not one");
    check(bowser.getPhase() == 1, "starts in phase 1");

    int defeatedEvents = 0;
    const auto sub = EventBus::getInstance().subscribe(
        EventType::BossDefeated, [&defeatedEvents](const GameEvent&) { ++defeatedEvents; });

    // Fire does nothing: he breathes it.
    bowser.onHitByFireball();
    check(bowser.getHealth() == 5, "immune to fireballs");

    auto stompAndSettle = [&bowser]() {
        bowser.onStomped();
        // Past the i-frames, so the next stomp is a separate hit.
        for (int i = 0; i < 70; ++i) bowser.update(1.0f / 60.0f);
    };

    stompAndSettle();
    check(bowser.getHealth() == 4, "a stomp costs exactly one health");
    stompAndSettle();
    check(bowser.getPhase() == 1, "still phase 1 above half health");
    stompAndSettle();
    check(bowser.getPhase() == 2, "phase 2 once health drops to half");

    stompAndSettle();
    stompAndSettle();
    check(bowser.isDefeated(), "five hits defeat him");
    check(defeatedEvents == 1, "BossDefeated is published exactly once");

    // The defeat sequence has to finish before he is removed, or the health bar
    // vanishes the instant the last hit lands.
    for (int i = 0; i < 200; ++i) bowser.update(1.0f / 60.0f);
    check(!bowser.isActive(), "and he is removed once the defeat animation ends");

    EventBus::getInstance().unsubscribe(sub);
}

void testBossInvulnerabilityWindowStopsDoubleCounting() {
    section("9.x  contact damage across frames counts as one hit");

    Bowser bowser({100.0f, 100.0f});
    // A stomp overlaps for several frames; without i-frames each frame was a hit
    // and the whole bar drained at once.
    for (int i = 0; i < 10; ++i) {
        bowser.onStomped();
        bowser.update(1.0f / 60.0f);
    }
    check(bowser.getHealth() == 4, "ten overlapping frames cost one health, not ten");
}

void testLevelThreeActuallyContainsItsBoss() {
    section("9.3  the level named after Bowser contains Bowser, with an arena");

    TileMap map;
    LevelData data;
    LevelLoader loader;
    const std::string path = levelPath("level_3.json");
    if (!loader.loadLevel(path, map, data)) {
        check(false, "level_3.json loads");
        return;
    }

    Boss* boss = nullptr;
    for (const auto& entity : data.entities) {
        if (auto* b = dynamic_cast<Boss*>(entity.get())) boss = b;
    }
    check(boss != nullptr, "Bowser's Castle Fortress contains a boss");
    if (!boss) return;

    check(boss->getDisplayName() == "BOWSER", "and it is Bowser");
    check(boss->hasArena(), "with an arena to fight him in");
    check(boss->getArena().width > 0.0f && boss->getArena().height > 0.0f,
          "whose bounds are real, so the camera lock has something to clamp to");

    // The arena has to gate the flagpole, or the fight is optional.
    float flagpoleX = -1.0f;
    for (const auto& entity : data.entities) {
        if (entity && entity->getTypeName() == "flagpole") flagpoleX = entity->getPosition().x;
    }
    check(flagpoleX > boss->getArena().x,
          "and the flagpole sits past it, so Bowser cannot be walked around");
}


void testBoomBoomEscalatesOncePerHit() {
    section("9.2  Boom Boom is a three-stomp fight that escalates per hit");

    BoomBoom boss({400.0f, 100.0f});
    check(boss.getMaxHealth() == 3, "three stomps, as the SPEC says");
    check(boss.getPhase() == 1, "starts in phase 1");

    auto stompAndSettle = [&boss]() {
        boss.onStomped();
        for (int i = 0; i < 70; ++i) boss.update(1.0f / 60.0f);
    };

    stompAndSettle();
    check(boss.getPhase() == 2, "one hit moves him to phase 2 — the default "
                                "half-health split cannot express three steps");
    stompAndSettle();
    check(boss.getPhase() == 3, "two hits reach phase 3, where the spin attack lives");
    stompAndSettle();
    check(boss.isDefeated(), "and the third finishes him");
}

void testBoomBoomStaysInsideItsArena() {
    section("9.2  Boom Boom cannot charge out of the room he is fought in");

    BoomBoom boss({400.0f, 100.0f});
    boss.setArena(AABB{320.0f, 0.0f, 480.0f, 736.0f});

    for (int i = 0; i < 600; ++i) {
        boss.update(1.0f / 60.0f);
        // The physics engine is not running here, so integrate the charge by hand.
        boss.setPosition({boss.getPosition().x + boss.getVelocity().x * (1.0f / 60.0f),
                          boss.getPosition().y});
    }

    const float x = boss.getPosition().x;
    check(x >= 320.0f && x <= 800.0f,
          "ten seconds of charging leaves him inside the arena (x = " +
          std::to_string(static_cast<int>(x)) + ")");
}

void testLevelTwoContainsItsMidBoss() {
    section("9.2  Level 2 contains Boom Boom, gating its flagpole");

    TileMap map;
    LevelData data;
    LevelLoader loader;
    if (!loader.loadLevel(levelPath("level_2.json"), map, data)) {
        check(false, "level_2.json loads");
        return;
    }

    Boss* boss = nullptr;
    for (const auto& entity : data.entities) {
        if (auto* b = dynamic_cast<Boss*>(entity.get())) boss = b;
    }
    check(boss != nullptr, "Level 2 has a mid-boss");
    if (!boss) return;

    check(boss->getDisplayName() == "BOOM BOOM", "and it is Boom Boom");
    check(boss->hasArena(), "with an arena");

    float flagpoleX = -1.0f;
    for (const auto& entity : data.entities) {
        if (entity && entity->getTypeName() == "flagpole") flagpoleX = entity->getPosition().x;
    }
    check(flagpoleX > boss->getArena().x,
          "and the flagpole is past it — SPEC 6.4's 'opens path to the flagpole'");

    // The check above only asked whether the flagpole is past the arena's LEFT
    // edge, which was already true even when the arena was wide enough to
    // swallow the flagpole whole: a player who simply ran right, inside the
    // "no escape until defeated" clamp (PlayingState::updateBossArena), still
    // landed on a touchable flagpole with Boom Boom alive — the level ended
    // without the fight ever happening. The real invariant is that the arena's
    // RIGHT edge must clear the flagpole, the way Bowser's does in level_3.json.
    const AABB& arena = boss->getArena();
    check(flagpoleX >= arena.x + arena.width,
          "and specifically past the arena's far edge, not just its near one — "
          "the arena cannot overlap the flagpole it is supposed to gate");
}

void testEndOfLevelCastleHasBuffer() {
    section("9.2  the end-of-level castle leaves room past its own back wall");

    // MapGenerator::MapGenerator.cpp enforces castleStartX + Castle::WIDTH_TILES
    // + 1 < config.width for procedurally-generated levels, but the hand-authored
    // campaign files went through no such check — level_2.json and level_3.json
    // once placed the castle with its back wall 16px (half a tile) from the
    // level boundary, a fifth of the buffer level_1.json and bonus_1.json leave,
    // which is what read as an unfinished-looking ending on those two stages.
    for (const std::string& file : {"level_2.json", "level_3.json"}) {
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(levelPath(file), map, data)) {
            check(false, file + " loads");
            continue;
        }

        float castleX = -1.0f;
        for (const auto& entity : data.entities) {
            if (entity && entity->getTypeName() == "castle") castleX = entity->getPosition().x;
        }
        check(castleX >= 0.0f, file + " has a castle");
        if (castleX < 0.0f) continue;

        const float castleRight = castleX + Castle::WIDTH_TILES * Constants::TILE_SIZE;
        const float levelRight  = data.width * Constants::TILE_SIZE;
        check(levelRight - castleRight >= Constants::TILE_SIZE,
              file + "'s castle leaves at least one tile of buffer to the level edge");
    }
}

void testGeneratedLevelsAreVerifiedSolvable() {
    section("16.2  MapGenerator::generateSolvable actually verifies what it claims");

    // The pit/platform placement heuristics in MapGenerator.cpp are trusted on
    // faith today — "Generated winnable procedural level" is printed
    // unconditionally, whether or not the level actually is. generateSolvable()
    // adds an independent BFS reachability check (Utils/LevelSolvability) and
    // retries with a new seed if it fails. This is a stress pass across every
    // theme/difficulty combination the menu's generator page can produce.
    int verifiedCount = 0;
    int total = 0;
    for (int themeIdx = 0; themeIdx < 4; ++themeIdx) {
        for (int diffIdx = 0; diffIdx < 3; ++diffIdx) {
            MapGeneratorConfig config;
            config.theme = static_cast<MapTheme>(themeIdx);
            config.difficulty = static_cast<MapDifficulty>(diffIdx);
            config.width = 200;
            config.seed = static_cast<unsigned int>(1000 + themeIdx * 10 + diffIdx);

            TileMap map;
            std::vector<std::unique_ptr<Entity>> entities;
            const bool verified = MapGenerator::generateSolvable(map, entities, config);
            ++total;
            if (verified) ++verifiedCount;

            // Whatever generateSolvable returned, re-check independently — the
            // function must not claim success on a level LevelSolvability
            // itself would reject.
            const bool actuallyReachable =
                LevelSolvability::isPathReachable(map, entities, 3, config.width - 15);
            check(!verified || actuallyReachable,
                  "theme " + std::to_string(themeIdx) + "/difficulty " + std::to_string(diffIdx) +
                  ": a level generateSolvable verified is independently reachable");
        }
    }
    check(verifiedCount == total,
          "every theme/difficulty combination generated a verifiably solvable level ("
          + std::to_string(verifiedCount) + "/" + std::to_string(total) + ")");
}

void testEveryBossTypeIsBuildable() {
    section("9.x  EntityFactory builds every boss the level format can name");

    // Both used to return nullptr, so a level naming one silently got nothing.
    auto bowser = EntityFactory::create(EntityType::Bowser, {0.0f, 0.0f});
    auto boomBoom = EntityFactory::create(EntityType::BoomBoom, {0.0f, 0.0f});
    check(dynamic_cast<Boss*>(bowser.get()) != nullptr, "bowser builds");
    check(dynamic_cast<Boss*>(boomBoom.get()) != nullptr, "boom_boom builds");

    check(SerializationUtils::parseEntityTypeName("bowser") == EntityType::Bowser,
          "and the level format's name for it round-trips");
    check(SerializationUtils::parseEntityTypeName("boom_boom") == EntityType::BoomBoom,
          "likewise boom_boom");
}


void testDifficultyStrategyActuallyChangesTheGame() {
    section("9.4  the difficulty setting is read by something at last");

    Game& game = Game::getInstance();
    const std::string original = game.getDifficulty();

    game.setDifficulty("normal");
    const float normalSpeed = game.difficulty().enemySpeedScale();
    const int   normalLives = game.difficulty().startingLives();
    const float normalTime  = game.difficulty().levelTimeScale();

    game.setDifficulty("easy");
    check(game.difficulty().getId() == "easy", "setDifficulty swaps the live strategy");
    check(game.difficulty().enemySpeedScale() < normalSpeed, "easy slows enemies down");
    check(game.difficulty().startingLives() > normalLives, "and hands out more lives");
    check(game.difficulty().levelTimeScale() > normalTime, "and more time on the clock");

    game.setDifficulty("hard");
    check(game.difficulty().enemySpeedScale() > normalSpeed, "hard speeds enemies up");
    check(game.difficulty().startingLives() < normalLives, "with fewer lives");
    check(game.difficulty().levelTimeScale() < normalTime, "and less time");

    // config.json is hand-editable, so a typo must not take the game down.
    game.setDifficulty("bananas");
    check(game.difficulty().getId() == "normal", "an unrecognised id falls back to normal");

    game.setDifficulty(original);
}

void testDifficultyScalesEnemiesAndBosses() {
    section("9.4  the modifiers reach enemies and boss health, not just a struct");

    Game& game = Game::getInstance();
    const std::string original = game.getDifficulty();

    // Enemy speed is scaled once, as the entity enters the world.
    Goomba goomba({0.0f, 0.0f});
    const float baseSpeed = goomba.getSpeed();
    goomba.applySpeedScale(2.0f);
    const float scaledSpeed = goomba.getSpeed();
    check(scaledSpeed > baseSpeed, "applySpeedScale moves an enemy's speed");
    goomba.applySpeedScale(-1.0f);
    check(goomba.getSpeed() == scaledSpeed, "and a nonsense scale is ignored, not applied");

    // Boss health is scaled at construction.
    game.setDifficulty("normal");
    const int normalHealth = Bowser({0.0f, 0.0f}).getMaxHealth();
    game.setDifficulty("easy");
    const int easyHealth = Bowser({0.0f, 0.0f}).getMaxHealth();
    game.setDifficulty("hard");
    const int hardHealth = Bowser({0.0f, 0.0f}).getMaxHealth();

    check(easyHealth < normalHealth, "easy shortens Bowser's bar (" +
          std::to_string(easyHealth) + " vs " + std::to_string(normalHealth) + ")");
    check(hardHealth > normalHealth, "hard lengthens it (" + std::to_string(hardHealth) + ")");
    check(easyHealth >= 1, "and never below one hit, whatever the multiplier");

    game.setDifficulty(original);
}


void testThwompRunsItsStateMachine() {
    section("9.1  Thwomp's animation follows its state machine, not a magic y value");

    Thwomp thwomp({320.0f, 96.0f});
    const auto* strategy = dynamic_cast<const ProximityTriggerStrategy*>(thwomp.getStrategy());
    check(strategy != nullptr, "it runs a ProximityTriggerStrategy");
    if (!strategy) return;

    check(strategy->getState() == ProximityState::Idle, "and starts Idle, waiting on the ceiling");
    check(strategy->getDebugState() == "Idle", "reporting that state by name for the AI overlay");

    // No player registered, so nothing triggers it: it must stay parked rather
    // than slamming because it happens to sit below y = 140.
    for (int i = 0; i < 120; ++i) thwomp.update(1.0f / 60.0f);
    check(strategy->getState() == ProximityState::Idle,
          "an untriggered Thwomp stays Idle — the old code read the sprite from "
          "position.y > 140, so one placed low in a level slammed forever");

    // Drive the machine directly through the four states.
    auto* mutableStrategy = const_cast<ProximityTriggerStrategy*>(strategy);
    mutableStrategy->setState(ProximityState::Slamming);
    check(strategy->getDebugState() == "Slamming", "Slamming reports by name");
    mutableStrategy->setState(ProximityState::Resting);
    check(strategy->getDebugState() == "Resting", "Resting reports by name");
    mutableStrategy->setState(ProximityState::Rising);
    check(strategy->getDebugState() == "Rising", "Rising reports by name");
}

void testEveryStrategyIdentifiesItself() {
    section("9.1  every shipped strategy names itself for the AI overlay");

    // An enemy whose strategy will not name itself is invisible in the overlay,
    // which is the one thing the overlay exists to prevent.
    struct Case { const char* enemy; std::unique_ptr<Entity> entity; };
    std::vector<std::pair<std::string, std::unique_ptr<Entity>>> cases;
    for (EntityType type : {EntityType::Goomba, EntityType::KoopaTroopa,
                            EntityType::KoopaParatroopa, EntityType::Boo,
                            EntityType::PiranhaPlant, EntityType::BulletBill,
                            EntityType::HammerBro, EntityType::Thwomp,
                            EntityType::ChainChomp, EntityType::Lakitu,
                            EntityType::Spiny}) {
        auto entity = EntityFactory::create(type, {100.0f, 100.0f});
        if (entity) cases.emplace_back(entity->getTypeName(), std::move(entity));
    }

    int unnamed = 0;
    int strategyless = 0;
    for (const auto& [name, entity] : cases) {
        auto* enemy = dynamic_cast<Enemy*>(entity.get());
        if (!enemy) continue;
        const IMovementStrategy* strategy = enemy->getStrategy();
        if (!strategy) {
            std::cout << "        no strategy: " << name << "\n";
            ++strategyless;
            continue;
        }
        if (strategy->getName() == "Strategy") {   // the un-overridden default
            std::cout << "        unnamed strategy on: " << name << "\n";
            ++unnamed;
        }
    }

    check(!cases.empty(), "the factory built the enemy roster");
    check(unnamed == 0, "no enemy is running an unnamed strategy");
    std::cout << "        (" << strategyless << " enemies drive themselves without a strategy)\n";
}


void testCampaignProgressUnlocksSequentially() {
    section("7.3  campaign progress is recorded, not fabricated");

    // This case calls CampaignProgress::reset(), which is a
    // std::filesystem::remove on a path resolved through
    // Serializer::saveDirectory(). The backup dance that used to sit here
    // renamed "saves/progress.json" — relative to the working directory —
    // which is NOT necessarily the file reset() deletes. Run from build/ they
    // were different files and the real progress.json was erased; that was
    // observed, with a seeded file vanishing between two game launches.
    // TestSaveSandbox now points reset() at a throwaway directory, so there is
    // nothing real to lose and nothing to put back.
    CampaignProgress::reset();

    check(CampaignProgress::isUnlocked(0), "the first level is always open");
    check(!CampaignProgress::isUnlocked(1), "and the second is not, on a fresh profile");
    check(CampaignProgress::highestUnlockedIndex() == 0, "so the map opens on 1-1");

    CampaignProgress::recordLevelCleared(0, {true, false, true});
    check(CampaignProgress::isUnlocked(1), "clearing a level unlocks the next one");
    check(CampaignProgress::highestUnlockedIndex() == 1, "and the map follows the player forward");
    check(!CampaignProgress::isUnlocked(2), "but only the next one — unlocking is sequential");
    check(CampaignProgress::totalStarCoins() == 2, "the star coins that were found are kept");

    // Serializer's own progress block generates levelsCompleted as [1..levelId-1],
    // which would have claimed everything before the current level was cleared.
    const std::vector<LevelProgress> progress = CampaignProgress::load();
    check(progress.size() == static_cast<std::size_t>(LevelCatalog::count()),
          "progress is always sized to the campaign, so callers can index it");
    check(progress[0].completed && !progress[1].completed,
          "only levels actually finished are marked finished");

    // Replaying a level worse than before must not take coins away.
    CampaignProgress::recordLevelCleared(0, {false, true, false});
    const std::vector<LevelProgress> merged = CampaignProgress::load();
    check(merged[0].starCoinCount() == 3,
          "a second run merges its coins in rather than overwriting them");

    check(!CampaignProgress::isUnlocked(-1) && !CampaignProgress::isUnlocked(999),
          "out-of-range levels are locked, not crashes");

    CampaignProgress::reset();
}


void testOnlyFireMarioCanShoot() {
    section("gameplay  only the Fire state may throw fireballs");

    Mario mario({100.0f, 100.0f});
    check(dynamic_cast<SmallState*>(mario.getCurrentState()) != nullptr, "starts Small");
    check(!mario.canShootFireball(),
          "Small Mario cannot shoot — shootFireball() had NO state check at all, "
          "so every form could, which made the Fire Flower pointless");

    mario.powerUp(0); // Super
    check(!mario.canShootFireball(), "nor can Super Mario");

    mario.powerUp(2); // CapeFeather
    check(dynamic_cast<CapeState*>(mario.getBaseState()) != nullptr, "reached Cape");
    check(!mario.canShootFireball(), "nor Cape Mario");

    mario.powerUp(3); // MiniMushroom
    check(!mario.canShootFireball(), "nor Mini Mario");

    mario.powerUp(1); // Fire
    check(dynamic_cast<FireState*>(mario.getBaseState()) != nullptr, "reached Fire");
    check(mario.canShootFireball(), "Fire Mario can");

    // A decorator wraps the form, it does not replace it.
    mario.powerUp(4); // Star
    check(dynamic_cast<StarDecorator*>(mario.getCurrentState()) != nullptr, "Star wraps Fire");
    check(mario.canShootFireball(),
          "and an invincible Fire Mario keeps his fireballs — getBaseState() "
          "unwraps the decorator rather than seeing through to nothing");

    // The cooldown is the other half of the gate.
    int shots = 0;
    const auto sub = EventBus::getInstance().subscribe(
        EventType::PlayerShotFireball, [&shots](const GameEvent&) { ++shots; });
    for (int i = 0; i < 10; ++i) mario.shootFireball();
    check(shots == 1, "ten presses in one frame fire once, not ten times");
    EventBus::getInstance().unsubscribe(sub);

    // Taking a hit steps down out of Fire, and the gate closes with it. Star has
    // to lapse first — it makes the player immune, so damage would be ignored.
    mario.powerUp(1);
    const int starFrames = static_cast<int>(Constants::STAR_DURATION * 60.0f) + 30;
    for (int i = 0; i < starFrames; ++i) mario.update(1.0f / 60.0f);
    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "the Star wore off");
    mario.setInvincible(0.0f);
    mario.takeDamage(1);
    check(!mario.canShootFireball(), "and powering down out of Fire closes the gate again");
}

void testFireballUsesItsAtlasArtAndBurns() {
    section("gameplay  the fireball draws the art the atlas has always shipped");

    auto sheet = loadAtlas("enemy_projectile");
    if (!sheet) {
        check(false, "enemy_projectile atlas loads");
        return;
    }
    bool flight = true;
    for (int i = 0; i < 4; ++i) {
        if (!sheet->hasFrame("flower_fireball_" + std::to_string(i))) flight = false;
    }
    check(flight, "the four-frame spin exists — it was being drawn with CircleShapes");
    bool burst = true;
    for (int i = 0; i < 3; ++i) {
        if (!sheet->hasFrame("flower_fireball_hit_" + std::to_string(i))) burst = false;
    }
    check(burst, "as does the three-frame impact burst, which nothing played");

    // A spent fireball bursts before it disappears, rather than vanishing.
    Fireball fireball({0.0f, 0.0f}, {200.0f, 0.0f});
    check(!fireball.isImpacting(), "a fresh fireball is flying");
    for (int i = 0; i < 4; ++i) fireball.bounce();
    check(fireball.isImpacting(), "its last bounce starts the burst");
    check(fireball.isActive(), "and it is still alive while the burst plays");
    for (int i = 0; i < 30; ++i) fireball.update(1.0f / 60.0f);
    check(!fireball.isActive(), "then it is gone");
}

void testFlagpoleFlagActuallyDescends() {
    section("gameplay  the flag reports the descent it now plays");

    Flagpole pole({100.0f, 100.0f}, 168.0f);
    check(pole.getFlagY() == 0.0f, "the flag starts at the top of the pole");

    Mario mario({100.0f, 200.0f});
    pole.onPlayerCollision(mario, 150.0f);

    for (int i = 0; i < 30; ++i) pole.update(1.0f / 60.0f);
    const float midway = pole.getFlagY();
    check(midway > 0.0f, "it slides once the player touches it — m_flagY was never "
                         "written at all, so it always read as still at the top");

    for (int i = 0; i < 90; ++i) pole.update(1.0f / 60.0f);
    check(pole.getFlagY() > midway, "and keeps going");
    check(pole.getFlagY() <= 168.0f, "stopping at the foot of the pole, not past it");
}


void testEveryEnemyCarriesItsOwnSpeed() {
    section("9.4  every enemy owns its speed, so the difficulty modifier reaches it");

    // Nine of thirteen enemies left Character::speed at zero and every strategy
    // substituted a literal of its own. getSpeed() answered 0 for enemies that
    // were visibly moving, and `speed *= 1.3` on zero is still zero — so Hard
    // only sped up four of them.
    const std::vector<std::pair<std::string, EntityType>> roster = {
        {"goomba", EntityType::Goomba}, {"koopa_troopa", EntityType::KoopaTroopa},
        {"koopa_paratroopa", EntityType::KoopaParatroopa}, {"boo", EntityType::Boo},
        {"piranha_plant", EntityType::PiranhaPlant}, {"bullet_bill", EntityType::BulletBill},
        {"hammer_bro", EntityType::HammerBro}, {"thwomp", EntityType::Thwomp},
        {"chain_chomp", EntityType::ChainChomp}, {"lakitu", EntityType::Lakitu},
        {"spiny", EntityType::Spiny}, {"bowser", EntityType::Bowser},
        {"boom_boom", EntityType::BoomBoom}
    };

    int zeroSpeed = 0;
    for (const auto& [name, type] : roster) {
        auto entity = EntityFactory::create(type, {100.0f, 100.0f});
        auto* enemy = dynamic_cast<Enemy*>(entity.get());
        if (!enemy) continue;
        if (enemy->getSpeed() <= 0.0f) {
            std::cout << "        speed is zero: " << name << "\n";
            ++zeroSpeed;
        }
    }
    check(zeroSpeed == 0, "no enemy reports a speed of zero");

    // And the modifier actually moves all of them.
    Game& game = Game::getInstance();
    const std::string original = game.getDifficulty();
    game.setDifficulty("hard");
    const float scale = game.difficulty().enemySpeedScale();

    int unscaled = 0;
    for (const auto& [name, type] : roster) {
        auto entity = EntityFactory::create(type, {100.0f, 100.0f});
        auto* enemy = dynamic_cast<Enemy*>(entity.get());
        if (!enemy) continue;
        const float before = enemy->getSpeed();
        enemy->applySpeedScale(scale);
        if (enemy->getSpeed() <= before) {
            std::cout << "        not scaled: " << name << "\n";
            ++unscaled;
        }
    }
    check(unscaled == 0, "and Hard speeds up every one of them, not just four");
    game.setDifficulty(original);
}

void testMegaShrinksBackDown() {
    section("gameplay  leaving Mega restores the previous size");

    Mario mario({100.0f, 100.0f});
    const sf::Vector2f smallSize = mario.getTargetSize();

    mario.powerUp(0); // Super
    const sf::Vector2f superSize = mario.getTargetSize();
    check(superSize.y > smallSize.y, "Super is taller than Small");

    mario.powerUp(5); // Mega
    const sf::Vector2f megaSize = mario.getTargetSize();
    check(megaSize.y > superSize.y, "Mega is bigger again");
    check(mario.getBoundingBox().height == megaSize.y,
          "and the collision box grew with the sprite, not just the sprite");
    check(megaSize.y == 128.0f, "Mega is 4 tiles tall, as the SPEC says");

    // It used to be a 128x128 square. Nothing the player can be is square, so
    // the sprite aspect-fitted to ~39px inside a 128px box and the player
    // collided with things 45px to either side of where they appeared.
    const float megaAspect = megaSize.x / megaSize.y;
    const float superAspect = superSize.x / superSize.y;
    check(std::abs(megaAspect - superAspect) < 0.01f,
          "and keeps the shape of the form it wraps rather than becoming a square");

    // Swapping the base form under Mega is what made this visible: a Fire
    // Flower while giant produced a stretched figure adrift in its own hitbox.
    mario.powerUp(1); // FireFlower, underneath the active Mega
    check(mario.getTargetSize() == megaSize,
          "collecting a power-up while Mega keeps the giant size proportional");

    // The existing Mega test only checked the state type — a size that never
    // came back down would have passed it.
    for (int i = 0; i < 12 * 60; ++i) mario.update(1.0f / 60.0f);
    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "the decorator retires");
    check(mario.getTargetSize() == superSize, "and the size comes back down to Super");
    check(mario.getBoundingBox().height == superSize.y, "collision box too");
}

void testRebindingCannotStrandThePlayer() {
    section("7.8  a rebind swaps controls rather than orphaning one");

    InputManager& input = InputManager::getInstance();
    input.resetBindingsToDefaults(0);
    check(input.getBoundKeyName("left") == "A", "defaults put left on A");
    check(input.getBoundKeyName("jump") == "W", "and jump on W");

    // Binding jump to A used to silently unbind left, and the only way back was
    // editing config.json by hand.
    input.applyBindings({{"jump", "A"}}, 0);
    check(input.getBoundKeyName("jump") == "A", "jump takes A");
    check(input.getBoundKeyName("left") == "W",
          "and left takes the key jump vacated instead of being left unbound");
    check(input.getActionForKey("A") == "jump", "the key reports its new owner");

    const auto defaults = input.resetBindingsToDefaults(0);
    check(input.getBoundKeyName("left") == "A" && input.getBoundKeyName("jump") == "W",
          "and RESET CONTROLS puts everything back");
    check(defaults.size() >= 7, "returning the map so the caller can persist what it applied");
}

void testSavesResolveToOneDirectory() {
    section("persistence  every save file resolves to the same directory");

    const std::string dir = Serializer::saveDirectory();
    check(!dir.empty(), "a save directory is chosen");

    // Bare relative paths meant the file you got depended on the working
    // directory: launching from build/ read build/saves/config.json while
    // launching from the source root read saves/config.json. A key rebind saved
    // in one was invisible from the other, which is how a player ends up unable
    // to move with a config file that looks correct.
    check(!std::filesystem::exists(std::filesystem::path(dir).parent_path() / "CMakeCache.txt"),
          "and it is never inside a CMake build tree");
}


void testObjectPoolRecyclesInsteadOfAllocating() {
    section("10.1  the object pool stops allocating once its working set is reached");

    ObjectPool<Fireball> pool;
    check(pool.constructedCount() == 0 && pool.freeCount() == 0, "a fresh pool holds nothing");

    // Two on screen at once is the cap PlayingState enforces, so the working set
    // is two — however many shots are fired.
    std::vector<std::unique_ptr<Fireball>> live;
    for (int shot = 0; shot < 50; ++shot) {
        live.push_back(pool.acquire(sf::Vector2f{0.0f, 0.0f}, sf::Vector2f{200.0f, 0.0f}));
        if (live.size() > 2) {
            pool.release(std::move(live.front()));
            live.erase(live.begin());
        }
    }
    for (auto& fireball : live) pool.release(std::move(fireball));

    check(pool.constructedCount() <= 3,
          "fifty shots built at most three objects (built " +
          std::to_string(pool.constructedCount()) + ")");
    check(pool.recycledCount() >= 45, "the rest came off the free list");
    check(pool.freeCount() <= 3, "and the free list stays the size of the working set");
}

void testPooledObjectsComeBackFresh() {
    section("10.1  a recycled object is reset, not handed back spent");

    ObjectPool<Fireball> pool;
    auto first = pool.acquire(sf::Vector2f{10.0f, 20.0f}, sf::Vector2f{200.0f, 0.0f});
    Fireball* raw = first.get();

    // Spend it completely: four bounces start the burst, and the burst ends it.
    for (int i = 0; i < 4; ++i) first->bounce();
    for (int i = 0; i < 30; ++i) first->update(1.0f / 60.0f);
    check(!first->isActive() && first->getBouncesLeft() <= 0, "the first shot is spent");

    pool.release(std::move(first));
    auto second = pool.acquire(sf::Vector2f{99.0f, 5.0f}, sf::Vector2f{-200.0f, 0.0f});

    check(second.get() == raw, "the same object comes back — that is the point");
    check(second->isActive(), "and it is alive again");
    check(second->getBouncesLeft() == 4, "with its bounces restored");
    check(!second->isImpacting(),
          "and not mid-burst — the animator survives recycling, so a shot that "
          "was not rewound would spawn already exploding");
    check(second->getPosition().x == 99.0f, "at the position it was asked for");
}

void testPoolWillNotGrowWithoutBound() {
    section("10.1  the free list is capped — a pool that never frees is a leak");

    ObjectPool<Fireball> pool;
    pool.setMaxRetained(4);
    for (int i = 0; i < 40; ++i) {
        pool.release(pool.acquire(sf::Vector2f{0.0f, 0.0f}, sf::Vector2f{0.0f, 0.0f}));
    }
    check(pool.freeCount() <= 4, "the free list respects its cap");
}


void testEveryWritableTypeRoundTrips() {
    section("persistence  every entity the game can save parses back to itself");

    // getTypeName() is what LevelLoader writes into a level file, and
    // parseEntityTypeName is what reads it. Five entities never overrode
    // getTypeName, so they wrote "unknown" — which parsed back as a Goomba.
    // Saving from the map editor silently turned Bullet Bills, Chain Chomps,
    // Hidden Blocks, Ice Blocks and Conveyor Belts into Goombas.
    int broken = 0;
    for (int i = 0; i < 40; ++i) {
        const EntityType type = static_cast<EntityType>(i);
        auto entity = EntityFactory::create(type, {0.0f, 0.0f});
        if (!entity) continue;

        const std::string name = entity->getTypeName();
        if (SerializationUtils::parseEntityTypeName(name) != type) {
            std::cout << "        \"" << name << "\" does not parse back to its own type\n";
            ++broken;
        }
    }
    check(broken == 0, "every constructible type survives a name round-trip");
}

void testEntityConfigDrivesTuning() {
    section("10.2  entities.json is read, and covers the whole roster");

    EntityConfig::reload();
    check(EntityConfig::entryCount() >= 13,
          "the file covers all 13 enemies (it had 3, and nothing read it)");

    const EntityConfigEntry* goomba = EntityConfig::find("goomba");
    check(goomba != nullptr, "and is keyed by the name entities report");
    if (!goomba) return;
    check(goomba->speed > 0.0f && goomba->score > 0, "with real values in it");

    // Seeded from the code, so adopting the file changed no behaviour. This is
    // the check that catches the two drifting apart again.
    auto configured = EntityFactory::create(EntityType::Goomba, {0.0f, 0.0f});
    auto raw = EntityFactory::createUnconfigured(EntityType::Goomba, {0.0f, 0.0f});
    auto* configuredEnemy = dynamic_cast<Enemy*>(configured.get());
    auto* rawEnemy = dynamic_cast<Enemy*>(raw.get());
    check(configuredEnemy && rawEnemy, "the factory builds both ways");
    if (!configuredEnemy || !rawEnemy) return;

    check(configuredEnemy->getSpeed() == rawEnemy->getSpeed(),
          "the file agrees with the constructor on speed, so nothing shifted");
    check(configuredEnemy->getScoreValue() == rawEnemy->getScoreValue(), "and on score");

    int missing = 0;
    for (EntityType type : {EntityType::Goomba, EntityType::KoopaTroopa,
                            EntityType::KoopaParatroopa, EntityType::Boo,
                            EntityType::PiranhaPlant, EntityType::BulletBill,
                            EntityType::HammerBro, EntityType::Thwomp,
                            EntityType::ChainChomp, EntityType::Lakitu,
                            EntityType::Spiny, EntityType::Bowser, EntityType::BoomBoom}) {
        auto entity = EntityFactory::create(type, {0.0f, 0.0f});
        if (!entity) continue;
        if (!EntityConfig::find(entity->getTypeName())) {
            std::cout << "        not in entities.json: " << entity->getTypeName() << "\n";
            ++missing;
        }
    }
    check(missing == 0, "no enemy is missing from the config");
}


void testBackgroundThemesAreDistinctAndDrawable() {
    section("5.5  every level theme resolves to its own backdrop");

    BackgroundRenderer bg;
    check(bg.parseThemeName("underground") == BackgroundTheme::Underground, "themes parse");
    check(bg.parseThemeName("castle") == BackgroundTheme::Castle, "including castle");
    check(bg.parseThemeName("ice") == BackgroundTheme::Ice, "and ice");
    check(bg.parseThemeName("nonsense") == BackgroundTheme::Overworld,
          "and an unknown theme falls back to Overworld rather than drawing nothing");

    // Each theme must look different, or the field is decorative.
    std::set<std::uint32_t> skies;
    for (BackgroundTheme theme : {BackgroundTheme::Overworld, BackgroundTheme::Underground,
                                  BackgroundTheme::Castle, BackgroundTheme::Ice}) {
        bg.setTheme(theme);
        skies.insert(bg.getSkyColor().toInteger());
    }
    check(skies.size() == 4, "all four skies are distinct colours");

    // Every level has to name a theme the renderer knows, and the levels used to
    // all claim "overworld" — including the ice cavern and the castle.
    int wrong = 0;
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        const std::string& rel = LevelCatalog::pathFor(i);
        const std::string file = rel.substr(rel.find_last_of('/') + 1);
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(levelPath(file), map, data)) continue;
        if (data.theme.empty()) {
            std::cout << "        no theme: " << file << "\n";
            ++wrong;
        }
    }
    check(wrong == 0, "every campaign level names a theme");
}


void testLavaIsARealTileThatBurns() {
    section("5.10  lava exists, round-trips, and is not walkable");

    // Water has always been a tile type; lava had none at all, despite Level 3
    // being "Castle / Lava" in the SPEC and its pit being filled with water.
    check(SerializationUtils::getTileTypeName(TileType::Lava) == "lava", "lava has a name");
    check(SerializationUtils::parseTileTypeName("lava") == TileType::Lava, "and it round-trips");
    check(static_cast<int>(TileType::Lava) == 10,
          "appended after Used, so no previously-saved tile value changes meaning");

    TileMap map;
    map.initialize(8, 8);
    map.setTile(3, 3, TileType::Lava);
    check(map.getTileType(3, 3) == TileType::Lava, "a map can hold it");
    check(!TileMap::getInfo(TileType::Lava).isSolid,
          "and it is not solid — you fall into lava, you do not stand on it");

    // And the castle level actually has some, or the feature is unreachable.
    LevelData data;
    LevelLoader loader;
    TileMap castle;
    if (loader.loadLevel(levelPath("level_3.json"), castle, data)) {
        check(countTiles(castle, TileType::Lava) > 0,
              "Bowser's Castle Fortress has lava in it (it had water)");
        check(countTiles(castle, TileType::Water) == 0, "and no water left in the castle");
    } else {
        check(false, "level_3.json loads");
    }
}


void testCameraLooksAheadAndHonoursScrollModes() {
    section("4.3  the camera leads a running player and honours its scroll mode");

    const AABB level{0.0f, 0.0f, 6400.0f, 736.0f};

    Camera plain;
    plain.setBounds(level);
    plain.setLookahead(0.0f);
    plain.snapTo({2000.0f, 400.0f});
    for (int i = 0; i < 240; ++i) plain.follow({2000.0f, 400.0f}, {300.0f, 0.0f}, 1.0f / 60.0f);
    const float plainX = plain.getPosition().x;

    Camera leading;
    leading.setBounds(level);
    leading.setLookahead(140.0f);
    leading.snapTo({2000.0f, 400.0f});
    for (int i = 0; i < 240; ++i) leading.follow({2000.0f, 400.0f}, {300.0f, 0.0f}, 1.0f / 60.0f);
    check(leading.getPosition().x > plainX + 50.0f,
          "running right pushes the camera ahead of the player");

    Camera trailing;
    trailing.setBounds(level);
    trailing.setLookahead(140.0f);
    trailing.snapTo({2000.0f, 400.0f});
    for (int i = 0; i < 240; ++i) trailing.follow({2000.0f, 400.0f}, {-300.0f, 0.0f}, 1.0f / 60.0f);
    check(trailing.getPosition().x < plainX - 50.0f, "and running left pulls it back");

    // Locked: a boss arena clamps through its bounds, so chasing only fights it.
    Camera locked;
    locked.setBounds(level);
    locked.snapTo({1000.0f, 400.0f});
    locked.setScrollMode(Camera::ScrollMode::Locked);
    for (int i = 0; i < 120; ++i) locked.follow({5000.0f, 400.0f}, {300.0f, 0.0f}, 1.0f / 60.0f);
    check(locked.getPosition().x == 1000.0f, "a locked camera ignores the target entirely");

    Camera horizontal;
    horizontal.setBounds(level);
    horizontal.snapTo({1000.0f, 400.0f});
    horizontal.setScrollMode(Camera::ScrollMode::Horizontal);
    for (int i = 0; i < 120; ++i) horizontal.follow({1400.0f, 50.0f}, {300.0f, 0.0f}, 1.0f / 60.0f);
    check(horizontal.getPosition().y == 400.0f, "horizontal mode does not follow vertically");
    check(horizontal.getPosition().x > 1000.0f, "but still follows horizontally");
}


void testColorblindModeIsActuallyConsumed() {
    section("11.4  the colourblind setting changes what is drawn");

    Game& game = Game::getInstance();
    const bool original = game.getColorblindMode();

    game.setColorblindMode(false);
    const sf::Color standardPlayer = ColorPalette::get(ColorPalette::Role::Player);
    const sf::Color standardEnemy  = ColorPalette::get(ColorPalette::Role::Enemy);
    check(standardPlayer.g > standardPlayer.r && standardEnemy.r > standardEnemy.g,
          "the standard minimap palette is the green/red pair that needed fixing");

    game.setColorblindMode(true);
    check(ColorPalette::isColorblindModeActive(), "the flag reaches the palette at all");
    const sf::Color cbPlayer = ColorPalette::get(ColorPalette::Role::Player);
    const sf::Color cbEnemy  = ColorPalette::get(ColorPalette::Role::Enemy);
    check(cbPlayer != standardPlayer || cbEnemy != standardEnemy,
          "and switching it changes the colours — before this it changed nothing at all");

    // Blue against vermilion: separable by hue under every common type of colour
    // blindness, and by brightness even in greyscale.
    auto luminance = [](sf::Color c) {
        return 0.2126f * static_cast<float>(c.r) + 0.7152f * static_cast<float>(c.g)
             + 0.0722f * static_cast<float>(c.b);
    };
    check(std::abs(luminance(cbPlayer) - luminance(cbEnemy)) > 20.0f,
          "player and enemy differ in brightness, not only in hue");

    std::set<std::uint32_t> seen;
    for (ColorPalette::Role role : {ColorPalette::Role::Player, ColorPalette::Role::Enemy,
                                    ColorPalette::Role::Item, ColorPalette::Role::Block,
                                    ColorPalette::Role::Hazard}) {
        seen.insert(ColorPalette::get(role).toInteger());
    }
    check(seen.size() == 5, "all five gameplay roles are distinct colours");

    game.setColorblindMode(original);
}


void testNewGamePlusEscalatesAndKeepsUnlocks() {
    section("11.3  New Game+ resets the campaign without wiping what was earned");

    const std::string path = Serializer::saveDirectory() + "/progress.json";
    const std::string backup = path + ".metabak";
    const bool hadExisting = std::filesystem::exists(path);
    if (hadExisting) std::filesystem::rename(path, backup);
    CampaignProgress::reset();

    check(MetaGame::newGamePlusLevel() == 0, "a fresh profile is a first playthrough");
    check(MetaGame::enemySpeedMultiplier() == 1.0f, "with no speed bonus");
    check(MetaGame::newGamePlusLabel().empty(), "and nothing to show in the menu");

    // Clear a level and collect coins, then finish the campaign.
    CampaignProgress::recordLevelCleared(0, {true, true, false});
    check(CampaignProgress::isUnlocked(1), "1-2 is unlocked");

    MetaGame::advanceNewGamePlus();
    check(MetaGame::newGamePlusLevel() == 1, "finishing the campaign advances the cycle");
    check(MetaGame::enemySpeedMultiplier() > 1.0f, "and enemies get faster");
    check(MetaGame::newGamePlusLabel() == "NEW GAME+", "with a label for the menu");

    // The point of NG+ is that the campaign resets and the meta-progress does not.
    check(!CampaignProgress::isUnlocked(1), "the campaign is locked back to 1-1");
    check(CampaignProgress::totalStarCoins() == 2,
          "but the star coins already found are kept — otherwise it is just a wipe");

    // The escalation is capped: past roughly 1.6x an enemy moves more than its
    // own width per frame and tunnels through the collision grid.
    for (int i = 0; i < 12; ++i) MetaGame::advanceNewGamePlus();
    check(MetaGame::enemySpeedMultiplier() <= 1.65f,
          "and the multiplier is capped, so enemies cannot outrun collision");

    CampaignProgress::reset();
    if (hadExisting) std::filesystem::rename(backup, path);
}

void testDailyChallengeIsTheSameForEveryone() {
    section("11.3  the daily challenge is reproducible from its date");

    const unsigned int today = MetaGame::dailySeed(2026, 8, 20);
    check(today == MetaGame::dailySeed(2026, 8, 20),
          "the same date gives the same seed — otherwise it is not a shared challenge");
    check(today != MetaGame::dailySeed(2026, 8, 21), "and a different date gives a different one");
    check(today != MetaGame::dailySeed(2026, 9, 20), "including a different month");
    check(today != 0u,
          "and never zero, which MapGenerator reads as 'pick a random seed' — that "
          "would make the daily challenge different on every launch");

    const MapGeneratorConfig a = MetaGame::dailyChallengeConfig(today);
    const MapGeneratorConfig b = MetaGame::dailyChallengeConfig(today);
    check(a.seed == b.seed && a.pitProbability == b.pitProbability
          && a.enemySpawnRate == b.enemySpawnRate,
          "and the whole generator config is derived from it, not just the seed");
    check(MetaGame::dailyChallengeConfig(MetaGame::dailySeed(2026, 8, 21)).seed != a.seed,
          "tomorrow is a different level");
}


void testDebugConsoleDispatchesCommands() {
    section("10.4  the debug console parses and dispatches");

    DebugConsole& console = DebugConsole::getInstance();
    console.init();
    console.clearOutput();

    const std::vector<std::string> names = console.commandNames();
    check(names.size() >= 8, "the built-in command set is registered");
    check(std::find(names.begin(), names.end(), "help") != names.end(), "including help");

    const std::string helpOutput = console.submit("help");
    check(helpOutput.find("give") != std::string::npos, "help lists the other commands");

    // A typo must report rather than do something surprising.
    const std::string unknown = console.submit("flibbertigibbet");
    check(unknown.find("unknown command") != std::string::npos, "an unknown command says so");

    // Commands that need a player say so instead of dereferencing null.
    Game::getInstance().setPlayer(nullptr);
    check(console.submit("give star").find("no active player") != std::string::npos,
          "and a command needing a player reports it rather than crashing");

    // With a player, the effect is real.
    Mario mario({100.0f, 100.0f});
    Game::getInstance().setPlayer(&mario);

    console.submit("give mushroom");
    check(dynamic_cast<SuperState*>(mario.getBaseState()) != nullptr,
          "give mushroom actually powers the player up");

    console.submit("lives 7");
    check(mario.getLives() == 7, "lives sets the life count");

    console.submit("tp 640 320");
    check(mario.getPosition().x == 640.0f && mario.getPosition().y == 320.0f, "tp moves the player");

    const std::string godOn = console.submit("god");
    check(godOn.find("on") != std::string::npos && mario.getInvincibilityTimer() > 9000.0f,
          "god toggles invincibility on");
    console.submit("god");
    check(mario.getInvincibilityTimer() <= 0.0f, "and off again");

    // parseEntityTypeName falls back to Goomba, so an unrecognised name would
    // otherwise silently spawn one.
    check(console.submit("spawn wumpus").find("unknown entity") != std::string::npos,
          "spawn rejects an unknown entity rather than quietly making a goomba");

    const std::string difficulty = console.submit("difficulty hard");
    check(difficulty.find("hard") != std::string::npos, "difficulty takes effect");
    console.submit("difficulty normal");

    console.submit("clear");
    check(console.getOutput().empty(), "clear empties the output");

    Game::getInstance().setPlayer(nullptr);
}


void testReplayRecordsThinsAndPlaysBack() {
    section("10.3  replays record, thin out, persist and play back");

    ReplayRecorder& replay = ReplayRecorder::getInstance();
    replay.clear();

    check(!replay.isRecording() && replay.frameCount() == 0, "a cleared recorder is empty");
    check(!replay.startPlayback(), "and refuses to play nothing");

    replay.startRecording("1-1");
    check(replay.isRecording(), "recording starts");

    // Offered every frame, kept every Nth: a snapshot per frame would make a
    // three-minute level tens of megabytes.
    const int offered = 600;
    for (int i = 0; i < offered; ++i) {
        GameSnapshot frame;
        frame.levelTimer = 300.0f - static_cast<float>(i) * 0.1f;
        frame.playerState.position = {static_cast<float>(i), 100.0f};
        frame.playerState.lives = 3;
        frame.entityStates.push_back({7u, {static_cast<float>(i) * 2.0f, 50.0f}, {1.0f, 0.0f}, true});
        replay.record(frame);
    }
    const std::size_t kept = replay.frameCount();
    check(kept > 0, "frames are kept");
    check(kept <= static_cast<std::size_t>(offered / ReplayRecorder::kFrameInterval) + 1,
          "but thinned to roughly one in " + std::to_string(ReplayRecorder::kFrameInterval) +
          " (kept " + std::to_string(kept) + " of " + std::to_string(offered) + ")");

    replay.stopRecording();
    check(!replay.isRecording(), "recording stops");

    // Round-trip through disk.
    const std::string dir = Serializer::saveDirectory() + "/replays";
    check(replay.save("regression_test"), "a replay saves");
    replay.clear();
    check(replay.frameCount() == 0, "and the recorder can be emptied");
    check(replay.load("regression_test"), "then loaded back");
    check(replay.frameCount() == kept, "with every frame intact");
    check(replay.levelName() == "1-1", "and the level it was recorded on");

    // Playback walks the frames once and stops.
    check(replay.startPlayback(), "playback starts");
    std::size_t played = 0;
    const GameSnapshot* first = replay.advance();
    check(first != nullptr, "the first frame comes back");
    if (first) {
        check(!first->entityStates.empty() && first->entityStates[0].id == 7u,
              "with its entity states, keyed by id rather than index");
    }
    while (replay.advance() != nullptr) ++played;
    check(played + 1 == kept, "and it walks every frame exactly once");
    check(!replay.isPlaying(), "then stops on its own");

    check(!replay.load("no_such_replay"), "loading a missing replay fails cleanly");

    std::error_code ec;
    std::filesystem::remove(dir + "/regression_test.json", ec);
    replay.clear();
}


void testTwoPlayerBindingsAreIndependent() {
    section("11.1  Player 2 has its own controls, separate from Player 1");

    InputManager& input = InputManager::getInstance();
    input.resetBindingsToDefaults(0);
    input.resetBindingsToDefaults(1);

    // Player 2's bindings have existed since InputManager was written and
    // nothing had ever registered a second player against them.
    check(input.getBoundKeyName("left", 0) == "A", "P1 moves on A");
    check(input.getBoundKeyName("left", 1) == "Left", "P2 moves on the arrow keys");
    check(input.getBoundKeyName("jump", 0) != input.getBoundKeyName("jump", 1),
          "and the two never share a jump key");

    // Rebinding one player must not disturb the other.
    input.applyBindings({{"jump", "J"}}, 0);
    check(input.getBoundKeyName("jump", 0) == "J", "P1 rebinds");
    check(input.getBoundKeyName("jump", 1) == "Up", "and P2 is untouched");

    Mario p1({0.0f, 0.0f});
    Luigi p2({48.0f, 0.0f});
    input.registerPlayer(&p1, 0);
    input.registerPlayer(&p2, 1);
    check(input.getPlayer(0) == &p1 && input.getPlayer(1) == &p2,
          "both players register against their own slot");
    check(input.getPlayer(0) != input.getPlayer(1), "and they are genuinely two players");

    input.resetBindingsToDefaults(0);
    input.registerPlayer(nullptr, 0);
    input.registerPlayer(nullptr, 1);
}


void testEventBusSurvivesHandlersThatMutateIt() {
    section("X-7  the event bus does not copy its subscribers, and survives mutation");

    EventBus& bus = EventBus::getInstance();

    // A handler unsubscribing itself mid-delivery used to be survivable only
    // because publish() copied the whole subscriber vector — a std::function
    // heap allocation per subscriber, on every coin, stomp and jump.
    int selfCancelling = 0;
    EventBus::SubscriptionId id = 0;
    id = bus.subscribe(EventType::CoinCollected, [&](const GameEvent&) {
        ++selfCancelling;
        bus.unsubscribe(id);
    });

    int bystander = 0;
    const auto bystanderId = bus.subscribe(EventType::CoinCollected,
                                           [&bystander](const GameEvent&) { ++bystander; });

    bus.publish({EventType::CoinCollected, 1});
    check(selfCancelling == 1, "a handler that unsubscribes itself still runs once");
    check(bystander == 1, "and the subscriber after it still receives the event");

    bus.publish({EventType::CoinCollected, 1});
    check(selfCancelling == 1, "and does not run again");
    check(bystander == 2, "while the bystander does");

    // Subscribing during delivery must not deliver the in-flight event to the
    // new subscriber, which would be an easy accidental infinite loop.
    int lateJoiner = 0;
    EventBus::SubscriptionId lateId = 0;
    const auto joinerId = bus.subscribe(EventType::BlockBroken, [&](const GameEvent&) {
        if (lateId == 0) {
            lateId = bus.subscribe(EventType::BlockBroken,
                                   [&lateJoiner](const GameEvent&) { ++lateJoiner; });
        }
    });
    bus.publish({EventType::BlockBroken, 0});
    check(lateJoiner == 0, "a subscriber added during delivery does not receive that event");
    bus.publish({EventType::BlockBroken, 0});
    check(lateJoiner == 1, "but does receive the next one");

    bus.unsubscribe(bystanderId);
    bus.unsubscribe(joinerId);
    if (lateId != 0) bus.unsubscribe(lateId);
}

void testScopedSubscriptionCannotBeForgotten() {
    section("X-7  a scoped subscription unsubscribes itself");

    EventBus& bus = EventBus::getInstance();
    int hits = 0;

    {
        EventBus::ScopedSubscription token(EventType::PlayerDamaged,
                                           [&hits](const GameEvent&) { ++hits; });
        check(token.active(), "the token holds a live subscription");
        bus.publish({EventType::PlayerDamaged, 0});
        check(hits == 1, "which receives events");
    }

    // The whole point: leaving the scope cancels it. A raw SubscriptionId that
    // its owner forgets to release leaves a callback pointing into a destroyed
    // object, which fires the next time that event is published.
    bus.publish({EventType::PlayerDamaged, 0});
    check(hits == 1, "and leaving the scope cancels it without anyone remembering to");

    // Move-only, so two owners cannot cancel the same id twice.
    EventBus::ScopedSubscription first(EventType::PlayerDamaged,
                                       [&hits](const GameEvent&) { ++hits; });
    EventBus::ScopedSubscription second = std::move(first);
    check(!first.active() && second.active(), "moving transfers ownership");
    bus.publish({EventType::PlayerDamaged, 0});
    check(hits == 2, "and the moved-to token still works");
}


void testHeldKeysComeFromEventsNotTheOs() {
    section("input  held keys are tracked from events, not polled from the OS");

    InputManager& input = InputManager::getInstance();
    input.resetBindingsToDefaults(0);
    input.clearHeldKeys();

    auto keyDown = [](sf::Keyboard::Key code) {
        sf::Event::KeyPressed pressed;
        pressed.code = code;
        return sf::Event(pressed);
    };
    auto keyUp = [](sf::Keyboard::Key code) {
        sf::Event::KeyReleased released;
        released.code = code;
        return sf::Event(released);
    };

    check(!input.isHeld(sf::Keyboard::Key::D), "nothing is held to begin with");
    input.noteKeyEvent(keyDown(sf::Keyboard::Key::D));
    check(input.isHeld(sf::Keyboard::Key::D), "a press marks the key held");
    input.noteKeyEvent(keyUp(sf::Keyboard::Key::D));
    check(!input.isHeld(sf::Keyboard::Key::D), "and a release clears it");

    // The bug this replaces: held actions asked sf::Keyboard::isKeyPressed,
    // which reads global OS key state. On macOS that needs Input Monitoring
    // permission and silently returns false without it — so jump worked (a press
    // mapping, driven by events) while walking did not (a hold mapping, driven
    // by polling), with nothing logged anywhere.
    Mario mario({100.0f, 100.0f});
    input.registerPlayer(&mario, 0);

    input.clearHeldKeys();
    input.update(mario);
    check(!mario.isMoveRightRequested(), "with no key held, no movement is requested");

    input.noteKeyEvent(keyDown(sf::Keyboard::Key::D));
    input.update(mario);
    check(mario.isMoveRightRequested(),
          "holding D drives MoveRight through the event-tracked state");

    // A release that arrives while another window has focus never reaches us,
    // so focus loss has to clear everything or the key sticks down forever.
    input.noteKeyEvent(keyDown(sf::Keyboard::Key::A));
    check(input.isHeld(sf::Keyboard::Key::A), "A is held");
    input.noteKeyEvent(sf::Event(sf::Event::FocusLost{}));
    check(!input.isHeld(sf::Keyboard::Key::A) && !input.isHeld(sf::Keyboard::Key::D),
          "losing focus releases everything, so no key sticks");

    input.registerPlayer(nullptr, 0);
    input.clearHeldKeys();
}

} // namespace

// ---------------------------------------------------------------------------
// Reported by playtest: bosses fall out of the level, enemies walk into pits,
// the pipe to a sub-level "diverges", fireballs do not kill, and a picked-up
// shell can never be put down.
// ---------------------------------------------------------------------------

// A fireball had a passing unit test that called resolveFireballVsEnemy directly
// and only asserted the *fireball* died. Nothing checked the enemy, and nothing
// went through the dispatch the game actually uses. This drives the whole
// pipeline: PhysicsEngine broadphase -> resolveEntityVsEntity -> category
// dispatch -> Fireball::onHitEnemy.
void testFireballKillsThroughTheRealPipeline() {
    section("playtest  a fireball fired into a Goomba actually kills it");

    TileMap map;
    map.initialize(40, 20);
    for (int x = 0; x < 40; ++x) map.setTile(x, 18, TileType::Ground);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Goomba>(sf::Vector2f{300.0f, 544.0f}));
    entities.push_back(std::make_unique<Fireball>(sf::Vector2f{200.0f, 548.0f},
                                                 sf::Vector2f{350.0f, 0.0f}));
    Goomba* goomba = static_cast<Goomba*>(entities[0].get());

    PhysicsEngine physics;
    bool goombaDied = false;
    for (int frame = 0; frame < 60 && !goombaDied; ++frame) {
        physics.update(entities, map, 1.0f / 60.0f);
        for (auto& e : entities) if (e) e->update(1.0f / 60.0f);
        goombaDied = goomba->isDeadOrDying();
    }

    check(goombaDied, "the fireball travelled into the Goomba and killed it");

    // And the same pipeline must NOT let a hammer do it — the category dispatch
    // is shared, so this is the pair that keeps damagesEnemies() honest.
    std::vector<std::unique_ptr<Entity>> hammerScene;
    hammerScene.push_back(std::make_unique<Goomba>(sf::Vector2f{300.0f, 544.0f}));
    hammerScene.push_back(std::make_unique<Hammer>(sf::Vector2f{200.0f, 548.0f},
                                                  sf::Vector2f{350.0f, 0.0f}));
    Goomba* survivor = static_cast<Goomba*>(hammerScene[0].get());
    for (int frame = 0; frame < 60; ++frame) {
        physics.update(hammerScene, map, 1.0f / 60.0f);
        for (auto& e : hammerScene) if (e) e->update(1.0f / 60.0f);
    }
    check(!survivor->isDeadOrDying(), "a thrown hammer passing through does not");
}

// Touching any unflipped Koopa used to pick it up, so a patrolling Koopa could
// not hurt the player at all and a shell someone had just kicked was harmless.
void testKoopaContactFollowsTheSeriesRules() {
    section("playtest  Koopa contact: walkers hurt, resting shells are kicked or carried");

    CollisionResolver resolver;
    CollisionInfo side;
    side.collided = true;
    side.normal = sf::Vector2f(1.0f, 0.0f);

    auto sideContact = [&resolver, &side](Mario& player, KoopaTroopa& koopa) {
        // Side contact: level with the Koopa and moving, never descending onto it.
        player.setPosition({koopa.getPosition().x - 20.0f, koopa.getPosition().y});
        player.setVelocity({80.0f, 0.0f});
        resolver.resolvePlayerVsEnemy(player, koopa, side);
    };

    {   // A walking Koopa hurts you, like every other enemy.
        Mario player({0.0f, 0.0f});
        KoopaTroopa koopa({200.0f, 200.0f});
        const int livesBefore = player.getLives();
        sideContact(player, koopa);
        check(player.getInvincibilityTimer() > 0.0f || player.getLives() < livesBefore,
              "walking into a live Koopa damages the player rather than handing it over");
        check(player.getHeldEntity() == nullptr,
              "and does not put the Koopa in the player's hands");
    }

    {   // A resting shell you simply walk into gets kicked away.
        Mario player({0.0f, 0.0f});
        KoopaTroopa koopa({200.0f, 200.0f});
        koopa.onStomped();                       // walking -> ShellIdle
        check(koopa.getState() == KoopaState::ShellIdle, "a stomp shells the Koopa");
        sideContact(player, koopa);
        check(koopa.getState() == KoopaState::ShellKicked,
              "walking into a resting shell kicks it");
        check(std::abs(koopa.getVelocity().x) > 100.0f, "and it actually moves off");
    }

    {   // Holding run picks it up instead.
        Mario player({0.0f, 0.0f});
        KoopaTroopa koopa({200.0f, 200.0f});
        koopa.onStomped();
        RunCommand run;
        run.execute(player);   // the same command the run key is bound to
        sideContact(player, koopa);
        check(koopa.getState() == KoopaState::ShellHeld,
              "holding run picks the shell up instead of kicking it");
        check(player.getHeldEntity() == &koopa, "and the player is carrying it");

        // The whole point of carrying: you can put it down again. Nothing ever
        // cleared m_heldEntity, so a shell was carried for the rest of the level.
        player.setFacingRight(true);
        const bool threw = player.throwHeldEntity();
        check(threw && player.getHeldEntity() == nullptr, "the fire button throws it");
        check(koopa.getState() == KoopaState::ShellKicked && koopa.getVelocity().x > 0.0f,
              "and it flies off in the direction the player is facing");
    }

    {   // A shell already sliding hurts you.
        Mario player({0.0f, 0.0f});
        KoopaTroopa koopa({200.0f, 200.0f});
        koopa.onStomped();
        koopa.kick({300.0f, 0.0f});

        // Straight after the kick it is harmless to whoever launched it — it is
        // still overlapping them. Check that first, so the grace window is not
        // silently what makes the damage check below pass or fail.
        const int livesAtKick = player.getLives();
        sideContact(player, koopa);
        check(player.getInvincibilityTimer() <= 0.0f && player.getLives() == livesAtKick,
              "a shell you just kicked does not hurt you on the way out");

        // Once the grace expires it is a hazard like any other.
        for (int i = 0; i < 40; ++i) koopa.update(1.0f / 60.0f);
        const int livesBefore = player.getLives();
        sideContact(player, koopa);
        check(player.getInvincibilityTimer() > 0.0f || player.getLives() < livesBefore,
              "running into a shell that is already sliding damages the player");
    }

    {   // Being hurt while carrying puts the shell down rather than dragging it.
        Mario player({0.0f, 0.0f});
        KoopaTroopa koopa({200.0f, 200.0f});
        koopa.onStomped();
        koopa.pickUp(&player);
        player.holdEntity(&koopa);
        player.takeDamage(1);
        check(player.getHeldEntity() == nullptr && koopa.getState() == KoopaState::ShellIdle,
              "taking damage drops the shell instead of carrying it around");
    }
}

// "Many entities tend to move to the void": only the red variants turned at a
// ledge, so most of the cast walked off the first drop and was gone before the
// player arrived.
void testGroundPatrolsTurnAtLedgesAndHazards() {
    section("playtest  ground patrols turn at a ledge instead of walking into the void");

    TileMap map;
    map.initialize(30, 20);
    // Floor from x=0..9, then a pit, so an enemy walking right must turn at x=9.
    for (int x = 0; x <= 9; ++x) map.setTile(x, 18, TileType::Ground);
    // A lava surface further left: "not Empty" but nothing to stand on.
    for (int x = 12; x <= 20; ++x) map.setTile(x, 18, TileType::Lava);
    Game::getInstance().setTileMap(&map);

    struct Case { const char* name; std::unique_ptr<Enemy> enemy; };
    std::vector<Case> cases;
    cases.push_back({"Goomba",       std::make_unique<Goomba>(sf::Vector2f{32.0f, 544.0f})});
    cases.push_back({"green Koopa",  std::make_unique<KoopaTroopa>(sf::Vector2f{32.0f, 544.0f}, false)});
    cases.push_back({"Spiny",        std::make_unique<Spiny>(sf::Vector2f{32.0f, 544.0f})});

    for (auto& c : cases) {
        Enemy* enemy = c.enemy.get();
        // Aim it at the pit.
        if (auto* patrol = dynamic_cast<PatrolStrategy*>(
                const_cast<IMovementStrategy*>(enemy->getStrategy()))) {
            patrol->setMovingRight(true);
        }

        bool fellOff = false;
        for (int frame = 0; frame < 600; ++frame) {
            // Held grounded on purpose: the ledge test, not gravity, is what is
            // under examination here.
            enemy->setGrounded(true);
            enemy->update(1.0f / 60.0f);
            enemy->setPosition({enemy->getPosition().x + enemy->getVelocity().x / 60.0f,
                                enemy->getPosition().y});
            if (enemy->getPosition().x > 10.0f * Constants::TILE_SIZE ||
                enemy->getPosition().x < -Constants::TILE_SIZE) {
                fellOff = true;
                break;
            }
        }
        check(!fellOff, std::string("a ") + c.name + " turns back at the ledge");
    }

    Game::getInstance().setTileMap(nullptr);
}

// The boss was authored inside a solid 5x5 pillar with a one-tile slot in it.
void testBossesStandOnOpenArenaFloor() {
    section("playtest  each boss stands on its arena floor, not inside a block");

    struct Case { const char* file; const char* bossType; };
    const Case cases[] = {
        {"level_2.json", "boom_boom"},
        {"level_3.json", "bowser"},
    };

    for (const Case& c : cases) {
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(levelPath(c.file), map, data)) {
            check(false, std::string("could not load ") + c.file);
            continue;
        }

        Boss* boss = nullptr;
        for (const auto& e : data.entities) {
            if (auto* b = dynamic_cast<Boss*>(e.get())) { boss = b; break; }
        }
        if (!boss) {
            check(false, std::string(c.file) + " contains no boss");
            continue;
        }

        const AABB box = boss->getBoundingBox();
        // Nothing solid may overlap the boss's own body.
        bool embedded = false;
        for (float y = box.y; y < box.y + box.height; y += Constants::TILE_SIZE * 0.5f) {
            for (float x = box.x; x < box.x + box.width; x += Constants::TILE_SIZE * 0.5f) {
                if (solidAt(map, x, y)) embedded = true;
            }
        }
        check(!embedded, std::string(c.bossType) + " is not buried inside a solid tile");

        // And there must be ground under it within a short fall.
        bool hasFloor = false;
        const float footX = box.x + box.width * 0.5f;
        for (float y = box.y + box.height; y < box.y + box.height + 3.0f * Constants::TILE_SIZE;
             y += 4.0f) {
            if (solidAt(map, footX, y)) { hasFloor = true; break; }
        }
        check(hasFloor, std::string(c.bossType) + " has floor beneath it instead of a void");

        // The arena must be walkable rather than a wall of blocks: the boss
        // patrols it, and the player has to fight in it.
        check(boss->hasArena(), std::string(c.bossType) + " declares an arena");
        const AABB arena = boss->getArena();
        int solidAtHeadHeight = 0;
        for (float x = arena.x; x < arena.x + arena.width; x += Constants::TILE_SIZE) {
            if (solidAt(map, x, box.y)) ++solidAtHeadHeight;
        }
        check(solidAtHeadHeight == 0,
              std::string("the ") + c.bossType + " arena is clear at fighting height");
    }
}

// Entering a sub-level teleported the player to tile 104 of a 65-tile level, and
// coming back landed six tiles behind the pipe you had just left.
void testWarpPipesLandInsideTheirDestination() {
    section("playtest  every warp pipe lands the player inside the level it targets");

    const char* files[] = {
        "level_1.json", "level_1_sub.json",
        "level_2.json", "level_2_sub.json",
        "level_3.json", "level_3_sub.json",
        "bonus_1.json",
    };

    for (const char* file : files) {
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(levelPath(file), map, data)) continue;

        for (const auto& e : data.entities) {
            auto* pipe = dynamic_cast<Pipe*>(e.get());
            if (!pipe || pipe->getTargetLevel().empty()) continue;

            TileMap target;
            LevelData targetData;
            LevelLoader targetLoader;
            const std::string targetName =
                pipe->getTargetLevel().substr(pipe->getTargetLevel().find_last_of('/') + 1);
            if (!targetLoader.loadLevel(levelPath(targetName), target, targetData)) {
                check(false, std::string(file) + " warps to a level that will not load");
                continue;
            }

            const sf::Vector2f exit = pipe->getExitPosition();
            const bool inside =
                exit.x >= 0.0f && exit.x < target.getWidth() * Constants::TILE_SIZE &&
                exit.y >= 0.0f && exit.y < target.getHeight() * Constants::TILE_SIZE;
            check(inside, std::string(file) + " -> " + targetName + ": the exit is inside the map");

            if (!inside) continue;
            bool hasFloor = false;
            for (float y = exit.y; y < target.getHeight() * Constants::TILE_SIZE; y += 4.0f) {
                if (solidAt(target, exit.x, y)) { hasFloor = true; break; }
            }
            check(hasFloor, std::string(file) + " -> " + targetName + ": and has ground under it");
        }
    }
}

// The campaign listed the *_sub levels as ordinary stages. They carry no
// flagpole, so nothing in them can publish LevelComplete: finishing 1-1 advanced
// the player into "1-1 Sub" and the campaign could never move again.
void testCampaignPathContainsOnlyCompletableLevels() {
    section("playtest  every campaign level can actually be finished");

    for (int i = 0; i < LevelCatalog::count(); ++i) {
        TileMap map;
        LevelData data;
        LevelLoader loader;
        const std::string path = LevelCatalog::pathFor(i);
        const std::string name = path.substr(path.find_last_of('/') + 1);
        if (!loader.loadLevel(levelPath(name), map, data)) {
            check(false, "campaign level " + LevelCatalog::nameFor(i) + " loads");
            continue;
        }

        bool hasGoal = false;
        for (const auto& e : data.entities) {
            if (dynamic_cast<Flagpole*>(e.get())) hasGoal = true;
        }
        check(hasGoal, "campaign level " + LevelCatalog::nameFor(i) +
                       " has a flagpole, so the campaign can advance past it");
        check(name.find("_sub") == std::string::npos,
              "and " + name + " is a main-path level, not a pipe side room");
    }
}

// The cape was a costume. CapeState's enter/exit/handleInput/update were all
// empty bodies, so a Cape Feather changed the sprite and nothing else — the one
// power-up whose entire point is a behaviour had none.
void testCapeActuallyDoesSomething() {
    section("playtest  the cape glides and swings, instead of only changing the sprite");

    InputManager& input = InputManager::getInstance();
    Mario player({100.0f, 100.0f});
    input.registerPlayer(&player, 0);
    player.powerUp(2);                   // CapeFeather -> CapeState
    check(dynamic_cast<CapeState*>(player.getBaseState()) != nullptr,
          "a cape feather puts the player in CapeState");

    // --- glide -------------------------------------------------------------
    const sf::Keyboard::Key jumpKey = sf::Keyboard::Key::W;
    auto pressJump = [&input, jumpKey](bool down) {
        if (down) {
            sf::Event::KeyPressed pressed; pressed.code = jumpKey;
            input.noteKeyEvent(sf::Event(pressed));
        } else {
            sf::Event::KeyReleased released; released.code = jumpKey;
            input.noteKeyEvent(sf::Event(released));
        }
    };

    auto fallFor = [&player](int frames) {
        for (int i = 0; i < frames; ++i) {
            player.setGrounded(false);
            // Gravity, the same expression PhysicsEngine uses.
            player.setVelocity({player.getVelocity().x,
                                player.getVelocity().y +
                                    Constants::GRAVITY * Constants::GRAVITY_SCALE / 60.0f});
            player.update(1.0f / 60.0f);
        }
        return player.getVelocity().y;
    };

    pressJump(false);
    player.setVelocity({0.0f, 0.0f});
    const float freeFall = fallFor(30);

    pressJump(true);
    player.setVelocity({0.0f, 0.0f});
    const float gliding = fallFor(30);
    pressJump(false);

    check(freeFall > gliding * 2.0f,
          "holding jump while falling in the cape slows the descent sharply");
    check(gliding <= CapeState::GLIDE_FALL_SPEED + 1.0f,
          "the glide is capped at the drift speed rather than accelerating");
    check(player.isGliding(), "and the player reports gliding, so the art can follow");

    // Landing clears it, or the flag would stick for the rest of the level.
    player.setGrounded(true);
    player.update(1.0f / 60.0f);
    check(!player.isGliding(), "landing ends the glide");

    // --- spin --------------------------------------------------------------
    check(player.spinCape() && player.isSpinningCape(),
          "the fire button swings the cape instead of doing nothing");

    Goomba victim({100.0f, 100.0f});
    CollisionResolver resolver;
    CollisionInfo side;
    side.collided = true;
    side.normal = sf::Vector2f(1.0f, 0.0f);
    player.setVelocity({40.0f, 0.0f});
    resolver.resolvePlayerVsEnemy(player, victim, side);
    check(victim.isDeadOrDying(), "a spinning cape defeats an enemy it touches");

    // And it is a swing, not a permanent aura.
    for (int i = 0; i < 40; ++i) player.update(1.0f / 60.0f);
    check(!player.isSpinningCape(), "the swing ends on its own");

    // A player with no cape gets no free kills out of the same button.
    Mario plain({100.0f, 100.0f});
    check(!plain.spinCape(), "a player without the cape cannot spin");

    input.registerPlayer(nullptr, 0);
    input.clearHeldKeys();
}

// Reported as "moving platform doesn't have the sprite — find them and fix".
// Finding them by eye is the problem: setupAnimations() sets m_hasAnimation
// whether or not the frames it names exist in the atlas, and drawSprite()
// returns early on a zero-size sprite, so a wrong frame name draws *nothing* —
// not even the placeholder rectangle that would make it obvious. This asks
// every entity type in the factory what it would actually put on screen.
void testEveryEntityTypeDrawsRealArt() {
    section("playtest  every entity type draws real artwork, not nothing");

    auto playerSheet  = SpriteSheet::loadAtlas("player");
    auto enemySheet   = SpriteSheet::loadAtlas("enemy_projectile");
    auto itemSheet    = SpriteSheet::loadAtlas("item");
    auto scenerySheet = SpriteSheet::loadAtlas("world_scenery_item");
    if (!playerSheet || !enemySheet || !itemSheet || !scenerySheet) {
        check(false, "the four sprite atlases load");
        return;
    }

    struct Case { EntityType type; const char* name; };
    const Case kAll[] = {
        {EntityType::Mario, "Mario"}, {EntityType::Luigi, "Luigi"},
        {EntityType::Toad, "Toad"}, {EntityType::Peach, "Peach"},
        {EntityType::Goomba, "Goomba"}, {EntityType::KoopaTroopa, "KoopaTroopa"},
        {EntityType::KoopaParatroopa, "KoopaParatroopa"}, {EntityType::Boo, "Boo"},
        {EntityType::PiranhaPlant, "PiranhaPlant"}, {EntityType::BulletBill, "BulletBill"},
        {EntityType::HammerBro, "HammerBro"}, {EntityType::Thwomp, "Thwomp"},
        {EntityType::ChainChomp, "ChainChomp"}, {EntityType::Lakitu, "Lakitu"},
        {EntityType::Spiny, "Spiny"}, {EntityType::Hammer, "Hammer"},
        {EntityType::BossFireball, "BossFireball"}, {EntityType::Bowser, "Bowser"},
        {EntityType::BoomBoom, "BoomBoom"}, {EntityType::Mushroom, "Mushroom"},
        {EntityType::FireFlower, "FireFlower"}, {EntityType::Coin, "Coin"},
        {EntityType::Star, "Star"}, {EntityType::OneUpMushroom, "OneUpMushroom"},
        {EntityType::CapeFeather, "CapeFeather"}, {EntityType::MegaMushroom, "MegaMushroom"},
        {EntityType::MiniMushroom, "MiniMushroom"}, {EntityType::POWBlock, "POWBlock"},
        {EntityType::PSwitch, "PSwitch"}, {EntityType::Trampoline, "Trampoline"},
        {EntityType::StarCoin, "StarCoin"}, {EntityType::BrickBlock, "BrickBlock"},
        {EntityType::QuestionBlock, "QuestionBlock"}, {EntityType::Pipe, "Pipe"},
        {EntityType::Flagpole, "Flagpole"}, {EntityType::HiddenBlock, "HiddenBlock"},
        {EntityType::MovingPlatform, "MovingPlatform"},
        {EntityType::FallingPlatform, "FallingPlatform"},
        {EntityType::IceBlock, "IceBlock"}, {EntityType::ConveyorBelt, "ConveyorBelt"},
    };

    std::vector<std::string> invisible;
    for (const Case& c : kAll) {
        auto entity = EntityFactory::create(c.type, {100.0f, 100.0f});
        if (!entity) {
            invisible.push_back(std::string(c.name) + " (factory returned null)");
            continue;
        }
        Entity* e = entity.get();

        // The same routing PlayingState::wireEntityAnimations uses.
        if (auto* x = dynamic_cast<Player*>(e))          x->setupAnimations(playerSheet.get());
        else if (auto* x = dynamic_cast<Enemy*>(e))      x->setupAnimations(enemySheet.get());
        else if (auto* x = dynamic_cast<Projectile*>(e)) x->setupAnimations(enemySheet.get());
        else if (dynamic_cast<StarCoin*>(e))             static_cast<Item*>(e)->setupAnimations(scenerySheet.get());
        else if (auto* x = dynamic_cast<Item*>(e))       x->setupAnimations(itemSheet.get());
        else if (auto* x = dynamic_cast<Block*>(e))      x->setupAnimations(scenerySheet.get());

        const sf::Vector2f art = e->artworkSize();
        if (!e->hasArtwork()) {
            invisible.push_back(std::string(c.name) + " (no frame list installed)");
        } else if (art.x <= 0.0f || art.y <= 0.0f) {
            invisible.push_back(std::string(c.name) + " (names frames the atlas lacks)");
        }
    }

    for (const std::string& bad : invisible) {
        std::cout << "         -> " << bad << "\n";
    }
    check(invisible.empty(),
          "all " + std::to_string(sizeof(kAll) / sizeof(kAll[0])) +
              " entity types resolve to a real sprite");
}

// Defeating Bowser was a hard crash. His defeat sequence ends in destroy(); the
// prune step in update() then deleted him; updateBossArena() read
// m_activeBoss->isActive() through the freed pointer ninety lines later.
void testDefeatingABossDoesNotDangle() {
    section("playtest  a defeated boss does not leave a dangling pointer behind");

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<BoomBoom>(sf::Vector2f{300.0f, 300.0f}));
    Boss* boss = static_cast<Boss*>(entities[0].get());

    // Beat it, then run past the defeat sequence.
    for (int hit = 0; hit < boss->getMaxHealth() + 2; ++hit) {
        boss->onStomped();
        for (int i = 0; i < 70; ++i) boss->update(1.0f / 60.0f);
    }
    check(boss->isDefeated(), "the boss is defeated");

    // The defeat sequence ends in destroy(), which is what the prune keys off.
    for (int i = 0; i < 200 && boss->isActive(); ++i) boss->update(1.0f / 60.0f);
    check(!boss->isActive(),
          "and deactivates itself once the sequence finishes, so the prune takes it");
}

// Dying used to teleport the player to the spawn point on the frame they touched
// the pit, and — for a fatal fall — killPlayer() ran its whole body again every
// following frame, restarting the game-over fade so it never completed.
void testDeathIsASequenceThatEnds() {
    section("playtest  death falls, then respawns; the last one reaches game over");

    Mario player({100.0f, 100.0f});
    check(!player.isDying(), "a live player is not dying");
    check(player.collidesWithTiles(), "and collides with the level");

    player.beginDeathFall();
    check(player.isDying(), "beginDeathFall marks the player dying");
    check(player.getVelocity().y < 0.0f, "with an upward pop first");
    check(!player.collidesWithTiles() && !player.isCollidable(),
          "and no collision at all, so the corpse falls through the level");

    // Input and damage are both ignored on the way down.
    player.moveRight();
    player.jump();
    check(!player.isMoveRightRequested(), "a dying player ignores movement input");
    const int livesBefore = player.getLives();
    player.takeDamage(1);
    check(player.getLives() == livesBefore, "and cannot be killed twice on the way down");

    player.endDeathFall();
    check(!player.isDying() && player.collidesWithTiles(), "ending the fall restores it");
}

// Small Mario being hit by an enemy called loseLife() inside powerDown() and
// published PlayerDied, which nothing in PlayingState listened to. He lost a
// life and carried on standing there.
void testSmallMarioDyingReportsItInsteadOfSelfAccounting() {
    section("playtest  an enemy killing Small Mario reports a death, once");

    int deaths = 0;
    const auto sub = EventBus::getInstance().subscribe(
        EventType::PlayerDied, [&deaths](const GameEvent&) { ++deaths; });

    Mario player({100.0f, 100.0f});
    const int livesBefore = player.getLives();
    player.takeDamage(1);   // Small -> death

    check(deaths == 1, "the death is announced exactly once");
    check(player.getLives() == livesBefore,
          "and powerDown() no longer docks the life itself — PlayingState owns that, "
          "because only it knows about checkpoints and game over");
}

// A warp discards the Player and builds a new one. Everything not explicitly
// carried across is lost, and the power-up form was.
void testAFormSurvivesBeingRebuilt() {
    section("playtest  a power-up form survives the rebuild a warp performs");

    Mario before({100.0f, 100.0f});
    before.powerUp(1);                                   // FireFlower
    check(before.getForm() == Player::Form::Fire, "Fire Mario reports the Fire form");
    check(before.canShootFireball(), "and can shoot");

    // What loadLevelByPath does: capture, destroy, rebuild, restore.
    const Player::Form carried = before.getForm();
    Mario after({100.0f, 100.0f});
    check(after.getForm() == Player::Form::Small, "a fresh player starts Small");
    after.setForm(carried);
    check(after.getForm() == Player::Form::Fire,
          "and comes out of the pipe still holding the fire flower");
    check(after.canShootFireball(), "with the ability that goes with it");

    // Every form round-trips, not just the one that was reported.
    for (Player::Form form : {Player::Form::Small, Player::Form::Super,
                              Player::Form::Fire, Player::Form::Cape, Player::Form::Mini}) {
        Mario probe({100.0f, 100.0f});
        probe.setForm(form);
        if (probe.getForm() != form) {
            check(false, "a form failed to round-trip");
            return;
        }
    }
    check(true, "all five forms round-trip through getForm/setForm");
}

// The piranha plant sat in full view on the pipe mouth the whole time it was
// supposed to be hidden. The first attempt at this computed the visible height
// with two terms that cancelled, so it reported the plant fully visible while
// retracted — the exact state it was meant to hide.
void testPiranhaPlantHidesInsideItsPipe() {
    section("playtest  the piranha plant is invisible and harmless inside the pipe");

    PiranhaPlant plant({320.0f, 608.0f});

    // The cycle is 7s: 0-2 retracted, 2-3 emerging, 3-6 out, 6-7 retreating.
    auto runTo = [&plant](float seconds) {
        // Restart from zero each time by constructing fresh would lose the
        // strategy's timer, so step forward from wherever we are instead.
        for (int i = 0; i < static_cast<int>(seconds * 60.0f); ++i) {
            plant.update(1.0f / 60.0f);
        }
    };

    check(plant.artworkVisibleHeight() <= 0.01f,
          "at rest it shows nothing at all");
    check(!plant.isCollidable(), "and cannot bite from inside the pipe");

    runTo(4.0f);   // well into the emerged window
    check(plant.artworkVisibleHeight() > 16.0f, "once out, it is visible");
    check(plant.isCollidable(), "and dangerous");

    runTo(3.1f);   // past the 7s wrap, back to retracted
    check(plant.artworkVisibleHeight() <= 0.01f, "and it hides again on the way back down");
    check(!plant.isCollidable(), "harmless again once inside");
}

// Kicking a shell left it overlapping the kicker, and PhysicsEngine's friction
// applies to every Character — including an enemy running no strategy — so the
// shell stopped dead and the player walked back into a "moving shell".
void testAKickedShellKeepsGoingAndSparesItsKicker() {
    section("playtest  a kicked shell travels, and does not punish whoever kicked it");

    KoopaTroopa koopa({300.0f, 300.0f});
    koopa.onStomped();                       // -> ShellIdle
    koopa.kick({Constants::KOOPA_SHELL_KICK_SPEED, 0.0f});

    check(koopa.isHarmlessToKicker(),
          "immediately after the kick the shell cannot hurt the player who kicked it");

    // Friction would otherwise drag this to zero.
    for (int i = 0; i < 120; ++i) koopa.update(1.0f / 60.0f);
    check(std::abs(koopa.getVelocity().x) >= Constants::KOOPA_SHELL_KICK_SPEED - 1.0f,
          "two seconds later it is still travelling at kick speed");
    check(!koopa.isHarmlessToKicker(), "and by then it is dangerous again");
}

// --- Reported by Member B, August 2026 ------------------------------------
//
// Twelve defects found by playing the game rather than reading it. The ones with
// observable state are pinned here; the purely visual ones (hovering backdrops,
// pipe seams, the Mini sprite's proportions) were verified from captures.

void testComboExpiresInsteadOfAccumulatingForever() {
    section("reported  a combo is a chain, not a running total");

    Mario player({100.0f, 100.0f});
    check(player.getComboCounter() == 0, "a fresh player has no combo");
    check(player.getComboTimer() <= 0.0f, "and no chain running");

    player.incrementCombo();
    player.incrementCombo();
    check(player.getComboCounter() == 2, "two hits make a x2 chain");
    check(player.getComboTimer() > 0.0f, "which is running");

    // Another hit inside the window extends it rather than starting over.
    player.update(1.0f);
    player.incrementCombo();
    check(player.getComboCounter() == 3, "a hit inside the window extends the chain");
    check(player.getComboTimer() > 1.0f, "and refreshes its clock");

    // Left alone, it expires. resetCombo() existed with no callers at all, so the
    // counter was monotonic for the life of the Player: an "x7!" stayed nailed to
    // the middle of the screen for the rest of the level and the score multiplier
    // never came back down.
    player.update(Mario::COMBO_WINDOW + 0.1f);
    check(player.getComboCounter() == 0, "left alone, the chain expires");
    check(player.getComboTimer() <= 0.0f, "and its clock stops");

    // Taking a hit breaks it, which is what makes a long chain a risk.
    player.incrementCombo();
    player.incrementCombo();
    check(player.getComboCounter() == 2, "a new chain starts");
    player.takeDamage(1);
    check(player.getComboCounter() == 0, "and taking damage breaks it");
}

void testMiniIsNotSquare() {
    section("reported  Mini Mario is not a square, so its sprite is not stretched");

    // The tiny frames are 15-16 wide by 19-23 tall. A square box made
    // Entity::drawSprite pick its scale from the height and draw an ~11px-wide
    // figure inside a 14px box, so Mini read as pinched and vertically stretched
    // and slid around loose inside its own hitbox. This is the same defect the
    // Mega state's comment records; Mini was the one square left.
    MiniState mini;
    const sf::Vector2f size = mini.getSize();
    check(size.x != size.y, "Mini's box is not square");
    const float aspect = size.x / size.y;
    check(aspect > 0.68f && aspect < 0.90f,
          "and its aspect ratio matches the tiny artwork (~0.79)");

    // And it stays the smallest form.
    SuperState super;
    check(size.y < super.getSize().y, "Mini is still shorter than Super");
}

void testBossCannotBeCheesedByStandingOnIt() {
    section("reported  standing on a boss is not a free win");

    BoomBoom boss({200.0f, 200.0f});
    const int fullHealth = boss.getHealth();
    check(fullHealth > 1, "a boss takes more than one hit");

    // A landed hit starts its invulnerability window.
    check(boss.tryStomp(), "a first stomp lands");
    check(boss.getHealth() == fullHealth - 1, "and costs a health point");
    check(boss.isInvulnerable(), "and starts the i-frame window");

    // Which is the thing the resolver could not see before: isInvulnerable() was
    // protected, so contact awarded score and combo every frame regardless, and
    // one real hit landed each time the window silently lapsed. Standing still on
    // BoomBoom was the whole fight in three seconds.
    check(!boss.tryStomp(), "a second stomp inside the window does not land");
    check(boss.getHealth() == fullHealth - 1, "and costs nothing");

    // The descent gate is what stops resting on the boss counting as an attack.
    check(Boss::STOMP_MIN_DESCENT_SPEED > 0.0f,
          "a stomp requires an actual descent, not just contact");
}

void testDeathReportsWhichPlayerDied() {
    section("reported  in two players, the right player dies");

    // The event has always carried the dying player; the subscriber discarded it,
    // so an enemy hitting Player 2 ran the death sequence, the life deduction and
    // the game-over test on Player 1 — standing somewhere else, unharmed.
    Mario one({100.0f, 100.0f});
    Luigi two({300.0f, 100.0f});

    Player* reported = nullptr;
    EventBus::ScopedSubscription sub(
        EventType::PlayerDied, [&reported](const GameEvent& ev) {
            if (const Player* const* p = std::any_cast<Player*>(&ev.data)) {
                reported = const_cast<Player*>(*p);
            }
        });

    // Small + damage is the death path Player::powerDown publishes from.
    two.takeDamage(1);
    check(reported == &two, "Player 2 taking a fatal hit reports Player 2");

    reported = nullptr;
    one.takeDamage(1);
    check(reported == &one, "and Player 1 reports Player 1");

    // And the payload survives being read as the wrong type, which is what the
    // debug key publishes.
    reported = nullptr;
    EventBus::getInstance().publish({EventType::PlayerDied, 0});
    check(reported == nullptr, "an int payload is not mistaken for a player");
}

void testJinglesDoNotLoop() {
    section("reported  the level-clear jingle plays once");

    // Three owners played one cue: SoundManager's LevelComplete handler fired
    // playMusic AND playSound for the same fanfare, playMusic looped it for the
    // whole celebration, and VictoryState::enter played it again three seconds
    // later. The handler now owns it, once, unlooped.
    SoundManager& sound = SoundManager::getInstance();
    sound.playMusic("level_complete", /*loop=*/false);
    check(!sound.isMusicLooping(), "a jingle does not loop");

    sound.playMusic("overworld", /*loop=*/true);
    check(sound.isMusicLooping(), "level music still does");
}

void testPlayerTwoHasEveryControl() {
    section("reported  Player 2's controls are complete and reachable");

    InputManager& input = InputManager::getInstance();
    input.resetBindingsToDefaults(0);
    input.resetBindingsToDefaults(1);

    for (const char* action : {"left", "right", "jump", "run", "crouch", "fire",
                               "groundpound"}) {
        check(!input.getBoundKeyName(action, 1).empty(),
              std::string("Player 2 has a key bound for ") + action);
    }

    // Both pads expose the same action set — that symmetry is the claim worth
    // pinning. (The alternate jump keys, P1's Space and P2's new RShift, live in
    // the press mappings and deliberately not in the bound-key table, which holds
    // one key per action for the rebinding UI to display.)
    const auto padOne = input.resetBindingsToDefaults(0);
    const auto padTwo = input.resetBindingsToDefaults(1);
    check(padOne.size() == padTwo.size(),
          "and both pads default to the same number of controls");
    for (const auto& [action, key] : padOne) {
        check(padTwo.count(action) == 1,
              std::string("Player 2's defaults cover ") + action + " too");
    }
}


// ---------------------------------------------------------------------------
// Windows playtest, August 2026 — the 1-3 crash.
//
// Every spawn in this game arrives through a synchronous EventBus handler, and
// the handlers pushed straight onto the entity vector that PlayingState and
// PhysicsEngine were iterating at the time. A push_back that reallocates leaves
// those loops holding a pointer into freed storage.
//
// PlayingState cannot be constructed headlessly, so this reproduces the SHAPE
// of the bug against a std::vector: iterate with a range-for, let a handler
// append, and show that the deferred-queue discipline is what makes it safe.
// The assertion that matters is the one about capacity — it is what proves the
// unsafe version really would have reallocated mid-loop.
// ---------------------------------------------------------------------------
void testMidFrameSpawnsDoNotInvalidateTheEntityLoop() {
    section("crash  a spawn during the entity loop cannot reallocate under it");

    using Vec = std::vector<std::unique_ptr<Goomba>>;

    // Bowser breathes every 1.2-2.2s for the whole fight, so over a real fight
    // the list grows well past whatever capacity it started the frame with.
    Vec entities;
    for (int i = 0; i < 8; ++i) {
        entities.push_back(std::make_unique<Goomba>(sf::Vector2f(i * 32.0f, 0.0f)));
    }
    entities.shrink_to_fit();
    const std::size_t capacityAtFrameStart = entities.capacity();
    check(entities.size() == capacityAtFrameStart,
          "the entity list starts the frame full, as it does after a level load");

    // The deferred discipline: a handler firing mid-loop queues, and the queue
    // is drained after the loop.
    Vec pending;
    std::size_t visited = 0;
    for (const auto& entity : entities) {
        ++visited;
        (void)entity->getPosition();
        // Stand-in for Bowser::breatheFire() publishing EntitySpawnRequested.
        pending.push_back(std::make_unique<Goomba>(sf::Vector2f(0.0f, 0.0f)));
    }
    check(visited == 8, "every entity was visited exactly once, with no reallocation under the loop");
    check(entities.capacity() == capacityAtFrameStart,
          "the vector did not grow while it was being iterated");

    for (auto& queued : pending) entities.push_back(std::move(queued));
    check(entities.size() == 16, "the queue is admitted after the loop, so nothing is lost");
    check(entities.capacity() > capacityAtFrameStart,
          "admitting them DID reallocate - which is exactly what the old code did mid-loop");
}

// ---------------------------------------------------------------------------
// Windows playtest — the backdrop stood one tile inside the ground.
//
// syncBackdropGround() anchored the parallax layers to the lowest row holding
// any solid tile. Every shipped level floors on a two-row slab, so that is the
// BOTTOM of the slab, and every hill, bush and fence was drawn a full tile
// buried. The rule is now "widest solid row, then climb to the top of that
// contiguous slab", which this reproduces against the real level files.
// ---------------------------------------------------------------------------
void testBackdropStandsOnTheGroundSurface() {
    section("playtest  the backdrop's ground line is the top of the floor slab");

    auto surfaceRow = [](const TileMap& map) {
        std::vector<int> solidPerRow(static_cast<std::size_t>(map.getHeight()), 0);
        for (int y = 0; y < map.getHeight(); ++y) {
            for (int x = 0; x < map.getWidth(); ++x) {
                if (TileMap::getInfo(map.getTileType(x, y)).isSolid) {
                    ++solidPerRow[static_cast<std::size_t>(y)];
                }
            }
        }
        int floorRow = -1, widest = 0;
        for (int y = 0; y < map.getHeight(); ++y) {
            if (solidPerRow[static_cast<std::size_t>(y)] >= widest &&
                solidPerRow[static_cast<std::size_t>(y)] > 0) {
                widest = solidPerRow[static_cast<std::size_t>(y)];
                floorRow = y;
            }
        }
        if (floorRow < 0) return -1;
        const int threshold = std::max(1, (widest * 3) / 5);
        int row = floorRow;
        while (row > 0 && solidPerRow[static_cast<std::size_t>(row - 1)] >= threshold) --row;
        return row;
    };

    for (const std::string& name : {"level_1.json", "level_2.json", "level_3.json"}) {
        LevelLoader loader;
        LevelData data;
        TileMap map;
        if (!loader.loadLevel(levelPath(name), map, data)) {
            check(false, name + " loads");
            continue;
        }

        const int row = surfaceRow(map);
        // The answer must be a row the player can actually stand ON: solid here,
        // empty directly above.
        bool solidHere = false, clearAbove = true;
        for (int x = 0; x < map.getWidth(); ++x) {
            if (TileMap::getInfo(map.getTileType(x, row)).isSolid) { solidHere = true; break; }
        }
        int solidAbove = 0;
        for (int x = 0; x < map.getWidth(); ++x) {
            if (row > 0 && TileMap::getInfo(map.getTileType(x, row - 1)).isSolid) ++solidAbove;
        }
        clearAbove = solidAbove * 2 < map.getWidth();   // not another full slab row

        check(solidHere, name + ": the backdrop ground row is solid");
        check(clearAbove, name + ": nothing stands on top of it, so it is the surface not the underside");
    }
}

// ---------------------------------------------------------------------------
// Windows playtest — the flag hung in mid-air in every level.
//
// A flagpole is a 24x168 sprite positioned by its TOP-left corner, and all four
// level files named tile row 12 for it: foot at world y 552, floor at 672. The
// pole is now settled onto the floor at load time, so what the files must
// guarantee is only that there IS a floor in the flagpole's column.
// ---------------------------------------------------------------------------
void testEveryLevelsFlagpoleHasAFloorUnderIt() {
    section("playtest  the flagpole stands on ground, and the castle behind it");

    for (const std::string& name : {"level_1.json", "level_2.json", "level_3.json", "bonus_1.json"}) {
        LevelLoader loader;
        LevelData data;
        TileMap map;
        if (!loader.loadLevel(levelPath(name), map, data)) {
            check(false, name + " loads");
            continue;
        }

        const Flagpole* pole = nullptr;
        const Entity* castle = nullptr;
        for (const auto& entity : data.entities) {
            if (auto* f = dynamic_cast<Flagpole*>(entity.get())) pole = f;
            if (entity && entity->getTypeName() == "castle") castle = entity.get();
        }

        if (!pole) { check(false, name + " has a flagpole"); continue; }
        check(castle != nullptr, name + " ends with a castle");

        // Settling needs a floor somewhere below the pole's column.
        const float probeX = pole->getPosition().x + 12.0f;
        bool floorFound = false;
        float floorTop = 0.0f;
        const int gx = static_cast<int>(probeX / Constants::TILE_SIZE);
        for (int y = 0; y < map.getHeight(); ++y) {
            if (TileMap::getInfo(map.getTileType(gx, y)).isSolid) {
                floorFound = true;
                floorTop = static_cast<float>(y) * Constants::TILE_SIZE;
                break;
            }
        }
        check(floorFound, name + ": there is a floor in the flagpole's column to settle onto");
        if (!floorFound) continue;

        // And once settled, the pole's foot is exactly on it.
        const float settledTop = floorTop - pole->getPoleHeight();
        check(std::abs((settledTop + pole->getPoleHeight()) - floorTop) < 0.5f,
              name + ": the settled pole's foot lands on the floor surface");
    }
}

// ---------------------------------------------------------------------------
// Windows playtest — Bowser was nearly impossible.
//
// He is immune to fire, and a boss's i-frames make contact during them harmful,
// so the only legal input was five clean descending stomps under continuous
// fire. Fireballs now buy a stagger: an opening in which he stops attacking and
// can be hit safely, and which closes with the hit it paid for.
// ---------------------------------------------------------------------------
void testFireBuysAnOpeningOnBowser() {
    section("playtest  fire staggers Bowser, and the opening costs one hit");

    Bowser bowser({0.0f, 0.0f});
    const int startHealth = bowser.getHealth();

    check(!bowser.isStaggered(), "Bowser does not start staggered");

    for (int i = 0; i < Bowser::FIRE_HITS_PER_STAGGER - 1; ++i) {
        bowser.onHitByFireball();
    }
    check(!bowser.isStaggered(),
          "three fireballs are not enough - the opening has a price");
    check(bowser.getHealth() == startHealth,
          "fire still costs him no health: he breathes the stuff");

    bowser.onHitByFireball();
    check(bowser.isStaggered(), "the fourth fireball staggers him");
    check(!bowser.isInvulnerable(),
          "the stagger clears his i-frames, so the opening is real rather than visible-only");
    check(bowser.getHealth() == startHealth,
          "the stagger itself does no damage - it is an opening, not an attack");

    check(bowser.tryStomp(), "a stomp during the opening lands");
    check(bowser.getHealth() == startHealth - 1, "and costs him exactly one point");
    check(!bowser.isStaggered(),
          "the opening closes with the hit it paid for, so one stagger is not the whole fight");

    // And the next opening costs the same again.
    for (int i = 0; i < Bowser::FIRE_HITS_PER_STAGGER; ++i) bowser.onHitByFireball();
    check(bowser.isStaggered(), "four more fireballs buy the next opening");
}

// ---------------------------------------------------------------------------
// The axe: the non-combat way past Bowser. Reaching it must end the fight
// outright, whatever the health bar says and whatever guards are up.
// ---------------------------------------------------------------------------
void testTheAxeEndsTheFightOutright() {
    section("playtest  the axe defeats Bowser through every guard");

    Bowser bowser({0.0f, 0.0f});
    check(!bowser.isDefeated(), "Bowser starts the fight alive");

    // Put both guards up: a hit lands i-frames, and a stagger is also active.
    bowser.tryStomp();
    check(bowser.isInvulnerable(), "a landed hit leaves him invulnerable");

    bowser.defeatNow();
    check(bowser.isDefeated(), "the axe ends it regardless of i-frames");
    check(bowser.getHealth() == 0, "and takes the whole remaining bar");
}

// ---------------------------------------------------------------------------
// Windows playtest — the HUD said WORLD 1-1 in every level.
//
// HudData carried worldMajor/worldMinor as ints and PlayingState never assigned
// them, so the HUD printed its own defaults forever. It carries a label now,
// and the label has to come from the same catalogue the level select reads.
// ---------------------------------------------------------------------------
void testTheWorldLabelIsNotAlwaysOneOne() {
    section("playtest  the HUD's world field is per-level, not a constant");

    // The label is built from LevelCatalog, so distinct levels must have
    // distinct names for the field to be capable of changing at all.
    std::set<std::string> names;
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        names.insert(LevelCatalog::nameFor(i));
    }
    check(static_cast<int>(names.size()) == LevelCatalog::count(),
          "every catalogue level has a distinct display name");
    check(names.count("1-1") == 1 && names.count("1-3") == 1,
          "the campaign names are the ones the HUD will show");

    // HudData's default must not silently stand in for a level again.
    HudData fresh;
    check(fresh.worldLabel == "WORLD 1-1",
          "the default is still 1-1 - which is why PlayingState must overwrite it every load");
    HudData assigned;
    assigned.worldLabel = "WORLD 1-3";
    check(assigned.worldLabel != fresh.worldLabel,
          "and the field is writable per level");
}

// ---------------------------------------------------------------------------
// Windows playtest — Player 2 had no presence in the HUD.
// ---------------------------------------------------------------------------
void testTheHudCanShowBothPlayers() {
    section("playtest  the HUD carries Player 2's icon and lives");

    HudData data;
    check(!data.hasSecondPlayer, "single player by default, so nothing is drawn");

    data.hasSecondPlayer = true;
    data.secondCharacterName = "luigi";
    data.secondLives = 4;
    check(data.hasSecondPlayer && data.secondLives == 4,
          "a match supplies the second player's character and life count");
    check(data.secondCharacterName != data.characterName,
          "the two badges name different characters, which is what makes them distinguishable");
}


// ---------------------------------------------------------------------------
// The entity catalogue is a cross-file contract (g-rule-17).
//
// The same list of types was hand-written in three places — EntityFactory,
// SerializationUtils and the map editor's palette — and they had drifted: the
// editor knew 16 of 40 types and not one of them was an enemy or a block. The
// catalogue is now the single source; this is the test that makes adding a type
// in only one place a failure rather than a silent omission.
// ---------------------------------------------------------------------------
void testEntityCatalogueIsCompleteAndRoundTrips() {
    section("editor  every entity type is creatable, named correctly and placeable");

    const auto& entries = EntityCatalogue::all();
    check(!entries.empty(), "the catalogue is not empty");

    // No duplicate names, and no duplicate types: either one means a level file
    // can name something the catalogue answers two ways.
    std::set<std::string> names;
    std::set<int> types;
    bool duplicateName = false, duplicateType = false;
    for (const auto& entry : entries) {
        if (!names.insert(entry.name).second) duplicateName = true;
        if (!types.insert(static_cast<int>(entry.type)).second) duplicateType = true;
    }
    check(!duplicateName, "no two entries share a serialised name");
    check(!duplicateType, "no two entries share an EntityType");

    int uncreatable = 0, misnamed = 0, badParse = 0;
    std::string firstUncreatable, firstMisnamed, firstBadParse;

    for (const auto& entry : entries) {
        // (a) the factory can build it. A catalogue entry the factory does not
        //     handle is a palette button that silently places nothing.
        auto made = EntityFactory::create(entry.type, {64.0f, 64.0f});
        if (!made) {
            if (uncreatable++ == 0) firstUncreatable = entry.name;
            continue;
        }

        // (b) the class agrees with the catalogue about its own name. This is
        //     the one that matters most: saveLevel writes getTypeName(), and
        //     loadLevel parses it, so a mismatch means a level saved from the
        //     editor loads back as a different entity.
        if (made->getTypeName() != entry.name) {
            if (misnamed++ == 0) {
                firstMisnamed = entry.name + " -> " + made->getTypeName();
            }
        }

        // (c) the round trip closes.
        if (SerializationUtils::parseEntityTypeName(entry.name) != entry.type) {
            if (badParse++ == 0) firstBadParse = entry.name;
        }
    }

    check(uncreatable == 0,
          "every catalogued type can be built by EntityFactory" +
          (uncreatable ? " (first failure: " + firstUncreatable + ")" : std::string()));
    check(misnamed == 0,
          "every class's getTypeName() matches its catalogue name" +
          (misnamed ? " (first mismatch: " + firstMisnamed + ")" : std::string()));
    check(badParse == 0,
          "every catalogue name parses back to its own type" +
          (badParse ? " (first failure: " + firstBadParse + ")" : std::string()));

    // And the palette actually covers the game. The specific complaint was that
    // no enemy and no block could be placed at all.
    const std::size_t enemies =
        EntityCatalogue::inCategory(EntityCatalogue::Category::Enemy).size();
    const std::size_t blocks =
        EntityCatalogue::inCategory(EntityCatalogue::Category::Block).size();
    const std::size_t items =
        EntityCatalogue::inCategory(EntityCatalogue::Category::Item).size();
    check(enemies >= 13, "the palette offers every enemy, bosses included");
    check(blocks >= 8, "the palette offers the blocks and platforms");
    check(items >= 12, "the palette offers the items and power-ups");

    // Projectiles are runtime-only and must stay out of the placeable set: a
    // level file containing a hammer would spawn one frozen in mid-air.
    bool projectilePlaceable = false;
    for (auto category : EntityCatalogue::placeableCategories()) {
        if (category == EntityCatalogue::Category::Projectile) projectilePlaceable = true;
    }
    check(!projectilePlaceable, "projectiles are not offered as placeable scenery");
}


// ---------------------------------------------------------------------------
// The sandbox itself, asserted.
//
// Every fix above depends on TestSaveSandbox actually being in effect. If a
// future main() drops it, or Serializer::setSaveDirectory stops taking effect,
// the suite would go back to reading and DELETING real save files while
// continuing to report green - which is precisely how campaign progress was
// lost in the first place. So the suite checks its own containment.
// ---------------------------------------------------------------------------
void testTheSuiteCannotTouchRealSaveData() {
    section("hermeticity  this process cannot reach the real saves/ directory");

    const std::filesystem::path dir = Serializer::saveDirectory();
    check(!dir.empty(), "a save directory is resolved at all");

    // Compared canonically: "saves" and "./saves" are the same place, and a
    // string compare would be fooled by either spelling.
    std::error_code ec;
    const std::filesystem::path here = std::filesystem::current_path(ec);
    const std::filesystem::path resolved = std::filesystem::weakly_canonical(dir, ec);
    const std::filesystem::path tmp =
        std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(ec), ec);

    bool insideTemp = false;
    for (auto q = resolved; !q.empty() && q != q.root_path(); q = q.parent_path()) {
        if (q == tmp) { insideTemp = true; break; }
    }
    check(insideTemp,
          "the save directory is under the system temp dir, not the repository");

    // And specifically not any of the candidates Serializer would auto-resolve.
    bool isRealSaves = false;
    for (const char* candidate : {"saves", "../saves", "../../saves",
                                  "SuperMarioGame/saves", "../SuperMarioGame/saves"}) {
        std::error_code ce;
        const std::filesystem::path real =
            std::filesystem::weakly_canonical(here / candidate, ce);
        if (!ce && std::filesystem::exists(real, ce) && real == resolved) isRealSaves = true;
    }
    check(!isRealSaves, "and is none of the real saves/ directories on this machine");

    // Writing has to actually land there, or the redirection is cosmetic.
    HighScoreEntry probe;
    probe.score = 4242;
    probe.levelName = "hermeticity-probe";
    Serializer::recordHighScore(probe);
    check(std::filesystem::exists(dir / "highscores.json"),
          "a recorded score lands inside the sandbox, so the redirect is real");
    Serializer::clearHighScores();

    // CampaignProgress resolves through the same door. This is the call that
    // deleted a real file, so it is the one worth pinning.
    CampaignProgress::recordLevelCleared(0, {true, false, false});
    check(std::filesystem::exists(dir / "progress.json"),
          "campaign progress is written inside the sandbox too");
    CampaignProgress::reset();
    check(!std::filesystem::exists(dir / "progress.json"),
          "and reset() deletes THAT file - never the developer's");
}

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("regressions");

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
    testLevelsArePopulated();
    testHiddenBlockCanBeRevealed();
    testMovingPlatformActuallyMoves();
    testNonCollidableEntitiesAreNotAtTheOrigin();
    testKeyBindingsApply();
    testOverlayStatesRenderTheStack();
    testStateOperationsAreDeferred();
    testLevelCatalogCoversTheCampaign();
    testHighScoreTableIsSortedAndBounded();
    testFlagpoleFramesExist();
    testPowerUpFramesExistForEveryCharacter();
    testProjectilesOnlyHurtWhatTheyShould();
    testBowserTakesFiveHitsAndChangesPhase();
    testBossInvulnerabilityWindowStopsDoubleCounting();
    testLevelThreeActuallyContainsItsBoss();
    testBoomBoomEscalatesOncePerHit();
    testBoomBoomStaysInsideItsArena();
    testLevelTwoContainsItsMidBoss();
    testEndOfLevelCastleHasBuffer();
    testEveryBossTypeIsBuildable();
    testGeneratedLevelsAreVerifiedSolvable();
    testDifficultyStrategyActuallyChangesTheGame();
    testDifficultyScalesEnemiesAndBosses();
    testThwompRunsItsStateMachine();
    testEveryStrategyIdentifiesItself();
    testCampaignProgressUnlocksSequentially();
    testOnlyFireMarioCanShoot();
    testFireballUsesItsAtlasArtAndBurns();
    testFlagpoleFlagActuallyDescends();
    testEveryEnemyCarriesItsOwnSpeed();
    testMegaShrinksBackDown();
    testRebindingCannotStrandThePlayer();
    testSavesResolveToOneDirectory();
    testObjectPoolRecyclesInsteadOfAllocating();
    testPooledObjectsComeBackFresh();
    testPoolWillNotGrowWithoutBound();
    testEveryWritableTypeRoundTrips();
    testEntityConfigDrivesTuning();
    testBackgroundThemesAreDistinctAndDrawable();
    testLavaIsARealTileThatBurns();
    testCameraLooksAheadAndHonoursScrollModes();
    testColorblindModeIsActuallyConsumed();
    testNewGamePlusEscalatesAndKeepsUnlocks();
    testDailyChallengeIsTheSameForEveryone();
    testDebugConsoleDispatchesCommands();
    testReplayRecordsThinsAndPlaysBack();
    testTwoPlayerBindingsAreIndependent();
    testHeldKeysComeFromEventsNotTheOs();
    testEventBusSurvivesHandlersThatMutateIt();
    testScopedSubscriptionCannotBeForgotten();
    testFireballKillsThroughTheRealPipeline();
    testKoopaContactFollowsTheSeriesRules();
    testGroundPatrolsTurnAtLedgesAndHazards();
    testBossesStandOnOpenArenaFloor();
    testWarpPipesLandInsideTheirDestination();
    testCampaignPathContainsOnlyCompletableLevels();
    testCapeActuallyDoesSomething();
    testEveryEntityTypeDrawsRealArt();
    testDefeatingABossDoesNotDangle();
    testDeathIsASequenceThatEnds();
    testSmallMarioDyingReportsItInsteadOfSelfAccounting();
    testAFormSurvivesBeingRebuilt();
    testPiranhaPlantHidesInsideItsPipe();
    testAKickedShellKeepsGoingAndSparesItsKicker();

    // Reported by Member B, August 2026.
    testComboExpiresInsteadOfAccumulatingForever();
    testMiniIsNotSquare();
    testBossCannotBeCheesedByStandingOnIt();
    testDeathReportsWhichPlayerDied();
    testJinglesDoNotLoop();
    testPlayerTwoHasEveryControl();

    // Reported from the Windows playtest, August 2026.
    testMidFrameSpawnsDoNotInvalidateTheEntityLoop();
    testBackdropStandsOnTheGroundSurface();
    testEveryLevelsFlagpoleHasAFloorUnderIt();
    testFireBuysAnOpeningOnBowser();
    testTheAxeEndsTheFightOutright();
    testTheWorldLabelIsNotAlwaysOneOne();
    testTheHudCanShowBothPlayers();
    testEntityCatalogueIsCompleteAndRoundTrips();
    testTheSuiteCannotTouchRealSaveData();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
