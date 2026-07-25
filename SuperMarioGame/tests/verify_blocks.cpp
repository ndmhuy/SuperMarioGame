#include <iostream>
#include <cassert>
#include <cmath>
#include <SFML/System/Vector2.hpp>
#include "Entities/Block.hpp"
#include "Entities/BrickBlock.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Player.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/EventBus.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/CollisionDetector.hpp"

// Mock singletons/stubs to link without full game framework
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
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

// TileMap stubs
const TileInfo& TileMap::getInfo(TileType type) {
    static TileInfo info;
    return info;
}
void TileMap::render(sf::RenderTarget& target, Camera& camera) {}
TileType TileMap::getTileAt(float px, float py) const { return TileType::Empty; }
sf::Vector2i TileMap::worldToGrid(float px, float py) const { return {0,0}; }
sf::Vector2f TileMap::gridToWorld(int gx, int gy) const { return {0.f, 0.f}; }
TileType TileMap::getTileSurfaceType(float px, float py) const { return TileType::Empty; }
void TileMap::swapBricksAndCoins() {}
void TileMap::initialize(int width, int height) {}
void TileMap::setTile(int gx, int gy, TileType type) {}

// SoundManager stubs
SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}
void SoundManager::playSound(const std::string& id) {}
void SoundManager::playMusic(const std::string& path) {}
void SoundManager::stopMusic() {}
void SoundManager::pauseMusic() {}
void SoundManager::resumeMusic() {}
void SoundManager::shutdown() {}
void SoundManager::setSFXVolume(float volume) {}
void SoundManager::setMusicVolume(float volume) {}
SoundManager::SoundManager() {}

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
    void update(float dt) override {}
    void render(sf::RenderTarget& target) override {}
};

int main() {
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
        // Small state means height is 32.f
        assert(smallPlayer.getBoundingBox().height == 32.0f);
        
        // 1.2: Super player hitting brick
        BrickBlock brick2(blockPos, 0);
        TestPlayer superPlayer;
        superPlayer.changeState(std::make_unique<SuperState>());
        superPlayer.setPosition({ 100.f, 164.f }); 
        assert(superPlayer.getBoundingBox().height == 64.0f);

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

        bool powerupSpawned = false;
        int spawnedType = -1;
        auto subId = EventBus::getInstance().subscribe(EventType::PowerUpCollected, [&](const GameEvent& event) {
            powerupSpawned = true;
            spawnedType = std::any_cast<int>(event.data);
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

    std::cout << "[TEST] All Block Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
