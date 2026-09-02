// verify_r21o_offcamera_gate.cpp — the off-camera entity update gate.
//
// THE DEFECT
// ----------
// Endless Mode never discards a chunk it has appended. extendEndlessLevelIfNeeded()
// splices 100 fresh tiles of content in every time the player gets within 40
// tiles of the current far edge, and PlayingState::update()'s entity pass then
// called update() on every one of those entities, every frame, for the rest of
// the run — including the chunks the player had walked past twenty seconds ago
// and can never see again. A four-minute scripted run reaches 166 live entities
// (tests/scripts/verify_r21o_offcamera_gate.txt); the count has no ceiling.
//
// WHY BOTH HALVES ARE TESTED HERE
// -------------------------------
// A gate that freezes something the player notices is worse than no gate, so a
// guard asserting only "the far entity stopped" would license exactly the bug
// the gate has to avoid. Every case below therefore comes in a pair: something
// at that distance MUST stop, and something else at the SAME distance MUST NOT.
//
// The four exemptions and the incident each avoids:
//
//   Player      Shadow Mario trails the human by up to 8 s of recorded input —
//               2400 px, 75 tiles, past any margin. Freezing it stalls the
//               leash the whole mode is built on. Same for a CPU player 2, who
//               in versus is routinely the far one of the pair.
//   Projectile  update() is what expires a shot and returns it to its pool. A
//               frozen shot never expires: it flies on forever and its pool
//               slot never comes back. That is a leak, not a saving.
//   Block       MovingPlatform sweeps a parametric path the player rides and
//               returns to. Terrain lives in TileMap, not m_entities, so the
//               whole category is a handful of near-free update()s.
//   no tiles    An entity that does not collide with tiles has nothing in the
//               world to stop it. PhysicsEngine::update() is NOT gated, so it
//               keeps integrating whatever velocity was left in a frozen
//               entity — a Boo or a Lakitu would coast away on a stale
//               velocity with nothing to hit and nothing to correct it, and a
//               Lakitu the player merely outran would leave for good instead
//               of catching up.
//
// Driven against a LIVE PlayingState — the real camera, the real m_entities,
// the real update() — because the gate is a property of that frame ordering.
// A harness that called freezableOffCamera() alone would pass while update()
// ignored it.
//
// Run via:  ctest -R r21o_offcamera_gate --output-on-failure

#include "Core/Game.hpp"
#include "Core/GameMode.hpp"
#include "Core/PlayingState.hpp"
#include "Entities/Boo.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Mario.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/Player.hpp"
#include "Graphics/Camera.hpp"
#include "Physics/AABB.hpp"
#include "Utils/Constants.hpp"

#include "TestSaveSandbox.hpp"

#include <imgui.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Global namespace, to match the `friend class OffCameraGateTestHooks;`
// declaration in PlayingState.hpp.
class OffCameraGateTestHooks {
public:
    static AABB thinkingRegion(const PlayingState& state) { return state.thinkingRegion(); }
    static AABB visibleBounds(const PlayingState& state) {
        return state.m_camera.getVisibleBounds();
    }
    static bool freezable(const PlayingState& state, const Entity& entity) {
        return state.freezableOffCamera(entity);
    }

    static std::size_t thought(const PlayingState& state) { return state.m_entitiesThought; }
    static std::size_t frozen(const PlayingState& state)  { return state.m_entitiesFrozen; }
    static std::size_t exempt(const PlayingState& state)  { return state.m_entitiesExempt; }

    static Player* playerOne(const PlayingState& state) { return state.m_player; }

    // The state's own floor query, so the harness parks the camera without
    // inventing its own floor-finding. NOT findSafeRespawn(): that one clamps
    // its answer to the CURRENT view, so asking it for tile 100 while the
    // camera is still at the level start returns tile 3.
    static float floorTopAt(const PlayingState& state, float worldX) {
        return state.floorBelow(worldX, 0.0f);
    }

