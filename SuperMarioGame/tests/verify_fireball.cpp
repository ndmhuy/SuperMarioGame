#include "Entities/Fireball.hpp"
#include "Entities/Goomba.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/CollisionDetector.hpp"
#include <iostream>
#include <cassert>
#include "TestSaveSandbox.hpp"

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("fireball");

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

        // Simulate 3.1 seconds: the lifetime expires and the burst starts.
        fireball.update(3.1f);
        // An expired fireball burns out over ~0.24s rather than vanishing, so it
        // is still active for that burst. This asserted !isActive() from before
        // there was an impact animation to play.
        for (int i = 0; i < 20; ++i) fireball.update(1.0f / 60.0f);
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
        // resolveFireballVsEnemy is gone: projectiles go through the generic
        // category dispatch now, so a hammer is never static_cast to a Fireball.
        resolver.resolveEntityVsEntity(fireball, goomba, info);

        // And the enemy must actually die. This assertion is the point of the
        // test and it was missing: it only ever checked the fireball, which is
        // how three enemies shipped with a shadowed m_isFlipped that made them
        // survive being hit.
        assert(goomba.isDeadOrDying());

        // The fireball bursts rather than vanishing, so let the burst finish
        // before asserting it is gone.
        for (int i = 0; i < 20; ++i) fireball.update(1.0f / 60.0f);
        assert(!fireball.isActive());

        std::cout << "[PASS] Test 4: Fireball vs Enemy collision impact verified." << std::endl;
    }

    std::cout << "=== ALL FIREBALL TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
