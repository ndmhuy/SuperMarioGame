#include <iostream>
#include <cassert>
#include <cmath>
#include <SFML/System/Vector2.hpp>
#include "Entities/Block.hpp"
#include "Entities/BrickBlock.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Player.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/EventBus.hpp"
#include "Core/GameSnapshot.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/CollisionDetector.hpp"
#include "TestSaveSandbox.hpp"

// The stub Game, TileMap and SoundManager that used to live here are gone: the
// target links the whole engine, so every stub collided with the real
// definition and this harness had not built in a long time. Running against the
// real classes is also the only way its assertions describe shipped behaviour.

// Minimal Player subclass for testing
class TestPlayer : public Player {
public:
    TestPlayer() {
        position = { 0.f, 0.f };
        velocity = { 0.f, 0.f };
        boundingBox.width = 32.f;
        boundingBox.height = 32.f; // Default Small Mario
        changeState(std::make_unique<SmallState>());
    }
    std::string getCharacterName() const override { return "TestPlayer"; }
    void update(float dt) override {}
    void render(sf::RenderTarget& target) override {}
};

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("blocks");

    std::cout << "[TEST] Starting Block Verification Suite..." << std::endl;

    CollisionResolver resolver;

    // -------------------------------------------------------------
    // Test 1: BrickBlock Breaking vs Bumping
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running BrickBlock tests..." << std::endl;
        sf::Vector2f blockPos(100.f, 100.f);
        
        // 1.1: Small player hitting brick
        BrickBlock brick(blockPos, 0); // No coins, breakable
        TestPlayer smallPlayer;
        smallPlayer.setPosition({ 100.f, 132.f }); // Directly underneath
        
        // Simulate hit from below
        CollisionInfo info;
        info.collided = true;
        info.normal = { 0.f, 1.f }; // Ceiling hit pushes player down
        info.overlap = { 0.f, 4.f };

        resolver.resolveCharacterVsBlock(smallPlayer, brick, info);

        // Brick must still be active and trigger bump animation
        assert(brick.isActive());
        // SmallState is 24x30 — 32 was the placeholder box from before the
        // states owned their own sizes.
        assert(smallPlayer.getBoundingBox().height == 30.0f);
        
        // 1.2: Super player hitting brick
        BrickBlock brick2(blockPos, 0);
        TestPlayer superPlayer;
        superPlayer.changeState(std::make_unique<SuperState>());
        superPlayer.setPosition({ 100.f, 164.f }); 
        // SuperState is 24x60 — 64 was the placeholder box from before the
        // player states owned their own sizes.
        assert(superPlayer.getBoundingBox().height == 60.0f);

        bool blockBrokenEvent = false;
        auto subId = EventBus::getInstance().subscribe(EventType::BlockBroken, [&](const GameEvent& event) {
            blockBrokenEvent = true;
        });

        resolver.resolveCharacterVsBlock(superPlayer, brick2, info);

        // Brick2 must be broken (deactivated)
        assert(!brick2.isActive());
        assert(blockBrokenEvent);

        EventBus::getInstance().unsubscribe(subId);

        // 1.3: Brick Block containing coins
        BrickBlock brickWithCoins(blockPos, 3);
        TestPlayer player;
        player.changeState(std::make_unique<SmallState>());

        resolver.resolveCharacterVsBlock(player, brickWithCoins, info);
        assert(brickWithCoins.isActive());
        assert(brickWithCoins.getCoinsLeft() == 2);
        assert(player.getCoins() == 1);
        assert(player.getScore() == 200);

        std::cout << "[TEST] BrickBlock tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 2: QuestionBlock Item Spawning
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running QuestionBlock tests..." << std::endl;
        sf::Vector2f blockPos(200.f, 200.f);
        QuestionBlock qBlock(blockPos, 1); // Contains item type 1 (Mushroom)

        TestPlayer player;
        CollisionInfo info;
        info.collided = true;
        info.normal = { 0.f, 1.f };
        info.overlap = { 0.f, 4.f };

        assert(!qBlock.isEmpty());

        // PowerUpRequested, not PowerUpCollected. The block asks for an item to
        // be spawned on top of it; PowerUpCollected is the *pickup*
        // notification. It used to publish the pickup event and nothing
        // listened for it as a spawn request, so all 59 question blocks in the
        // game awarded points and produced nothing (audit B-2). This test
        // asserted the broken behaviour.
        bool powerupSpawned = false;
        int spawnedType = -1;
        auto subId = EventBus::getInstance().subscribe(EventType::PowerUpRequested, [&](const GameEvent& event) {
            powerupSpawned = true;
            spawnedType = std::any_cast<PowerUpRequest>(event.data).itemType;
        });

        resolver.resolveCharacterVsBlock(player, qBlock, info);

        // QuestionBlock must now be empty but active (solid block)
        assert(qBlock.isActive());
        assert(qBlock.isEmpty());
        assert(powerupSpawned);
        assert(spawnedType == 1);

        // Subsequent hits do nothing
        powerupSpawned = false;
        resolver.resolveCharacterVsBlock(player, qBlock, info);
        assert(!powerupSpawned);

        EventBus::getInstance().unsubscribe(subId);
        std::cout << "[TEST] QuestionBlock tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 3: Warp Pipe Checks
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Pipe tests..." << std::endl;
        sf::Vector2f pipePos(300.f, 300.f);
        sf::Vector2f warpExit(500.f, 500.f);
        Pipe pipe(pipePos, 1, warpExit, "", true); // Entrance pipe

        TestPlayer player;
        
        // 3.1: Player not on pipe
        player.setPosition({ 100.f, 100.f });
        assert(!pipe.checkWarp(player));

        // 3.2: Player on pipe but not pressing Down
        // Pipe is at 300, 300. Width is 64. Player width is 32.
        // Stand on top center: x = 316. Feet at y = 300 (so position.y = 268)
        player.setPosition({ 316.f, 268.f });
        assert(!pipe.checkWarp(player));

        std::cout << "[TEST] Pipe tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 4: Flagpole Heights & Scores
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running Flagpole tests..." << std::endl;
        sf::Vector2f polePos(400.f, 100.f); // Top is at y=100, height=300 -> bottom is at y=400
        Flagpole flagpole(polePos, 300.f);

        TestPlayer player;

        // 4.1: Hit near the top (e.g. collisionY = 120, height from bottom = 280 / 300 = 93%) -> 5000 points
        flagpole.onPlayerCollision(player, 120.f);
        assert(flagpole.isTriggered());
        assert(player.getScore() == 5000);

        // 4.2: Another player hits a new flagpole near bottom (e.g. collisionY = 380, height from bottom = 20 / 300 = 6%) -> 100 points
        Flagpole flagpole2(polePos, 300.f);
        TestPlayer player2;
        flagpole2.onPlayerCollision(player2, 380.f);
        assert(player2.getScore() == 100);

        std::cout << "[TEST] Flagpole tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 5: R5 (D8) — the contact rules that moved out of the resolver
    // -------------------------------------------------------------
    //
    // CollisionResolver used to name HiddenBlock, Flagpole, POWBlock and PSwitch
    // by dynamic_cast, one branch each. Those four branches are now
    // Block::onCharacterTouch and Item::onPlayerTouch overrides, and nothing in
    // the suite pinned any of them — so a wrong return value would have been
    // invisible. These checks go through the same resolver entry points the
    // engine uses, not through the overrides directly.
    //
    // They deliberately do NOT use assert(). This target is built with
    // -DNDEBUG (the shared Release configuration), which compiles every
    // assert() in this file away to nothing — so the sections above pass
    // vacuously. r5check() reports and fails the process for real.
    {
        std::cout << "[TEST] Running R5 dispatch-hook tests..." << std::endl;

        int r5Failures = 0;
        auto r5check = [&r5Failures](bool condition, const char* what) {
            std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << std::endl;
            if (!condition) ++r5Failures;
        };

        CollisionInfo fromBelow;   // player's head hits the underside
        fromBelow.collided = true;
        fromBelow.normal = { 0.f, 1.f };
        fromBelow.overlap = { 0.f, 4.f };

        CollisionInfo fromSide;    // player walks into the left face
        fromSide.collided = true;
        fromSide.normal = { -1.f, 0.f };
        fromSide.overlap = { 4.f, 0.f };

        // 5.1: an unrevealed HiddenBlock is solid only from underneath.
        {
            HiddenBlock hidden({ 200.f, 200.f });
            TestPlayer sideRunner;
            sideRunner.setPosition({ 172.f, 200.f });
            const sf::Vector2f before = sideRunner.getPosition();
            resolver.resolveCharacterVsBlock(sideRunner, hidden, fromSide);
            r5check(!hidden.isRevealed(),
                    "an unrevealed hidden block is not revealed by side contact");
            r5check(std::abs(sideRunner.getPosition().x - before.x) < 0.001f,
                    "and does not displace the runner: invisible geometry is not solid");

            TestPlayer headButter;
            headButter.setPosition({ 200.f, 232.f });
            resolver.resolveCharacterVsBlock(headButter, hidden, fromBelow);
            r5check(hidden.isRevealed(), "but a hit from below reveals it");
        }

        // 5.2: a Flagpole triggers on contact and never blocks the player.
        {
            Flagpole pole({ 400.f, 100.f }, 300.f);
            TestPlayer runner;
            runner.setPosition({ 396.f, 300.f });
            const sf::Vector2f before = runner.getPosition();
            resolver.resolveCharacterVsBlock(runner, pole, fromSide);
            r5check(pole.isTriggered(), "running into a flagpole triggers it");
            r5check(runner.getScore() > 0, "and pays the catch-height score");
            r5check(std::abs(runner.getPosition().x - before.x) < 0.001f,
                    "without physically blocking the player");
        }

        // 5.3: a POW block is terrain, struck only from below or by a real stomp.
        {
            POWBlock pow({ 500.f, 200.f });
            TestPlayer brusher;
            brusher.setPosition({ 468.f, 200.f });
            const sf::Vector2f before = brusher.getPosition();
            resolver.resolvePlayerVsItem(brusher, pow, fromSide);
            r5check(pow.getChargesLeft() == 3, "brushing a POW block's side is not a strike");
            r5check(!pow.isCollected(), "nor is it a pickup");
            r5check(std::abs(brusher.getPosition().x - before.x) > 0.001f,
                    "but it is solid, so the player is pushed back out");

            TestPlayer striker;
            striker.setPosition({ 500.f, 232.f });
            resolver.resolvePlayerVsItem(striker, pow, fromBelow);
            r5check(pow.getChargesLeft() == 2, "a hit from below spends one charge");
        }

        // 5.4: a P-Switch is pressed by any contact and stays solid underfoot.
        {
            PSwitch pswitch({ 600.f, 200.f });
            TestPlayer presser;
            presser.setPosition({ 568.f, 200.f });
            const sf::Vector2f before = presser.getPosition();
            resolver.resolvePlayerVsItem(presser, pswitch, fromSide);
            r5check(pswitch.isCollected(), "touching a P-Switch presses it");
            r5check(std::abs(presser.getPosition().x - before.x) > 0.001f,
                    "and the squished switch is still solid to stand on");
        }

        // 5.5: an ordinary powerup still falls through to the default collect.
        {
            Mushroom shroom({ 700.f, 200.f });
            TestPlayer collector;
            collector.setPosition({ 668.f, 200.f });
            resolver.resolvePlayerVsItem(collector, shroom, fromSide);
            r5check(shroom.isCollected(),
                    "an ordinary powerup is still collected from any direction");
        }

        if (r5Failures > 0) {
            std::cout << "[TEST] R5 dispatch-hook tests FAILED (" << r5Failures
                      << " checks)" << std::endl;
            return 1;
        }
        std::cout << "[TEST] R5 dispatch-hook tests PASSED!" << std::endl;
    }

    std::cout << "[TEST] All Block Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