    // Insertion through the state's own door, so an entity placed by this
    // harness is wired exactly as one the level loader placed.
    static Entity* admit(PlayingState& state, std::unique_ptr<Entity> entity) {
        Entity* raw = entity.get();
        state.admitEntity(raw);
        state.m_entities.push_back(std::move(entity));
        return raw;
    }
};

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!condition) ++g_failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

constexpr float FRAME = 1.0f / 60.0f;

// An entity that reports whether the update loop called it, and nothing else.
//
// Needed because the census counters are AGGREGATES over a live level: level_1
// has its own entities drifting across the region boundary, so "the total went
// up by one" cannot say WHICH entity stopped. This says it per object, and it
// lets each category be tested at the same distance without hunting for a real
// class that happens to have the right pair of flags.
//
// isPhysicsDriven() and isCollidable() are false so that the physics pass
// cannot MOVE the probe: its distance from the camera is the thing under test,
// and gravity would walk it out of position between frames.
class UpdateCounter : public Entity {
public:
    UpdateCounter(sf::Vector2f pos, EntityCategory category, bool collidesTiles)
        : Entity(pos, {32.0f, 32.0f}), m_category(category), m_collidesTiles(collidesTiles) {}

    void update(float) override { ++updates; }
    void render(sf::RenderTarget&) override {}

    EntityCategory getCategory() const override   { return m_category; }
    bool collidesWithTiles() const override       { return m_collidesTiles; }
    bool isPhysicsDriven() const override         { return false; }
    bool isCollidable() const override            { return false; }
    std::string getTypeName() const override      { return "test_update_counter"; }

    int updates = 0;

private:
    EntityCategory m_category;
    bool           m_collidesTiles;
};

// Park the player — and therefore the camera — in the MIDDLE of the level.
//
// This is not tidiness, and it took two attempts. PhysicsEngine's step 4.5
// clamps every entity to [0, mapWidth] regardless of isPhysicsDriven(), so at
// the level start the thinking region reaches x = -640 and anything placed
// "far behind" was snapped straight back to x = 0, inside the region. Then a
// bare setPosition() at tile 100 dropped the player into the geometry, killed
// them, and respawned them at x = 96 — back where it started. The Y therefore
// comes from the state's own floorBelow().
void parkCameraMidLevel(PlayingState& state) {
    Player* player = OffCameraGateTestHooks::playerOne(state);
    if (!player) return;
    constexpr float kParkX = 100.0f * Constants::TILE_SIZE;
    const float floorTop = OffCameraGateTestHooks::floorTopAt(state, kParkX);
    const sf::Vector2f perch{kParkX, floorTop - player->getBoundingBox().height};

    // PINNED every frame, not placed once. Dropped in at tile 100 the player
    // slid off whatever it landed on, fell, died and respawned at x = 96 — the
    // camera never left the level start and every "far behind" probe was
    // clamped back into view. Holding the position is scaffolding: the camera
    // is what has to move, and 120 frames is well past Camera::update()'s
    // smoothing so the region the probes are placed against is the one the next
    // frame will use.
    for (int i = 0; i < 120; ++i) {
        player->setPosition(perch);
        state.update(FRAME);
    }
    player->setPosition(perch);
}

sf::Vector2f farBehind(const PlayingState& state) {
    const AABB region = OffCameraGateTestHooks::thinkingRegion(state);
    return {region.x - 4.0f * Constants::TILE_SIZE, region.y + region.height * 0.5f};
}

sf::Vector2f farAhead(const PlayingState& state) {
    const AABB region = OffCameraGateTestHooks::thinkingRegion(state);
    return {region.x + region.width + 4.0f * Constants::TILE_SIZE,
            region.y + region.height * 0.5f};
}

} // namespace

