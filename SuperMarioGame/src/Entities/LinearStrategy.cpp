#include "Entities/LinearStrategy.hpp"
#include "Entities/Enemy.hpp"
#include <cmath>

LinearStrategy::LinearStrategy(float speed, sf::Vector2f direction)
    : m_speed(speed), m_direction(direction) {
    float len = std::sqrt(m_direction.x * m_direction.x + m_direction.y * m_direction.y);
    if (len > 0.001f) {
        m_direction /= len;
    }
}

float LinearStrategy::getSpeed() const {
    return m_speed;
}

void LinearStrategy::setSpeed(float speed) {
    m_speed = speed;
}

sf::Vector2f LinearStrategy::getDirection() const {
    return m_direction;
}

void LinearStrategy::setDirection(sf::Vector2f direction) {
    m_direction = direction;
    float len = std::sqrt(m_direction.x * m_direction.x + m_direction.y * m_direction.y);
    if (len > 0.001f) {
        m_direction /= len;
    }
}

void LinearStrategy::applyMovement(Enemy& enemy, float dt) {
    enemy.velocity = m_direction * m_speed;
    enemy.facingRight = (enemy.velocity.x > 0.0f);
}
