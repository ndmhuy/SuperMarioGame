#include "Core/StatisticsTracker.hpp"
#include "Core/Game.hpp"
#include <any>
#include <exception>
#include <iostream>

StatisticsTracker& StatisticsTracker::getInstance() {
    static StatisticsTracker instance;
    return instance;
}

void StatisticsTracker::init() {
    if (m_initialized) return;

    // Subscribe to EventBus notifications
    auto sub = [this](EventType type) {
        return EventBus::ScopedSubscription(type, [this](const GameEvent& ev) { handleEvent(ev); });
    };
    m_subscriptions.push_back(sub(EventType::CoinCollected));
    m_subscriptions.push_back(sub(EventType::EnemyDefeated));
    m_subscriptions.push_back(sub(EventType::PlayerDied));
    m_subscriptions.push_back(sub(EventType::ComboHit));

    m_initialized = true;
}

void StatisticsTracker::shutdown() {
    m_subscriptions.clear();
    m_initialized = false;
}

void StatisticsTracker::reset() {
    m_stats = GameStatistics{};
}

void StatisticsTracker::update(float dt) {
    m_stats.totalTimePlayed += dt;
}

void StatisticsTracker::handleEvent(const GameEvent& event) {
    // A run recorded with Debug > Cheats on contributes nothing to the LIFETIME
    // counters. These are persisted to the profile and shown on the Game Over
    // panel and the STATS page, and a demo take would otherwise inflate every
    // one of them — including totalDeaths, since a lethal hit under IMMORTAL
    // still publishes PlayerDied on its way to being rescued. Same taint gate,
    // same reasoning, as AchievementManager::unlockAchievement.
    if (Game::getInstance().debugCheats().tainted()) return;

    try {
        switch (event.type) {
            case EventType::CoinCollected: {
                int amount = 1;
                if (event.data.has_value()) {
                    if (event.data.type() == typeid(int)) {
                        amount = std::any_cast<int>(event.data);
                    }
                }
                m_stats.totalCoinsCollected += amount;
                break;
            }
            case EventType::EnemyDefeated: {
                m_stats.totalEnemiesDefeated += 1;
                break;
            }
            case EventType::PlayerDied: {
                m_stats.totalDeaths += 1;
                break;
            }
            case EventType::ComboHit: {
                if (event.data.has_value() && event.data.type() == typeid(int)) {
                    int combo = std::any_cast<int>(event.data);
                    if (combo > m_stats.highestCombo) {
                        m_stats.highestCombo = combo;
                    }
                }
                break;
            }
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "StatisticsTracker::handleEvent exception: " << e.what() << std::endl;
    }
}
