#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/Mario.hpp"
#include "Core/EventBus.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/CollisionDetector.hpp"
#include <iostream>
#include <cassert>
#include "TestSaveSandbox.hpp"

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("flagpole_trampoline");

    std::cout << "=== Running Flagpole & Trampoline Verification Tests ===" << std::endl;

    // -------------------------------------------------------------------
    // Test 1: Flagpole Initialization, Catch Height Score & Flag Slide
    // -------------------------------------------------------------------
    {
        sf::Vector2f polePos(600.0f, 200.0f);
        Flagpole flagpole(polePos, 300.0f);
        Mario mario(sf::Vector2f(590.0f, 220.0f));

        assert(!flagpole.isTriggered());
        assert(flagpole.getPoleHeight() == 300.0f);

        bool levelCompletePublished = false;
        int pointsReceived = 0;

        auto subId = EventBus::getInstance().subscribe(EventType::LevelComplete, [&](const GameEvent& ev) {
            levelCompletePublished = true;
            if (ev.data.has_value()) {
                try {
                    pointsReceived = std::any_cast<int>(ev.data);
                } catch (...) {}
            }
        });

        // Player hits high on pole (catch height percentage >= 80% -> 5000 points)
        flagpole.onPlayerCollision(mario, 220.0f);

        assert(flagpole.isTriggered());
        assert(mario.getScore() == 5000);
        assert(levelCompletePublished);
        assert(pointsReceived == 5000);

        // Update flagpole and verify flag slides downward
        float initialFlagY = flagpole.getFlagY();
        flagpole.update(0.1f); // 100ms
        assert(flagpole.getFlagY() > initialFlagY);

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[PASS] Test 1: Flagpole score scaling, LevelComplete event, and flag sliding animation verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 2: Flagpole Low Catch Height Score
    // -------------------------------------------------------------------
    {
        sf::Vector2f polePos(600.0f, 200.0f);
        Flagpole flagpole(polePos, 300.0f);
        Mario mario(sf::Vector2f(590.0f, 450.0f));

        // Catch low on pole (collision Y = 450, distance from top = 250, height from bottom = 50 / 300 = 16.6% -> 100 points)
        flagpole.onPlayerCollision(mario, 450.0f);

        assert(flagpole.isTriggered());
        assert(mario.getScore() == 100);

        std::cout << "[PASS] Test 2: Low catch height score scaling verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 3: Trampoline Initialization & Spring Compression
    // -------------------------------------------------------------------
    {
        sf::Vector2f trampPos(400.0f, 400.0f);
        Trampoline trampoline(trampPos);
        Mario mario(sf::Vector2f(400.0f, 368.0f));

        assert(!trampoline.isBouncing());

        // Activate trampoline
        trampoline.activate(mario);

        assert(trampoline.isBouncing());
        assert(mario.getVelocity().y == -831.4f); // High spring jump impulse

        // Update trampoline timer
        trampoline.update(0.3f); // 300ms > 250ms compress duration
        assert(!trampoline.isBouncing());

        std::cout << "[PASS] Test 3: Trampoline spring compression and bounce impulse verified." << std::endl;
    }

    // -------------------------------------------------------------------
    // Test 4: Trampoline Directional Landing in CollisionResolver
    // -------------------------------------------------------------------
    {
        sf::Vector2f trampPos(400.0f, 400.0f);
        Trampoline trampoline(trampPos);
        Mario mario(sf::Vector2f(400.0f, 368.0f));

        CollisionInfo topLandingInfo;
        topLandingInfo.collided = true;
        topLandingInfo.normal = sf::Vector2f(0.0f, -1.0f); // Landing from top
        topLandingInfo.overlap = sf::Vector2f(0.0f, 4.0f);

        CollisionResolver resolver;
        resolver.resolvePlayerVsItem(mario, trampoline, topLandingInfo);

        assert(trampoline.isBouncing());
        assert(mario.getVelocity().y == -831.4f);

        std::cout << "[PASS] Test 4: Trampoline directional top collision landing verified." << std::endl;
    }

    std::cout << "=== ALL FLAGPOLE & TRAMPOLINE TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
