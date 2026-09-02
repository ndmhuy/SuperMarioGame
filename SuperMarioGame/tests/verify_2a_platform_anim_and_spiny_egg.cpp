// Lane 2A of the 2026-09-02 submission sweep: two independent gameplay
// defects, both wiring gaps rather than broken logic.
//
//   1. MovingPlatform::update() fully overrides Block::update() to own its
//      own kinematic motion (see MovingPlatform.hpp's isPhysicsDriven()
//      comment), but it never called back into the base implementation --
//      the ONLY place that advances m_animator. The platform moved and
//      carried the player correctly; its texture animation was frozen on
//      frame 0 forever. Fixed by MovingPlatform::update() calling
//      Block::update(dt) unconditionally, before its own early-return paths.
//
//   2. Spiny::isEgg was unreachable. SPEC.md 6.1 describes Spiny as walking
//      "on the ground after hatching from egg", and the class already
//      implements the full mechanic (Spiny::update(): show the egg sprite
//      while airborne, hatch to the walking sprite the frame isOnGround()
//      first becomes true) -- but nothing in production ever constructed one
//      with isEgg=true or called setEgg(true). EntityFactory::create() takes
//      only a type and a position, so every real spawn path (Lakitu's drop
//      via EntitySpawnRequested, LevelLoader, MapGenerator) goes through
//      EntityCatalogue's single-argument make<Spiny>, which always took the
//      default. Fixed by flipping that default (Spiny.hpp) to isEgg = true,
//      which reaches every real construction path without touching
//      EntitySpawnRequest (Core/GameSnapshot.hpp), PlayingState.cpp or
//      EntityCatalogue.cpp -- none of which this lane owns.
//
// Both cases below reproduce the defect exactly as MUTATING the fix would
// reintroduce it: comment out Block::update(dt) in MovingPlatform::update()
// and testMovingPlatformAnimatorAdvances fails; put the isEgg default back to
// false in Spiny.hpp and testSpinyDefaultsToEggAndHatchesOnLanding fails.
//
// Run via:  ctest -R verify_2a_platform_anim_and_spiny_egg --output-on-failure
#include "Core/Game.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/Spiny.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"
#include "TestSaveSandbox.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& what) {
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << std::endl;
    if (!condition) ++g_failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

const float DT = 1.0f / 60.0f;

// ---------------------------------------------------------------------------
// Defect 1 -- MovingPlatform's animator only advances if update() calls back
// into Block::update(). MovingPlatform's own shipped animation is a single,
// looping frame (Animator::update() re-derives m_curFrame == 0 every cycle
// regardless of dt), so it cannot be used to observe the freeze from outside
// the class. This subclass, local to the test, installs a ONE-SHOT
// (non-looping) probe animation through the protected members Block already
// exposes to any of its subclasses -- MovingPlatform included -- and reads
// back Animator::isDone(), which is public. isDone() flips to true and
// latches once enough dt has been fed to Animator::update(); it can only do
// that if something actually called m_animator->update(dt).
// ---------------------------------------------------------------------------
class ProbeMovingPlatform : public MovingPlatform {
public:
    using MovingPlatform::MovingPlatform;

    void setupAnimations(const SpriteSheet* spriteSheet) override {
        // Not MovingPlatform::setupAnimations(): that installs the real,
        // single-frame, looping animation this probe cannot observe through.
        // Block::setupAnimations() is the part actually under test here --
        // it is what constructs m_animator.
        Block::setupAnimations(spriteSheet);
        m_probeAnim = Animation("probe");
        m_probeAnim.frameList = {{"half_platform_long", 0.10f}};
        m_probeAnim.isLooping = false;
        if (m_animator) {
            m_animator->play(&m_probeAnim);
            m_hasAnimation = true;
        }
    }

    // Whether Block's animator has consumed its one probe frame -- true only
    // if MovingPlatform::update(dt) drove Block::update(dt) with enough
    // accumulated dt.
    bool animatorFinishedItsFrame() const {
        return m_animator && m_animator->isDone();
    }

private:
    Animation m_probeAnim;
};

