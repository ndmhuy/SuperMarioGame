#include "Core/StatisticsTracker.hpp"
#include <any>
#include <exception>
#include <iostream>

StatisticsTracker& StatisticsTracker::getInstance() {
    static StatisticsTracker instance;
    return instance;
}

void StatisticsTracker::init() {
    if (m_initialized) return;

    EventBus& bus = EventBus::getInstance();
    
    // Subscribe to EventBus notifications
    m_subscriptions.push_back(bus.subscribe(EventType::CoinCollected, [this](const GameEvent& ev) { handleEvent(ev); }));
    m_subscriptions.push_back(bus.subscribe(EventType::EnemyDefeated, [this](const GameEvent& ev) { handleEvent(ev); }));
    m_subscriptions.push_back(bus.subscribe(EventType::PlayerDied, [this](const GameEvent& ev) { handleEvent(ev); }));
    m_subscriptions.push_back(bus.subscribe(EventType::ComboHit, [this](const GameEvent& ev) { handleEvent(ev); }));

    m_initialized = true;
}

void StatisticsTracker::shutdown() {
    EventBus& bus = EventBus::getInstance();
    for (auto subId : m_subscriptions) {
        bus.unsubscribe(subId);
    }
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
