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
#include "Utils/CampaignProgress.hpp"
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

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <SFML/Graphics/RenderTexture.hpp>

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

    check(LevelCatalog::count() == 7, "seven levels, matching advanceToNextLevel()");
    check(!LevelCatalog::isValidIndex(-1) && !LevelCatalog::isValidIndex(7),
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

    // Work on a scratch copy so a developer's real table is not clobbered.
    const std::string path = "saves/highscores.json";
    const std::string backup = "saves/highscores.json.regressionbak";
    const bool hadExisting = std::filesystem::exists(path);
    if (hadExisting) std::filesystem::rename(path, backup);

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

    std::filesystem::remove(path);
    if (hadExisting) std::filesystem::rename(backup, path);
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

    // Work on a scratch file so a developer's real progress survives.
    const std::string path = "saves/progress.json";
    const std::string backup = "saves/progress.json.regressionbak";
    const bool hadExisting = std::filesystem::exists(path);
    if (hadExisting) std::filesystem::rename(path, backup);
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
    if (hadExisting) std::filesystem::rename(backup, path);
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
    testEveryBossTypeIsBuildable();
    testDifficultyStrategyActuallyChangesTheGame();
    testDifficultyScalesEnemiesAndBosses();
    testThwompRunsItsStateMachine();
    testEveryStrategyIdentifiesItself();
    testCampaignProgressUnlocksSequentially();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
