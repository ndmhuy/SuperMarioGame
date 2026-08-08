#include "Entities/Fireball.hpp"
#include "Entities/Goomba.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/CollisionDetector.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== Running Fireball Verification Tests ===" << std::endl;

    // -------------------------------------------------------------------
    // Test 1: Fireball Initialization & Movement Integration
    // -------------------------------------------------------------------
    {
        sf::Vector2f spawnPos(100.0f, 100.0f);
        sf::Vector2f initialVel(350.0f, 100.0f);
        Fireball fireball(spawnPos, initialVel);

        assert(fireball.isActive());
        assert(fireball.getPosition() == spawnPos);
        assert(fireball.getVelocity() == initialVel);

        // Update physics
        fireball.update(0.1f); // 100ms
        assert(fireball.getPosition().x > spawnPos.x);

        std::cout << "[PASS] Test 1: Fireball initialization and movement integration verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 2: Fireball Bouncing Mechanics
    // -------------------------------------------------------------------
    {
        sf::Vector2f spawnPos(100.0f, 100.0f);
        sf::Vector2f initialVel(350.0f, 100.0f);
        Fireball fireball(spawnPos, initialVel);

        int initialBounces = fireball.getBouncesLeft();
        fireball.bounce();

        assert(fireball.getVelocity().y == -240.0f);
        assert(fireball.getBouncesLeft() == initialBounces - 1);

        std::cout << "[PASS] Test 2: Fireball ground bounce mechanics verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 3: Fireball Lifetime Expiration
    // -------------------------------------------------------------------
    {
        sf::Vector2f spawnPos(100.0f, 100.0f);
        sf::Vector2f initialVel(350.0f, 0.0f);
        Fireball fireball(spawnPos, initialVel);

        // Simulate 3.1 seconds
        fireball.update(3.1f);
        assert(!fireball.isActive());

        std::cout << "[PASS] Test 3: Fireball 3.0s lifetime expiration verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 4: Fireball vs Enemy Collision Impact
    // -------------------------------------------------------------------
    {
        sf::Vector2f spawnPos(200.0f, 200.0f);
        Fireball fireball(spawnPos, sf::Vector2f(350.0f, 0.0f));
        Goomba goomba(spawnPos);

        assert(goomba.isActive());
        assert(fireball.isActive());

        CollisionInfo info;
        info.collided = true;
        info.normal = sf::Vector2f(1.0f, 0.0f);

        CollisionResolver resolver;
        resolver.resolveFireballVsEnemy(fireball, goomba, info);

        // Fireball should be destroyed on enemy hit
        assert(!fireball.isActive());

        std::cout << "[PASS] Test 4: Fireball vs Enemy collision impact verified." << std::endl;
    }

    std::cout << "=== ALL FIREBALL TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
