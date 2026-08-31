#pragma once

#include "Core/EventBus.hpp"
#include <vector>

struct GameStatistics {
    int totalEnemiesDefeated = 0;
    int totalCoinsCollected = 0;
    int totalDeaths = 0;
    float totalTimePlayed = 0.0f;
    int highestCombo = 0;
};

class StatisticsTracker {
public:
    static StatisticsTracker& getInstance();

    // Delete copy/move constructors for Singleton
    StatisticsTracker(const StatisticsTracker&) = delete;
    StatisticsTracker& operator=(const StatisticsTracker&) = delete;
    StatisticsTracker(StatisticsTracker&&) = delete;
    StatisticsTracker& operator=(StatisticsTracker&&) = delete;

    void init();
    void shutdown();
    void reset();

    void update(float dt);

    const GameStatistics& getStats() const { return m_stats; }
    void setStats(const GameStatistics& stats) { m_stats = stats; }

private:
    StatisticsTracker() = default;
    ~StatisticsTracker() = default;

    void handleEvent(const GameEvent& event);

    GameStatistics m_stats;
    std::vector<EventBus::ScopedSubscription> m_subscriptions;
    bool m_initialized = false;
};
