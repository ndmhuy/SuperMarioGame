#pragma once

#include "Core/EventBus.hpp"
#include <string>
#include <vector>

struct Achievement {
    std::string id;
    std::string name;
    std::string condition;
    std::string icon;
    bool unlocked = false;
};

struct AchievementToast {
    std::string id;
    std::string name;
    std::string icon;
    float timer = 0.0f; // display duration (3.0s total)
    float alpha = 1.0f; // fade out alpha
};

class AchievementManager {
public:
    static AchievementManager& getInstance();

    // Delete copy/move constructors for Singleton
    AchievementManager(const AchievementManager&) = delete;
    AchievementManager& operator=(const AchievementManager&) = delete;
    AchievementManager(AchievementManager&&) = delete;
    AchievementManager& operator=(AchievementManager&&) = delete;

    void init();
    void shutdown();
    void reset();

    void unlockAchievement(const std::string& id);
    bool isUnlocked(const std::string& id) const;

    const std::vector<Achievement>& getAchievements() const { return m_achievements; }
    void setUnlockedAchievements(const std::vector<std::string>& unlockedIds);
    std::vector<std::string> getUnlockedIds() const;

    // Toast updates
    void update(float dt);
    const std::vector<AchievementToast>& getActiveToasts() const { return m_activeToasts; }

    // Session helpers to monitor specific gameplay circumstances
    void registerDamageTaken();
    void registerBlockFound();
    void registerShellKickDefeat();
    void setLevelTime(float time);

private:
    AchievementManager() = default;
    ~AchievementManager() = default;

    void handleEvent(const GameEvent& event);
    void setupDefaultAchievementsList();

    std::vector<Achievement> m_achievements;
    std::vector<AchievementToast> m_activeToasts;
    std::vector<EventBus::SubscriptionId> m_subscriptions;

    // Session-based counters for achievement criteria verification
    int m_coinsThisRun = 0;
    float m_levelTime = 0.0f;
    bool m_takenDamageThisLevel = false;
    int m_enemiesDefeatedWithShell = 0;
    int m_hiddenBlocksFound = 0;
    int m_starCoinsCount = 0;

    bool m_initialized = false;
};