// ---------------------------------------------------------------------------
// 1. The two halves, at the same distance, in the same frame.
//
//    The pairing is the whole point: a guard that only showed the enemy
//    stopping would pass just as happily on a gate that froze the shadow, the
//    fireballs and the platforms too.
// ---------------------------------------------------------------------------
void testTheGateStopsWhatItMayAndNothingElse() {
    section("gate  a far enemy stops; a projectile, a platform and a player at the same distance do not");

    PlayingState state(false, false, MapGeneratorConfig(), 0, 0);
    state.enter();
    parkCameraMidLevel(state);

    check(OffCameraGateTestHooks::playerOne(state) != nullptr, "the level starts with a player");
    if (!OffCameraGateTestHooks::playerOne(state)) { state.exit(); return; }

    const sf::Vector2f behind = farBehind(state);
    const sf::Vector2f ahead  = farAhead(state);
    check(behind.x > 0.0f,
          "the 'far behind' point is inside the level, so the boundary clamp leaves it there");

    // One probe per verdict, all at the same distance from the same camera.
    auto* gateable = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(behind, EntityCategory::Enemy, true)));
    auto* projectile = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(behind, EntityCategory::Projectile, true)));
    auto* block = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(behind, EntityCategory::Block, true)));
    auto* playerLike = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(behind, EntityCategory::Player, true)));
    auto* flyer = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(behind, EntityCategory::Enemy, false)));
    auto* item = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(ahead, EntityCategory::Item, true)));

    for (int i = 0; i < 10; ++i) state.update(FRAME);

    check(gateable->updates == 0,
          "a tile-colliding Enemy 4 tiles past the region did not update once in 10 frames");
    check(item->updates == 0,
          "nor did an Item the same distance AHEAD of the camera");

    check(projectile->updates == 10,
          "a Projectile at that distance updated on every one of the 10 frames");
    check(block->updates == 10,
          "so did a Block (a platform the player will come back and ride)");
    check(playerLike->updates == 10,
          "so did a Player (Shadow Mario trails by up to 75 tiles)");
    check(flyer->updates == 10,
          "and so did an Enemy that collides with no tiles (nothing would stop it coasting)");

    // A gate is not a despawn: the same object must resume when it comes back.
    gateable->setPosition(OffCameraGateTestHooks::visibleBounds(state).getCenter());
    for (int i = 0; i < 5; ++i) state.update(FRAME);
    check(gateable->updates == 5,
          "and brought back on camera, the frozen Enemy resumes updating");

    state.exit();
}

// ---------------------------------------------------------------------------
// 2. The same verdicts, on the real classes. Section 1 proves the LOOP obeys
//    freezableOffCamera(); this proves freezableOffCamera() answers correctly
//    for the classes actually in the game.
// ---------------------------------------------------------------------------
void testTheRealClassesAreClassifiedCorrectly() {
    section("gate  the real entity classes get the verdict their comments claim");

    PlayingState state(false, false, MapGeneratorConfig(), 0, 0,
                       MatchConfig{GameMode::VersusHuman});
    state.enter();
    parkCameraMidLevel(state);

    const sf::Vector2f away = farBehind(state);

    Entity* goomba = OffCameraGateTestHooks::admit(
        state, std::make_unique<Goomba>(away));
    check(OffCameraGateTestHooks::freezable(state, *goomba),
          "Goomba — Enemy, collides with tiles: freezable");

    // Its update() is what expires it and hands it back to m_fireballPool. A
    // frozen shot flies forever and the slot never returns.
    Entity* fireball = OffCameraGateTestHooks::admit(
        state, std::make_unique<Fireball>(away, sf::Vector2f(200.0f, 0.0f)));
    check(!OffCameraGateTestHooks::freezable(state, *fireball),
          "Fireball — Projectile: exempt, or its pool slot never comes back");

    Entity* platform = OffCameraGateTestHooks::admit(
        state, std::make_unique<MovingPlatform>(away, sf::Vector2f(128.0f, 0.0f), 40.0f));
    check(!OffCameraGateTestHooks::freezable(state, *platform),
          "MovingPlatform — Block: exempt, the player returns to ride its sweep");

    // The case the category test alone gets wrong: an Enemy, but with no tiles
    // to stop it while the ungated physics pass keeps integrating its velocity.
    Entity* boo = OffCameraGateTestHooks::admit(state, std::make_unique<Boo>(away));
    check(!OffCameraGateTestHooks::freezable(state, *boo),
          "Boo — Enemy but collides with no tiles: exempt, it would coast away for good");

    Player* second = nullptr;
    {
        auto owned = std::make_unique<Mario>(away);
        second = owned.get();
        OffCameraGateTestHooks::admit(state, std::move(owned));
    }
    check(second != nullptr && !OffCameraGateTestHooks::freezable(state, *second),
          "Mario — Player: exempt, and Shadow Mario is a Player subclass for this reason");

    state.exit();
}

