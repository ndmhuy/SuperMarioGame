#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <SFML/System/Vector2.hpp>

// Include Core/Game and other mock/stubs
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"

// Include Entities
#include "Entities/Player.hpp"
#include "Entities/IPlayerState.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/BulletBill.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/ChainChomp.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Spiny.hpp"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

// No stubs here any more.
//
// This harness used to define its own Game, TileMap and SoundManager so it could
// link against a handful of entity files instead of the engine. The CMake target
// links the whole engine, so every stub collided with the real definition — 35
// duplicate symbols — and the target has not built in a long time. Running
// against the real classes is also what the other harnesses do, and it is the
// only way these assertions describe the shipped behaviour rather than the
// stubs' behaviour.
// Custom TestPlayer
class TestPlayer : public Player {
public:
    int damageTaken = 0;

    TestPlayer() {
        position = { 0.f, 0.f };
        velocity = { 0.f, 0.f };
        boundingBox.width = 32.f;
        boundingBox.height = 32.f;
        changeState(std::make_unique<SmallState>());
    }

    void update(float dt) override {}
    void render(sf::RenderTarget& target) override {}
    
    void takeDamage(int amount) override {
        damageTaken += amount;
    }

    // Player gained a pure virtual getCharacterName() when the save system
    // started recording who the run belonged to. This harness was never updated,
    // so TestPlayer stayed abstract and the target has not compiled since.
    std::string getCharacterName() const override { return "test"; }
};

