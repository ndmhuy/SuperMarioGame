#include <iostream>
#include <cassert>
#include <cmath>
#include <SFML/System/Vector2.hpp>
#include "Entities/Goomba.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Boo.hpp"
#include "Core/EventBus.hpp"
#include "Utils/Constants.hpp"

#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"
#include "TestSaveSandbox.hpp"

// The stub Game and TileMap that used to live here are gone.
//
// They existed so this harness could link against a handful of entity files
// rather than the engine. The CMake target links the engine, so every stub
// collided with the real definition and the target had not built in a long
// time. Running against the real classes is what the other harnesses do, and it
// is the only way these assertions describe the shipped behaviour.

// We need to make sure the EventBus is clear for each test
void clearEventBus() {
}

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("enemies");

    std::cout << "[TEST] Starting Enemy Verification Suite..." << std::endl;

    // -------------------------------------------------------------
    // Test 1: Goomba Behavior
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Goomba tests..." << std::endl;
        sf::Vector2f startPos(100.f, 100.f);
        Goomba goomba(startPos, false);

        assert(!goomba.isRed());
        assert(!goomba.isSquished());
        assert(!goomba.isFlipped());
        assert(goomba.isActive());
        assert(goomba.getScoreValue() == 100);

        // Stomp verification
        bool pointsReceived = false;
        int receivedPoints = 0;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
            receivedPoints = std::any_cast<int>(event.data);
        });

        goomba.onStomped();

        assert(goomba.isSquished());
        assert(goomba.getVelocity().x == 0.0f);
        assert(goomba.getVelocity().y == 0.0f);
        assert(pointsReceived);
        assert(receivedPoints == 100);

        // Squish timer expiration (0.5s)
        goomba.update(0.3f);
        assert(goomba.isActive()); // still active at 0.3s
        goomba.update(0.3f);
        // Defeated, not deleted: a squashed Goomba holds its squish frame and a
        // fireballed one flips off screen before the prune takes it. This used
        // to assert !isActive() on the same frame, from before there was any
        // death animation to see.
        assert(goomba.isDeadOrDying()); // destroyed at 0.6s total

        EventBus::getInstance().unsubscribe(subId);

        // Fireball hit verification
        Goomba goomba2(startPos, false);
        bool fireballPoints = false;
        subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            fireballPoints = true;
        });

        goomba2.onHitByFireball();
        assert(goomba2.isFlipped());
        assert(!goomba2.isSquished());
        assert(goomba2.getVelocity().y == -300.0f);
        assert(fireballPoints);

        // A flipped enemy opts out of collision rather than reporting an empty
        // box — a degenerate AABB in the spatial hash is what isCollidable()
        // exists to avoid (audit B-14). The box stays real so the death
        // animation still has something to draw and move.
        assert(!goomba2.isCollidable());
        assert(goomba2.getBoundingBox().width > 0.0f);

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[TEST] Goomba tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 2: Koopa Troopa Behavior
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Koopa Troopa tests..." << std::endl;
        sf::Vector2f startPos(200.f, 200.f);
        KoopaTroopa koopa(startPos, false);

        assert(koopa.getState() == KoopaState::Walking);
        assert(koopa.getScoreValue() == 200);

        bool pointsReceived = false;
        int points = 0;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
            points = std::any_cast<int>(event.data);
        });

        // Stomp from walking to shell idle
        koopa.onStomped();
        assert(koopa.getState() == KoopaState::ShellIdle);
        assert(koopa.getVelocity().x == 0.0f);
        assert(pointsReceived);
        assert(points == 200);

        EventBus::getInstance().unsubscribe(subId);

        // Kick shell
        koopa.kick(sf::Vector2f(Constants::KOOPA_SHELL_KICK_SPEED, 0.f));
        assert(koopa.getState() == KoopaState::ShellKicked);
        assert(koopa.getVelocity().x == Constants::KOOPA_SHELL_KICK_SPEED);

        // Wall bounce
        koopa.setOnWall(true);
        koopa.update(0.1f);
        assert(koopa.getVelocity().x == -Constants::KOOPA_SHELL_KICK_SPEED);
        assert(!koopa.isOnWall());

        // Stomp while kicked stops it
        koopa.onStomped();
        assert(koopa.getState() == KoopaState::ShellIdle);
        assert(koopa.getVelocity().x == 0.0f);

        // Shell wake timer (5.0s)
        koopa.update(4.0f);
        assert(koopa.getState() == KoopaState::ShellIdle);
        koopa.update(1.5f); // 5.5s total
        assert(koopa.getState() == KoopaState::Walking);

        std::cout << "[TEST] Koopa Troopa tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 3: Koopa Paratroopa Behavior
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Koopa Paratroopa tests..." << std::endl;
        sf::Vector2f startPos(300.f, 300.f);
        KoopaParatroopa paratroopa(startPos, false);

        assert(paratroopa.hasWings());
        assert(paratroopa.getScoreValue() == 400);

        // Sine wave fly movement check (vertical speed changes)
        paratroopa.update(0.0f); // Init
        float initialVelY = paratroopa.getVelocity().y;
        paratroopa.update(0.1f);
        float nextVelY = paratroopa.getVelocity().y;
        // Verify FlyStrategy is active (velocity.y changed or sinusoidal)
        assert(initialVelY != nextVelY || std::abs(nextVelY) > 0.0f);

        bool pointsReceived = false;
        int points = 0;
        auto subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
            points = std::any_cast<int>(event.data);
        });

        // Stomp to lose wings
        paratroopa.onStomped();
        assert(!paratroopa.hasWings());
        assert(paratroopa.getScoreValue() == 200); // Standard Koopa Troopa points now
        assert(pointsReceived);
        assert(points == 400);

        EventBus::getInstance().unsubscribe(subId);

        // Subsequent stomp behaves like KoopaTroopa
        pointsReceived = false;
        subId = EventBus::getInstance().subscribe(EventType::EnemyDefeated, [&](const GameEvent& event) {
            pointsReceived = true;
            points = std::any_cast<int>(event.data);
        });

        // Losing the wings grants a one-second grace, so a single stomp cannot
        // be counted twice by an overlap that persists for several frames. The
        // test used to stomp again on the same frame and expected it to land.
        for (int i = 0; i < 70; ++i) paratroopa.update(1.0f / 60.0f);

        paratroopa.onStomped();
        assert(paratroopa.getState() == KoopaState::ShellIdle);
        assert(pointsReceived);
        assert(points == 200);

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[TEST] Koopa Paratroopa tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 4: Boo Behavior
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Boo tests..." << std::endl;
        sf::Vector2f startPos(400.f, 400.f);
        Boo boo(startPos);

        assert(boo.isActive());
        
        // Stomp should do nothing
        boo.onStomped();
        assert(boo.isActive());

        // Fireball should do nothing
        boo.onHitByFireball();
        assert(boo.isActive());
        assert(boo.getBoundingBox().width > 0.0f); // Bounding box remains active/solid

        // Verify gravity and tile collision exemption
        assert(boo.getGravityMultiplier() == 0.0f);
        assert(!boo.collidesWithTiles());

        std::cout << "[TEST] Boo tests PASSED!" << std::endl;
    }

    std::cout << "[TEST] All Enemy Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