// ---------------------------------------------------------------------------
// 3. The margin. Half a view in each axis, so nothing within two viewport
//    widths of the camera centre is ever frozen.
// ---------------------------------------------------------------------------
void testTheMarginIsHalfAViewOnEachSide() {
    section("gate  the margin is half the live view, not a hardcoded tile count");

    PlayingState state(false, false, MapGeneratorConfig(), 0, 0);
    state.enter();
    parkCameraMidLevel(state);

    const AABB visible = OffCameraGateTestHooks::visibleBounds(state);
    const AABB region  = OffCameraGateTestHooks::thinkingRegion(state);

    check(std::abs((visible.x - region.x) - visible.width * 0.5f) < 1.0f,
          "the region starts half a view left of what is visible");
    check(std::abs(region.width - visible.width * 2.0f) < 1.0f,
          "and is two views wide in total");
    check(std::abs(region.height - visible.height * 2.0f) < 1.0f,
          "two views tall, likewise");

    // The reason it is derived rather than written as a constant: at the
    // default 1280x720 view this is 20 tiles horizontally. Stated as an
    // assertion so a change to WINDOW_WIDTH shows up here rather than as a
    // stale comment.
    const float marginTiles = (visible.width * 0.5f) / Constants::TILE_SIZE;
    check(std::abs(marginTiles - 20.0f) < 0.5f,
          "which at the shipped 1280 px view is 20 tiles of margin");

    // An entity just outside the visible bounds but inside the margin must
    // still think: this is the case a naive "cull to the view" gate gets wrong,
    // and the one a camera jump (a pipe warp, a checkpoint respawn, a rewind)
    // would expose — the view lands somewhere new in a single frame.
    auto* justOffScreen = static_cast<UpdateCounter*>(OffCameraGateTestHooks::admit(
        state, std::make_unique<UpdateCounter>(
                   sf::Vector2f(visible.x - 2.0f * Constants::TILE_SIZE,
                                visible.y + visible.height * 0.5f),
                   EntityCategory::Enemy, true)));
    for (int i = 0; i < 5; ++i) state.update(FRAME);
    check(justOffScreen->updates == 5,
          "an Enemy two tiles off the left edge is inside the margin and still thinks");

    state.exit();
}

int main() {
    TestSaveSandbox sandbox("r21o_offcamera_gate");

    std::cout << "=========================================\n";
    std::cout << " R21 Lane 2D — the off-camera entity update gate\n";
    std::cout << "=========================================\n";

    // PlayingState::update() asks ImGui::GetIO() whether a text field owns the
    // keyboard before handing keys to the player, and ImGui asserts on a missing
    // context. A bare context needs no window and no GL — the same trick
    // verify_r21_versus_and_axes and verify_r21_debug_cheats use.
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Constants::WINDOW_WIDTH),
                                       static_cast<float>(Constants::WINDOW_HEIGHT));

    Game::getInstance().setDifficulty("normal");

    testTheGateStopsWhatItMayAndNothingElse();
    testTheRealClassesAreClassifiedCorrectly();
    testTheMarginIsHalfAViewOnEachSide();

    ImGui::DestroyContext();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