int main() {
    std::cout << "[TEST] Starting New Enemies Verification Suite (Phase 3.9)..." << std::endl;

    TestPlayer player;
    Game::getInstance().setPlayer(&player);

    // -------------------------------------------------------------
    // Test 1: PiranhaPlant Behavior (TimerEmergenceStrategy)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running PiranhaPlant tests..." << std::endl;
        sf::Vector2f startPos(100.f, 100.f);
        PiranhaPlant plant(startPos);

        TEST_ASSERT(plant.isActive());
        
        // PiranhaPlant should be immune to stomp (damage player instead)
        player.damageTaken = 0;
        plant.onStomped();
        TEST_ASSERT(player.damageTaken == 1);
        TEST_ASSERT(plant.isActive()); // still alive

        // Staggered vertical emergence timer cycle checks:
        sf::Vector2f initialPos = plant.getPosition();
        
        // Retracted phase: no movement for 1.5s
        plant.update(1.5f);
        TEST_ASSERT(plant.getPosition().y == initialPos.y);
        TEST_ASSERT(plant.getVelocity().y == 0.0f);

        // Emerging phase: moves up (velocity set to negative)
        plant.update(1.0f); // total 2.5s -> inside emerging
        TEST_ASSERT(plant.getVelocity().y < 0.0f);

        // Emerged phase: position directly updated
        plant.update(1.0f); // total 3.5s -> inside emerged
        TEST_ASSERT(plant.getPosition().y < initialPos.y);
        TEST_ASSERT(plant.getVelocity().y == 0.0f);

        // Retreating phase: moves down (velocity set to positive)
        plant.update(3.0f); // total 6.5s -> inside retreating
        TEST_ASSERT(plant.getVelocity().y > 0.0f);

        // Wrapped around to Retracted: position reset
        plant.update(1.0f); // total 7.5s -> wrapped to 0.5s retracted
        TEST_ASSERT(plant.getPosition().y == initialPos.y);
        TEST_ASSERT(plant.getVelocity().y == 0.0f);

        // Fireball hits and defeats plant
        bool pointsReceived = false;
        int receivedPoints = 0;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
            receivedPoints = std::any_cast<int>(event.data);
        });

        plant.onHitByFireball();
        TEST_ASSERT(!plant.isActive());
        TEST_ASSERT(pointsReceived);
        TEST_ASSERT(receivedPoints == 100);

        EventBus::getInstance().unsubscribe(subId);

        // Verify gravity and tile collision exemption
        {
            PiranhaPlant testPlant(startPos);
            TEST_ASSERT(testPlant.getGravityMultiplier() == 0.0f);
            TEST_ASSERT(!testPlant.collidesWithTiles());
        }

        std::cout << "[TEST] PiranhaPlant tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 2: BulletBill Behavior (LinearStrategy, Stompable, Fireball Immune)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running BulletBill tests..." << std::endl;
        sf::Vector2f startPos(200.f, 200.f);
        BulletBill bill(startPos, -1.0f); // moves left

        TEST_ASSERT(bill.isActive());
        
        // Linear movement left (velocity should be negative horizontally)
        bill.update(1.0f);
        TEST_ASSERT(bill.getVelocity().x < 0.0f);
        TEST_ASSERT(bill.getVelocity().y == 0.0f);

        // Fireball immunity check
        bill.onHitByFireball();
        TEST_ASSERT(bill.isActive());

        // Stompable check
        bool pointsReceived = false;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
        });

        bill.onStomped();
        // A stomped Bullet Bill flips and flies off screen before it is pruned;
        // it is dying, not gone. This used to assert !isActive() on the same
        // frame, which was true back when a stomp deleted the enemy outright and
        // there was no death animation to watch.
        TEST_ASSERT(bill.isDeadOrDying());
        TEST_ASSERT(!bill.collidesWithTiles());
        TEST_ASSERT(pointsReceived);

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[TEST] BulletBill tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 3: HammerBro Behavior (HammerThrowStrategy)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running HammerBro tests..." << std::endl;
        sf::Vector2f startPos(300.f, 300.f);
        HammerBro bro(startPos);

        TEST_ASSERT(bro.isActive());
        
        // Update runs throw calculations, checks it doesn't crash
        bro.update(0.1f);
        TEST_ASSERT(bro.isActive());

        std::cout << "[TEST] HammerBro tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 4: Thwomp Behavior (ProximityTriggerStrategy)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Thwomp tests..." << std::endl;
        sf::Vector2f startPos(400.f, 100.f);
        Thwomp thwomp(startPos);

        TEST_ASSERT(thwomp.isActive());

        // Test stomp protection
        player.damageTaken = 0;
        thwomp.onStomped();
        TEST_ASSERT(player.damageTaken == 1);

        std::cout << "[TEST] Thwomp tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 5: ChainChomp Behavior (TetheredChaseStrategy)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running ChainChomp tests..." << std::endl;
        sf::Vector2f startPos(500.f, 500.f);
        ChainChomp chomp(startPos);

        TEST_ASSERT(chomp.isActive());

        // Test stomp protection
        player.damageTaken = 0;
        chomp.onStomped();
        TEST_ASSERT(player.damageTaken == 1);

        std::cout << "[TEST] ChainChomp tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 6: Lakitu & Spiny Spawn Behavior (FlyStrategy + Egg dropping)
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Lakitu & Spiny tests..." << std::endl;
        sf::Vector2f startPos(600.f, 50.f);
        Lakitu lakitu(startPos);

        TEST_ASSERT(lakitu.isActive());
        TEST_ASSERT(lakitu.getSpawnCount() == 0);

        // Position player under Lakitu
        player.setPosition({ 600.f, 200.f });

        // Update Lakitu for 4.1 seconds to trigger egg throw
        lakitu.update(2.0f);
        TEST_ASSERT(lakitu.getSpawnCount() == 0);
        lakitu.update(2.1f);
        TEST_ASSERT(lakitu.getSpawnCount() == 1);

        // Test Spiny
        sf::Vector2f eggDropPos = lakitu.getPosition();
        Spiny spiny(eggDropPos);
        TEST_ASSERT(spiny.isActive());
        TEST_ASSERT(!spiny.isFlipped());

        // Spiny stomp immunity
        player.damageTaken = 0;
        spiny.onStomped();
        TEST_ASSERT(player.damageTaken == 1);

        // Spiny fireball flip
        bool pointsReceived = false;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
        });

        spiny.onHitByFireball();
        TEST_ASSERT(spiny.isFlipped());
        TEST_ASSERT(pointsReceived);
        // Opts out of collision rather than reporting an empty box: a degenerate
        // AABB in the spatial hash is what isCollidable() exists to avoid
        // (audit B-14).
        TEST_ASSERT(!spiny.isCollidable());

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[TEST] Lakitu & Spiny tests PASSED!" << std::endl;
    }

    std::cout << "[TEST] All New Enemies Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
