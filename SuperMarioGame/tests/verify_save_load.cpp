#include "Utils/Serializer.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Player.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/EventBus.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <cmath>

static bool floatsEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void testSerializer() {
    std::cout << "Running testSerializer..." << std::endl;

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

    std::cout << "testSerializer PASSED!" << std::endl;
}

void testTrackerAndAchievements() {
    std::cout << "Running testTrackerAndAchievements..." << std::endl;

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
    std::cout << "Active Toasts count: " << toasts.size() << std::endl;

    std::cout << "testTrackerAndAchievements PASSED!" << std::endl;
}

void testSettings() {
    std::cout << "Running testSettings..." << std::endl;

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

    std::cout << "testSettings PASSED!" << std::endl;
}

int main() {
    // Initialize singletons
    StatisticsTracker::getInstance().init();
    AchievementManager::getInstance().init();

    testSerializer();
    testTrackerAndAchievements();
    testSettings();

    // Cleanup files
    Serializer::deleteSlot(1);

    std::cout << "\nAll save/load persistence verification tests PASSED successfully!" << std::endl;
    return 0;
}
