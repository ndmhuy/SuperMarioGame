// verify_r21_lakitu_flower — Lakitu's Fire Flower mercy drop.
//
// WHY THIS HARNESS EXISTS
//
// Bowser only staggers after FIRE_HITS_PER_STAGGER fireball hits, and only a
// staggered Bowser can be stomped. Nothing on the route to the bridge hands out
// a guaranteed Fire Flower, so a player who arrives as Small Mario had no route
// to win the game at all — the fight was unwinnable rather than hard. Lakitu now
// drops a Fire Flower after FLOWER_DROP_INTERVAL seconds of *engaged* time.
//
// The four properties that make it a rescue rather than decoration:
//
//   1. an engaged Lakitu eventually publishes an EntityType::FireFlower request;
//   2. it does so even while the concurrent Spiny cap is full — a player pinned
//      by three unstompable Spinies is exactly who the drop is for;
//   3. an UNENGAGED Lakitu never drops one, so the flower cannot be burnt
//      off-camera the way the lifetime Spiny cap burnt the eggs (R21 D8);
//   4. it cannot be farmed: at most one flower per interval, and none at all
//      while the player already holds the form.
//
// The drop leaves as an EventBus publish, exactly as the eggs do, so this
// harness subscribes to the bus and stands in for PlayingState's spawn handler
// the way verify_r21_entity_guards does for Spinies.
//
// Window-free and cheap, so CI can run it.
//
// Run via:  ctest -R r21_lakitu_flower --output-on-failure

#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Spiny.hpp"
#include "TestSaveSandbox.hpp"

#include <any>
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

// Stands in for PlayingState's EntitySpawnRequested handler, which is
// type-agnostic: everything that is not a pooled projectile goes to
// EntityFactory::create(). Counting the two types separately is the point —
// the flower must arrive without the eggs stopping.
struct SpawnWatcher {
    int flowers = 0;
    int eggs    = 0;
    std::vector<sf::Vector2f> flowerPositions;
    std::vector<sf::Vector2f> flowerVelocities;

    EventBus::ScopedSubscription sub;

    SpawnWatcher() {
        sub = EventBus::ScopedSubscription(
            EventType::EntitySpawnRequested, [this](const GameEvent& ev) {
                if (!ev.data.has_value() || ev.data.type() != typeid(EntitySpawnRequest)) return;
                const auto request = std::any_cast<EntitySpawnRequest>(ev.data);
                if (request.type == static_cast<int>(EntityType::FireFlower)) {
                    ++flowers;
                    flowerPositions.push_back(request.position);
                    flowerVelocities.push_back(request.velocity);
                } else if (request.type == static_cast<int>(EntityType::Spiny)) {
                    ++eggs;
                }
            });
    }
};

// Engaged seconds, in whole frames, so a test can talk in the same units the
// constant is written in.
int framesFor(float seconds) {
    return static_cast<int>(seconds * 60.0f + 0.5f);
}

