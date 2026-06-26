#include <iostream>
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
#include "Entities/HiddenBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"
#include "Entities/IceBlock.hpp"
#include "Entities/ConveyorBelt.hpp"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

// Singletons / Stubs definition
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
};

int main() {
    std::cout << "[TEST] Starting New Blocks Verification Suite (Phase 3.13)..." << std::endl;

    TestPlayer player;
    Game::getInstance().setPlayer(&player);

    // -------------------------------------------------------------
    // Test 1: HiddenBlock Behavior
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running HiddenBlock tests..." << std::endl;
        sf::Vector2f blockPos(100.f, 300.f);
        HiddenBlock block(blockPos, 0); // Contains a Coin

        TEST_ASSERT(!block.isRevealed());
        // Unrevealed bounding box is empty
        TEST_ASSERT(block.getBoundingBox().width == 0.0f);

        // Hit from below triggers reveal
        block.onHitFromBelow(player);
        TEST_ASSERT(block.isRevealed());
        
        // Revealed bounding box is solid 32x32
        TEST_ASSERT(block.getBoundingBox().width == 32.0f);
        TEST_ASSERT(block.getBoundingBox().height == 32.0f);

        std::cout << "[TEST] HiddenBlock tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 2: MovingPlatform Carrying Physics
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running MovingPlatform tests..." << std::endl;
        sf::Vector2f platformPos(200.f, 300.f);
        // Platform moves 100px horizontally at speed 100px/s
        MovingPlatform plat(platformPos, sf::Vector2f(100.f, 0.f), 100.f);

        // Put player on top of the platform:
        // Platform top is y = 300. Player is 32x32.
        // Stand at center of platform (platform width is 64, so center x is 200 + 32 - 16 = 216)
        // Player feet at y = 300 (so player position.y = 268)
        player.setPosition({ 216.f, 268.f });
        player.setVelocity({ 0.f, 0.f }); // Resting

        // Update platform, moving forward. It should move by 100 * 0.1 = 10px right.
        plat.update(0.1f);
        
        // Platform should move to 210
        TEST_ASSERT(std::abs(plat.getPosition().x - 210.f) < 1.0f);

        // Player should have been carried by 10px to x = 226
        TEST_ASSERT(std::abs(player.getPosition().x - 226.f) < 1.0f);

        std::cout << "[TEST] MovingPlatform tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 3: FallingPlatform Lifecycle States
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running FallingPlatform tests..." << std::endl;
        sf::Vector2f startPos(300.f, 300.f);
        FallingPlatform plat(startPos);

        TEST_ASSERT(plat.getState() == FallingPlatformState::Idle);

        // Player not on platform -> stays Idle
        player.setPosition({ 0.f, 0.f });
        plat.update(0.1f);
        TEST_ASSERT(plat.getState() == FallingPlatformState::Idle);

        // Player stands on top of platform
        // Platform top is y = 300. Player position.y = 268
        player.setPosition({ 316.f, 268.f });
        player.setVelocity({ 0.f, 0.f });
        
        // Next update triggers Shaking state
        plat.update(0.01f);
        TEST_ASSERT(plat.getState() == FallingPlatformState::Shaking);

        // Update through shake time (1.0s)
        plat.update(1.0f);
        TEST_ASSERT(plat.getState() == FallingPlatformState::Falling);

        // Update while falling, check player falls too
        float initialPlayerY = player.getPosition().y;
        plat.update(0.1f);
        TEST_ASSERT(plat.getPosition().y > startPos.y); // moved down
        TEST_ASSERT(player.getPosition().y > initialPlayerY); // dragged down

        // Fall far enough to trigger respawn
        plat.update(1.0f); // fell way past 400px
        TEST_ASSERT(plat.getState() == FallingPlatformState::Respawning);
        TEST_ASSERT(plat.getBoundingBox().width == 0.f); // empty box while respawning

        // Wait out respawn time (5.0s)
        plat.update(5.1f);
        TEST_ASSERT(plat.getState() == FallingPlatformState::Idle);
        TEST_ASSERT(plat.getPosition() == startPos);

        std::cout << "[TEST] FallingPlatform tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 4: IceBlock Low Friction
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running IceBlock tests..." << std::endl;
        sf::Vector2f blockPos(400.f, 300.f);
        IceBlock ice(blockPos);

        TEST_ASSERT(ice.getFriction() == 0.1f);

        std::cout << "[TEST] IceBlock tests PASSED!" << std::endl;
    }

    // -------------------------------------------------------------
    // Test 5: ConveyorBelt Lateral Pushing Physics
    // -------------------------------------------------------------
    {
        std::cout << "[TEST] Running ConveyorBelt tests..." << std::endl;
        sf::Vector2f beltPos(500.f, 300.f);
        // Belt pushes right at 100px/s
        ConveyorBelt belt(beltPos, true, 100.f);

        // Place player on top of conveyor belt
        // Belt top is y = 300. Player position.y = 268. Center of belt is 500 + 16 = 516
        player.setPosition({ 516.f, 268.f });
        player.setVelocity({ 0.f, 0.f });

        // Update conveyor belt. It should push player right by 100 * 0.1 = 10px.
        belt.update(0.1f);

        // Player position.x should now be 526
        TEST_ASSERT(std::abs(player.getPosition().x - 526.f) < 1.0f);

        std::cout << "[TEST] ConveyorBelt tests PASSED!" << std::endl;
    }

    std::cout << "[TEST] All New Blocks Verification Tests PASSED successfully!" << std::endl;
    return 0;
}
