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

// A dummy implementation of Game singleton because our tests might call Game::getInstance()
#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"

Game& Game::getInstance() {
    static Game instance;
    return instance;
}
Player* Game::getPlayer() const { return m_player; }
void Game::setPlayer(Player* player) { m_player = player; }
TileMap* Game::getTileMap() const { return m_tileMap; }
void Game::setTileMap(TileMap* tileMap) { m_tileMap = tileMap; }
void Game::run() {}
void Game::quit() {}
void Game::pushState(std::unique_ptr<IGameState> state) {}
void Game::popState() {}
void Game::changeState(std::unique_ptr<IGameState> state) {}
void Game::initWindow() {}
void Game::initImGui() {}
void Game::shutdown() {}

GameStateManager::~GameStateManager() = default;
TileType TileMap::getTileAt(float px, float py) const { return TileType::Empty; }

// We need to make sure the EventBus is clear for each test
void clearEventBus() {
}

int main() {
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
        assert(goomba.velocity.x == 0.0f);
        assert(goomba.velocity.y == 0.0f);
        assert(pointsReceived);
        assert(receivedPoints == 100);

        // Squish timer expiration (0.5s)
        goomba.update(0.3f);
        assert(goomba.isActive()); // still active at 0.3s
        goomba.update(0.3f);
        assert(!goomba.isActive()); // destroyed at 0.6s total

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
        assert(goomba2.velocity.y == -300.0f);
        assert(fireballPoints);

        // Bounding box should be empty on flipped/squished
        AABB box = goomba2.getBoundingBox();
        assert(box.width == 0.0f && box.height == 0.0f);

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
        assert(koopa.velocity.x == 0.0f);
        assert(pointsReceived);
        assert(points == 200);

        EventBus::getInstance().unsubscribe(subId);

        // Kick shell
        koopa.kick(sf::Vector2f(Constants::KOOPA_SHELL_KICK_SPEED, 0.f));
        assert(koopa.getState() == KoopaState::ShellKicked);
        assert(koopa.velocity.x == Constants::KOOPA_SHELL_KICK_SPEED);

        // Wall bounce
        koopa.onWall = true;
        koopa.update(0.1f);
        assert(koopa.velocity.x == -Constants::KOOPA_SHELL_KICK_SPEED);
        assert(!koopa.onWall);

        // Stomp while kicked stops it
        koopa.onStomped();
        assert(koopa.getState() == KoopaState::ShellIdle);
        assert(koopa.velocity.x == 0.0f);

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
        float initialVelY = paratroopa.velocity.y;
        paratroopa.update(0.1f);
        float nextVelY = paratroopa.velocity.y;
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

        std::cout << "[TEST] Boo tests PASSED!" << std::endl;
    }

    std::cout << "[TEST] All Enemy Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