// ---------------------------------------------------------------------------
// 1 + 4 — the drop arrives on the interval, once, and does not stop the eggs.
// ---------------------------------------------------------------------------
void testEngagedLakituDropsAFlowerOnTheInterval() {
    section("an engaged Lakitu drops a Fire Flower after FLOWER_DROP_INTERVAL");

    SpawnWatcher watcher;

    Mario mario({5000.0f, 500.0f});
    Game::getInstance().setPlayer(&mario);
    check(mario.getForm() == Player::Form::Small,
          "the player under test starts Small, i.e. with no route through Bowser");

    Lakitu lakitu({5000.0f, 200.0f});
    check(lakitu.isEngaged(), "Lakitu is engaged with the player beside it");
    check(lakitu.getFlowerCount() == 0, "and has dropped nothing yet");

    // Just short of the interval: nothing owed yet.
    for (int f = 0; f < framesFor(Lakitu::FLOWER_DROP_INTERVAL) - 2; ++f) lakitu.update(DT);
    check(watcher.flowers == 0,
          "no flower before the interval elapses (saw " + std::to_string(watcher.flowers) + ")");

    // Cross it.
    for (int f = 0; f < 4; ++f) lakitu.update(DT);
    check(watcher.flowers == 1,
          "exactly one flower once it does (saw " + std::to_string(watcher.flowers) + ")");
    check(lakitu.getFlowerCount() == 1, "and the entity's own tally agrees");

    if (watcher.flowers >= 1) {
        check(watcher.flowerPositions.front().y > lakitu.getPosition().y,
              "it is released below Lakitu, not inside it");
        check(watcher.flowerVelocities.front().x == 0.0f,
              "with no sideways kick, so it falls where the player can reach it");
    }

    // A second interval must pass before another one. This is the anti-farm
    // property: standing under a Lakitu is not a flower vending machine.
    for (int f = 0; f < framesFor(Lakitu::FLOWER_DROP_INTERVAL) - 4; ++f) lakitu.update(DT);
    check(watcher.flowers == 1,
          "still only one part-way through the next interval (saw " +
          std::to_string(watcher.flowers) + ")");

    for (int f = 0; f < 8; ++f) lakitu.update(DT);
    check(watcher.flowers == 2,
          "and a second once that interval completes (saw " +
          std::to_string(watcher.flowers) + ")");

    // Requirement 4: the egg behaviour another lane restored is untouched.
    check(watcher.eggs > 0,
          "Lakitu went on throwing Spiny eggs throughout (" + std::to_string(watcher.eggs) + ")");

    Game::getInstance().setPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// 2 — the flower is NOT rationed by the concurrent Spiny cap.
// ---------------------------------------------------------------------------
void testFlowerIgnoresTheConcurrentSpinyCap() {
    section("the mercy drop is not gated behind the Spiny cap");

    SpawnWatcher watcher;

    Mario mario({5000.0f, 500.0f});
    Game::getInstance().setPlayer(&mario);

    // Pin the player: MAX_SPINIES live Spinies, which is the state in which
    // Lakitu refuses to throw another egg.
    std::vector<std::unique_ptr<Spiny>> pinned;
    for (int i = 0; i < Lakitu::MAX_SPINIES; ++i) {
        pinned.push_back(std::make_unique<Spiny>(sf::Vector2f{5000.0f + 40.0f * i, 500.0f}));
    }
    check(Spiny::liveCount() >= Lakitu::MAX_SPINIES,
          "the census reports a full field of Spinies (" +
          std::to_string(Spiny::liveCount()) + ")");

    Lakitu lakitu({5000.0f, 200.0f});
    for (int f = 0; f < framesFor(Lakitu::FLOWER_DROP_INTERVAL) + 8; ++f) lakitu.update(DT);

    check(watcher.eggs == 0,
          "no further eggs while the field is full, so the cap still holds (saw " +
          std::to_string(watcher.eggs) + ")");
    check(watcher.flowers == 1,
          "but the flower still arrives — the player pinned by three Spinies is "
          "exactly who needs it (saw " + std::to_string(watcher.flowers) + ")");

    pinned.clear();
    Game::getInstance().setPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// 3 — an unengaged Lakitu drops nothing at all.
// ---------------------------------------------------------------------------
void testUnengagedLakituNeverDrops() {
    section("an unengaged Lakitu never drops a flower");

    SpawnWatcher watcher;

    Mario mario({0.0f, 500.0f});
    Game::getInstance().setPlayer(&mario);

    // The level_1 Lakitu sits at tile (175,11), ~5500px from the player spawn.
    Lakitu lakitu({5600.0f, 350.0f});
    check(!lakitu.isEngaged(), "a Lakitu 5600px away is not engaged");

    // Three minutes of the player being elsewhere — far longer than the walk
    // down level_1, and nine times the flower interval.
    for (int f = 0; f < framesFor(180.0f); ++f) lakitu.update(DT);
    check(watcher.flowers == 0,
          "it drops no flower at all while off-camera (saw " +
          std::to_string(watcher.flowers) + ")");
    check(watcher.eggs == 0, "nor any egg, as before");
    check(lakitu.getFlowerTimer() == 0.0f,
          "and it has not even banked the time, so the drop cannot be pre-earned");

    // Arriving must still work: the allowance was held, not spent.
    mario.setPosition({5600.0f, 500.0f});
    check(lakitu.isEngaged(), "and is engaged once the player arrives");
    for (int f = 0; f < framesFor(Lakitu::FLOWER_DROP_INTERVAL) + 8; ++f) lakitu.update(DT);
    check(watcher.flowers == 1,
          "then drops on schedule from the moment of arrival (saw " +
          std::to_string(watcher.flowers) + ")");

    Game::getInstance().setPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// 4b — a player who already holds Fire is not handed a second flower, and the
//      banked time is not thrown away either.
// ---------------------------------------------------------------------------
void testFirePlayerGetsNoneUntilTheFormIsLost() {
    section("a Fire player gets no flower, but loses nothing by waiting");

    SpawnWatcher watcher;

    Mario mario({5000.0f, 500.0f});
    mario.setForm(Player::Form::Fire);
    Game::getInstance().setPlayer(&mario);
    check(mario.getForm() == Player::Form::Fire, "the player under test is already Fire");

    Lakitu lakitu({5000.0f, 200.0f});

    // Twice the interval. A Fire Mario is not litter-bombed.
    for (int f = 0; f < framesFor(2.0f * Lakitu::FLOWER_DROP_INTERVAL); ++f) lakitu.update(DT);
    check(watcher.flowers == 0,
          "no flower for a player who already has the answer (saw " +
          std::to_string(watcher.flowers) + ")");

    // The interval is banked, not reset — this is what makes it a mercy: lose
    // the form mid-fight and the replacement is there, not twenty seconds later.
    check(lakitu.getFlowerTimer() >= Lakitu::FLOWER_DROP_INTERVAL,
          "the earned interval is parked at the threshold (timer " +
          std::to_string(lakitu.getFlowerTimer()) + ")");

    mario.setForm(Player::Form::Small);
    lakitu.update(DT);
    check(watcher.flowers == 1,
          "and lands on the very next frame after the form is lost (saw " +
          std::to_string(watcher.flowers) + ")");

    // Having paid out, the clock restarts: still not farmable.
    for (int f = 0; f < framesFor(Lakitu::FLOWER_DROP_INTERVAL) - 4; ++f) lakitu.update(DT);
    check(watcher.flowers == 1,
          "then a full interval before the next (saw " + std::to_string(watcher.flowers) + ")");

    Game::getInstance().setPlayer(nullptr);
}

// ---------------------------------------------------------------------------
// The other half of the contract: PlayingState's handler routes anything that
// is not a pooled projectile to EntityFactory. If the factory could not build a
// FireFlower from the type Lakitu publishes, the request would be swallowed and
// the drop would be inert in the running game.
// ---------------------------------------------------------------------------
void testTheFactoryCanBuildWhatLakituAsksFor() {
    section("EntityFactory builds the type Lakitu publishes");

    auto flower = EntityFactory::create(EntityType::FireFlower, {100.0f, 100.0f});
    check(flower != nullptr, "EntityType::FireFlower is a constructible registry entry");
    if (flower) {
        check(flower->getTypeName() == "fire_flower",
              "and it really is a fire flower (got \"" + flower->getTypeName() + "\")");
        check(flower->getCategory() == EntityCategory::Item,
              "categorised as an Item, so touching it collects it");
        check(flower->getGravityMultiplier() > 0.0f && flower->collidesWithTiles(),
              "with gravity and tile collision, so a dropped one lands instead of "
              "falling out of the world");
    }
}

}  // namespace

int main() {
    // Every save path in this process points at a throwaway directory, so
    // nothing here can read or delete real save data. See TestSaveSandbox.hpp.
    TestSaveSandbox sandbox("r21_lakitu_flower");

    testEngagedLakituDropsAFlowerOnTheInterval();
    testFlowerIgnoresTheConcurrentSpinyCap();
    testUnengagedLakituNeverDrops();
    testFirePlayerGetsNoneUntilTheFormIsLost();
    testTheFactoryCanBuildWhatLakituAsksFor();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
