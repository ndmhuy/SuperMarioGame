#include "Entities/RewardTracker.hpp"

RewardTracker::RewardTracker() {
    // Every subscription is the same shape: an event arrives, a weight is added.
    // The events all pre-date this class; nothing was added to the game to
    // produce them.
    m_coinSub = EventBus::ScopedSubscription(
        EventType::CoinCollected, [this](const GameEvent&) { add(m_weights.coin); });
    m_enemySub = EventBus::ScopedSubscription(
        EventType::EnemyDefeated, [this](const GameEvent&) { add(m_weights.enemyDefeated); });
    m_powerUpSub = EventBus::ScopedSubscription(
        EventType::PowerUpCollected, [this](const GameEvent&) { add(m_weights.powerUp); });
    m_starCoinSub = EventBus::ScopedSubscription(
        EventType::StarCoinCollected, [this](const GameEvent&) { add(m_weights.starCoin); });
    m_levelSub = EventBus::ScopedSubscription(
        EventType::LevelComplete, [this](const GameEvent&) { add(m_weights.levelComplete); });
    m_damagedSub = EventBus::ScopedSubscription(
        EventType::PlayerDamaged, [this](const GameEvent&) { add(m_weights.damaged); });
    m_diedSub = EventBus::ScopedSubscription(
        EventType::PlayerDied, [this](const GameEvent&) { add(m_weights.died); });
}

RewardTracker::~RewardTracker() = default;

void RewardTracker::add(float amount) {
    m_pending += amount;
    m_episodeTotal += amount;
}

void RewardTracker::observe(sf::Vector2f agentPosition) {
    if (!m_haveMark) {
        m_furthestX = agentPosition.x;
        m_haveMark = true;
        return;
    }

    // Only new ground pays. Crediting every rightward step and charging for
    // every leftward one nets to zero over a round trip in principle, but in
    // practice an agent finds the asymmetries and paces on the spot.
    if (agentPosition.x > m_furthestX) {
        add((agentPosition.x - m_furthestX) * m_weights.progressPerPixel);
        m_furthestX = agentPosition.x;
    }

    add(m_weights.timeStep);
}

float RewardTracker::consume() {
    const float value = m_pending;
    m_pending = 0.0f;
    return value;
}

void RewardTracker::reset(sf::Vector2f startPosition) {
    m_pending = 0.0f;
    m_episodeTotal = 0.0f;
    m_furthestX = startPosition.x;
    m_haveMark = true;
}
