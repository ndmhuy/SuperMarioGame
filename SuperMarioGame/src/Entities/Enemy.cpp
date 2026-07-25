#include "Entities/Enemy.hpp"

Enemy::Enemy(sf::Vector2f position, int scoreValue)
    : m_scoreValue(scoreValue) {
    this->position = position;
}

void Enemy::update(float dt) {
    if (m_aiStrategy) {
        m_aiStrategy->execute(*this, dt);
    }
}

void Enemy::render(sf::RenderTarget& target) {
    // Base rendering logic - override in concrete classes
}

void Enemy::setStrategy(std::unique_ptr<IMovementStrategy> strategy) {
    m_aiStrategy = std::move(strategy);
}

IMovementStrategy* Enemy::getStrategy() const {
    return m_aiStrategy.get();
}

int Enemy::getScoreValue() const {
    return m_scoreValue;
}

void Enemy::setScoreValue(int value) {
    m_scoreValue = value;
}
