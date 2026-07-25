#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/Serializer.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Player.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/EventBus.hpp"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <filesystem>
#include <cmath>
#include <unordered_map>

static bool floatsEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

// ---------------------------------------------------------
// Map Editor Tests
// ---------------------------------------------------------

void testTileCommands() {
    std::cout << "[Test 1/6] Running testTileCommands..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(10, 10);

    // Place a ground tile at (2, 3)
    PlaceTileCommand cmd1(tileMap, 2, 3, TileType::Ground);
    cmd1.execute();
    assert(tileMap.getTileType(2, 3) == TileType::Ground);

    // Place a brick tile at (2, 3) over ground
    PlaceTileCommand cmd2(tileMap, 2, 3, TileType::Brick);
    cmd2.execute();
    assert(tileMap.getTileType(2, 3) == TileType::Brick);

    // Undo brick placement
    cmd2.undo();
    assert(tileMap.getTileType(2, 3) == TileType::Ground);

    // Undo ground placement
    cmd1.undo();
    assert(tileMap.getTileType(2, 3) == TileType::Empty);

    // Erase command test
    tileMap.setTile(4, 4, TileType::Question);
    EraseTileCommand eraseCmd(tileMap, 4, 4);
    eraseCmd.execute();
    assert(tileMap.getTileType(4, 4) == TileType::Empty);
    
    eraseCmd.undo();
    assert(tileMap.getTileType(4, 4) == TileType::Question);

    std::cout << "  -> testTileCommands PASSED!" << std::endl;
}

void testEntityCommands() {
    std::cout << "[Test 2/6] Running testEntityCommands..." << std::endl;

    std::vector<std::unique_ptr<Entity>> entities;

    // Spawn a Mushroom
    PlaceEntityCommand cmd1(entities, "mushroom", 64.0f, 96.0f);
    cmd1.execute();
    assert(entities.size() == 1);
    assert(entities[0]->getPosition().x == 64.0f);
    assert(entities[0]->getPosition().y == 96.0f);

    // Erase the spawned Mushroom
    Entity* target = entities[0].get();
    EraseEntityCommand cmd2(entities, target);
    cmd2.execute();
    assert(entities.empty());

    // Undo erase
    cmd2.undo();
    assert(entities.size() == 1);
    assert(entities[0]->getPosition().x == 64.0f);

    // Undo spawn
    cmd1.undo();
    assert(entities.empty());

    std::cout << "  -> testEntityCommands PASSED!" << std::endl;
}

void testLevelSerialization() {
    std::cout << "[Test 3/6] Running testLevelSerialization..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(20, 10);
    tileMap.setTile(0, 9, TileType::Ground);
    tileMap.setTile(1, 9, TileType::Ground);
    tileMap.setTile(2, 8, TileType::Brick);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Mushroom>(sf::Vector2f(32.f, 64.f)));
    entities.push_back(std::make_unique<Coin>(sf::Vector2f(64.f, 64.f)));

    LevelLoader loader;
    std::string testPath = "saves/test_editor_save.json";

    // Save
    bool saved = loader.saveLevel(testPath, tileMap, entities, "Test Edit Level");
    assert(saved);
    assert(std::filesystem::exists(testPath));

    // Load back
    TileMap loadedMap;
    LevelData loadedData;
    bool loaded = loader.loadLevel(testPath, loadedMap, loadedData);
    assert(loaded);

    assert(loadedData.name == "Test Edit Level");
    assert(loadedMap.getWidth() == 20);
    assert(loadedMap.getHeight() == 10);
    assert(loadedMap.getTileType(0, 9) == TileType::Ground);
    assert(loadedMap.getTileType(2, 8) == TileType::Brick);

    // Mushroom and Coin should be spawned
    assert(loadedData.entities.size() == 2);
    
    // Cleanup
    std::filesystem::remove(testPath);

    std::cout << "  -> testLevelSerialization PASSED!" << std::endl;
}

// ---------------------------------------------------------
// Game Save/Load & Persistence Tests
// ---------------------------------------------------------

