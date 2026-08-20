#include "Entities/FlyStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

FlyStrategy::FlyStrategy(FlyMode mode, bool movingRight)
    : m_flyMode(mode),
      m_timer(0.0f),
      m_amplitude(48.0f),
      m_frequency(3.0f),
      m_baseY(0.0f),
      m_baseYInitialized(false),
      m_movingRight(movingRight) {}

FlyMode FlyStrategy::getFlyMode() const {
    return m_flyMode;
}

void FlyStrategy::setFlyMode(FlyMode mode) {
    m_flyMode = mode;
}

void FlyStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_baseYInitialized) {
        m_baseY = enemy.position.y;
        m_baseYInitialized = true;
    }

    if (m_flyMode == FlyMode::SinusoidalPatrol && enemy.onWall) {
        m_movingRight = !m_movingRight;
        enemy.onWall = false; // Reset wall flag after handling
        enemy.facingRight = m_movingRight;
    }
}

void FlyStrategy::applyMovement(Enemy& enemy, float dt) {
    m_timer += dt;

    if (m_flyMode == FlyMode::SinusoidalPatrol) {
        const float speed = enemy.speed;
        enemy.velocity.x = (m_movingRight ? 1.0f : -1.0f) * speed;
        enemy.velocity.y = m_amplitude * m_frequency * std::cos(m_frequency * m_timer);
        enemy.facingRight = m_movingRight;
    }
    else if (m_flyMode == FlyMode::VerticalBounce) {
        enemy.velocity.x = 0.0f;
        enemy.velocity.y = m_amplitude * m_frequency * std::cos(m_frequency * m_timer);
    }
    else if (m_flyMode == FlyMode::FollowPlayer) {
        Player* player = Game::getInstance().getNearestPlayer(enemy.getPosition());
        if (player) {
            float dx = player->position.x - enemy.position.x;
            const float trackSpeed = enemy.speed;

            if (std::abs(dx) > 10.0f) {
                enemy.velocity.x = (dx > 0.0f ? 1.0f : -1.0f) * trackSpeed;
            } else {
                enemy.velocity.x = 0.0f;
            }

            float targetY = player->position.y - 180.0f; // Hover 180px above player
            float dy = targetY - enemy.position.y;
            enemy.velocity.y = dy * 2.0f; // Smoothly approach target Y
            
            enemy.facingRight = (dx > 0.0f);
        } else {
            // Hover in place if player not found
            enemy.velocity.x = 0.0f;
            enemy.velocity.y = m_amplitude * m_frequency * std::cos(m_frequency * m_timer);
        }
    }
}