void testMovingPlatformAnimatorAdvances() {
    section("MovingPlatform::update() drives Block::update(), so the animator is not frozen");

    auto sheet = SpriteSheet::loadAtlas("world_scenery_item");
    if (!sheet) {
        check(false, "world_scenery_item atlas loads (required to construct an Animator at all)");
        return;
    }

    ProbeMovingPlatform platform({256.0f, 480.0f}, sf::Vector2f{4.0f * Constants::TILE_SIZE, 0.0f}, 50.0f);
    platform.setupAnimations(sheet.get());

    check(!platform.animatorFinishedItsFrame(),
          "the probe animation has not consumed its one frame yet");

    // 0.10s probe frame duration; 30 frames at 1/60s is 0.5s -- five times
    // over. If Block::update() is never reached, m_animator->update() is
    // never called and this stays false forever regardless of how long the
    // loop runs.
    for (int frame = 0; frame < 30; ++frame) {
        platform.update(DT);
    }

    check(platform.animatorFinishedItsFrame(),
          "after half a second of update()s the animator has advanced past its one probe frame");

    // The platform's own kinematics (the thing update() was already doing
    // correctly) must still work -- this is a regression guard, not a
    // rewrite, so it must not have traded motion for animation.
    check(platform.getPosition().x > 256.0f,
          "the platform still travels along its configured sweep");
}

// ---------------------------------------------------------------------------
// Defect 2 -- SPEC 6.1: a Spiny is "hatched from an egg" as its baseline
// description, and Lakitu specifically "drops Spiny eggs every 4s". Neither
// is reachable if every Spiny is constructed already hatched.
// ---------------------------------------------------------------------------
void testSpinyDefaultsToEggAndHatchesOnLanding() {
    section("SPEC 6.1  a Spiny is born an egg and hatches into a walking Spiny on landing");

    TileMap map;
    map.initialize(20, 20);
    for (int x = 0; x < 20; ++x) map.setTile(x, 15, TileType::Ground);
    Game::getInstance().setTileMap(&map);

    PhysicsEngine physics;
    std::vector<std::unique_ptr<Entity>> entities;
    // Dropped well above the ground row (y = 15*32 = 480) so it falls for
    // several frames before landing, the way Lakitu's toss does.
    entities.push_back(std::make_unique<Spiny>(sf::Vector2f{200.0f, 200.0f}));
    Spiny* spiny = static_cast<Spiny*>(entities[0].get());

    check(spiny->isEgg(),
          "a freshly constructed Spiny starts in the egg state -- the state every real "
          "spawn path (Lakitu's drop, LevelLoader, MapGenerator) relies on by default, "
          "since EntityFactory::create() has no way to request it explicitly");

    bool touchedGroundEarly = false;
    bool hatchedWhileAirborne = false;
    for (int frame = 0; frame < 10; ++frame) {
        entities[0]->update(DT);
        physics.update(entities, map, DT);
        if (spiny->isOnGround()) { touchedGroundEarly = true; break; }
        if (!spiny->isEgg()) hatchedWhileAirborne = true;
    }
    check(!hatchedWhileAirborne, "it does not hatch while still airborne");
    check(!touchedGroundEarly, "ten frames of an 8-tile fall is not yet enough to land "
                               "(the harness's own timing assumption, not the fix)");

    bool hatchedOnceGrounded = false;
    for (int frame = 0; frame < 200 && spiny->isActive(); ++frame) {
        entities[0]->update(DT);
        physics.update(entities, map, DT);
        if (spiny->isOnGround() && !spiny->isEgg()) {
            hatchedOnceGrounded = true;
            break;
        }
    }
    check(hatchedOnceGrounded,
          "and hatches into a walking Spiny the moment it actually lands");

    Game::getInstance().setTileMap(nullptr);

    // An explicit request for an already-hatched Spiny (a caller that wants
    // one placed directly, bypassing the egg entirely) must still work --
    // the default changed, not the constructor's contract.
    Spiny alreadyHatched({64.0f, 64.0f}, /*isEgg=*/false);
    check(!alreadyHatched.isEgg(), "explicit isEgg=false still constructs an already-hatched Spiny");
}

}  // namespace

int main() {
    // Nothing here writes a save, but the harness convention is to seal the
    // process off regardless (g-rule-13).
    TestSaveSandbox sandbox("2a_platform_anim_and_spiny_egg");

    std::cout << "Lane 2A regression: MovingPlatform animator + Spiny egg hatching\n";

    testMovingPlatformAnimatorAdvances();
    testSpinyDefaultsToEggAndHatchesOnLanding();

    std::cout << "\n----------------------------------------\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)" << std::endl;
        return 1;
    }
    std::cout << "ALL PASS" << std::endl;
    return 0;
}