void testGameSaveLoad() {
    std::cout << "[Test 4/6] Running testGameSaveLoad..." << std::endl;

    // Create a player instance
    std::unique_ptr<Player> originalPlayer = std::make_unique<Mario>(sf::Vector2f(150.5f, 250.7f));
    originalPlayer->addCoins(25);
    originalPlayer->addScore(5000);
    originalPlayer->gainLife(); // Lives is now 4

    int levelId = 2;
    std::string levelName = "Underground Test";
    float timeRemaining = 185.5f;
    float checkpointX = 100.0f;
    float checkpointY = 50.0f;
    std::vector<bool> starCoins = { true, false, true };

    // Save slot 1
    bool saveSuccess = Serializer::saveGame(1, *originalPlayer, levelId, levelName, 
                                            timeRemaining, checkpointX, checkpointY, starCoins);
    assert(saveSuccess && "Failed to save game slot 1");

    // Load slot 1
    std::unique_ptr<Player> loadedPlayer;
    int loadedLevelId;
    std::string loadedLevelName;
    float loadedTimeRemaining;
    float loadedCheckX, loadedCheckY;
    std::vector<bool> loadedStarCoins;

    bool loadSuccess = Serializer::loadGame(1, loadedPlayer, loadedLevelId, loadedLevelName, 
                                            loadedTimeRemaining, loadedCheckX, loadedCheckY, loadedStarCoins);
    assert(loadSuccess && "Failed to load game slot 1");
    assert(loadedPlayer != nullptr && "Loaded player is null");

    // Assert assertions
    assert(floatsEqual(loadedPlayer->getPosition().x, 150.5f));
    assert(floatsEqual(loadedPlayer->getPosition().y, 250.7f));
    assert(loadedPlayer->getCoins() == 25);
    assert(loadedPlayer->getScore() == 5000);
    assert(loadedPlayer->getLives() == 4);

    assert(loadedLevelId == levelId);
    assert(loadedLevelName == levelName);
    assert(floatsEqual(loadedTimeRemaining, timeRemaining));
    assert(floatsEqual(loadedCheckX, checkpointX));
    assert(floatsEqual(loadedCheckY, checkpointY));
    assert(loadedStarCoins == starCoins);

    std::cout << "  -> testGameSaveLoad PASSED!" << std::endl;
}

void testTrackerAndAchievements() {
    std::cout << "[Test 5/6] Running testTrackerAndAchievements..." << std::endl;

    StatisticsTracker& tracker = StatisticsTracker::getInstance();
    AchievementManager& achievements = AchievementManager::getInstance();

    tracker.reset();
    achievements.reset();

    // Verify initial state
    assert(tracker.getStats().totalCoinsCollected == 0);
    assert(!achievements.isUnlocked("first_stomp"));
    assert(!achievements.isUnlocked("100_coins"));

    // Simulate events
    EventBus& bus = EventBus::getInstance();
    
    // Coin collected
    bus.publish({EventType::CoinCollected, 1});
    bus.publish({EventType::CoinCollected, 99}); // total 100
    assert(tracker.getStats().totalCoinsCollected == 100);
    assert(achievements.isUnlocked("100_coins"));

    // Enemy defeated
    bus.publish({EventType::EnemyDefeated, 0});
    assert(tracker.getStats().totalEnemiesDefeated == 1);
    assert(achievements.isUnlocked("first_stomp"));

    // Combo hit
    bus.publish({EventType::ComboHit, 8});
    assert(tracker.getStats().highestCombo == 8);
    assert(achievements.isUnlocked("combo_king"));

    // Check active toasts queue
    const auto& toasts = achievements.getActiveToasts();
    assert(!toasts.empty() && "No toast was created on unlock");

    std::cout << "  -> testTrackerAndAchievements PASSED!" << std::endl;
}

void testSettings() {
    std::cout << "[Test 6/6] Running testSettings..." << std::endl;

    float sfx = 45.0f, music = 35.0f;
    std::string diff = "hard";
    std::unordered_map<std::string, std::string> bindings = { {"jump", "Space"}, {"left", "Left"}, {"right", "Right"} };
    bool colorblind = true;

    bool saveSuccess = Serializer::saveSettings(sfx, music, diff, bindings, colorblind);
    assert(saveSuccess && "Failed to save settings");

    float loadedSfx, loadedMusic;
    std::string loadedDiff;
    std::unordered_map<std::string, std::string> loadedBindings;
    bool loadedColorblind;

    bool loadSuccess = Serializer::loadSettings(loadedSfx, loadedMusic, loadedDiff, loadedBindings, loadedColorblind);
    assert(loadSuccess && "Failed to load settings");

    assert(floatsEqual(loadedSfx, sfx));
    assert(floatsEqual(loadedMusic, music));
    assert(loadedDiff == diff);
    assert(loadedBindings == bindings);
    assert(loadedColorblind == colorblind);

    std::cout << "  -> testSettings PASSED!" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  RUNNING ALL SUPER MARIO GAME SUITE TESTS" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    // Initialize singletons
    StatisticsTracker::getInstance().init();
    AchievementManager::getInstance().init();

    // Map Editor tests
    testTileCommands();
    testEntityCommands();
    testLevelSerialization();

    // Save/Load & Persistence tests
    testGameSaveLoad();
    testTrackerAndAchievements();
    testSettings();

    // Cleanup temp save slot
    Serializer::deleteSlot(1);

    std::cout << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "  ALL VERIFICATION TESTS PASSED (6/6)!   " << std::endl;
    std::cout << "==========================================" << std::endl;
    return 0;
}
