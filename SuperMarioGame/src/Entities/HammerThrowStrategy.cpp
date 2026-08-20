#include "Entities/HammerThrowStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

HammerThrowStrategy::HammerThrowStrategy(float throwCooldown, float jumpCooldown)
    : m_throwCooldownTimer(throwCooldown),
      m_jumpCooldownTimer(jumpCooldown),
      m_throwCooldownMax(throwCooldown),
      m_jumpCooldownMax(jumpCooldown) {}

void HammerThrowStrategy::setThrowCallback(std::function<void(sf::Vector2f position, bool faceRight)> callback) {
    m_throwCallback = callback;
}

void HammerThrowStrategy::setThrowCallbackVel(std::function<void(sf::Vector2f position, sf::Vector2f velocity)> callback) {
    m_throwVelCallback = callback;
}

void HammerThrowStrategy::calculateTarget(Enemy& enemy, float dt) {
    Player* player = Game::getInstance().getNearestPlayer(enemy.getPosition());
    if (player) {
        enemy.facingRight = (player->position.x > enemy.position.x);
    }

    // Update throwing timer
    m_throwCooldownTimer -= dt;
    if (m_throwCooldownTimer <= 0.0f) {
        sf::Vector2f throwPos = enemy.position + sf::Vector2f(enemy.boundingBox.width / 2.0f, -8.0f);
        
        // Calculate dynamic parabolic arc throw velocity targeting player position
        sf::Vector2f throwVel(enemy.facingRight ? 220.0f : -220.0f, -380.0f);
        if (player) {
            float dx = player->position.x - throwPos.x;
            float dy = player->position.y - throwPos.y;
            float vy0 = -380.0f;
            float g = 1200.0f;

            // Solve kinematic quadratic: 0.5 * g * T^2 + vy0 * T - dy = 0
            float discriminant = vy0 * vy0 + 2.0f * g * dy;
            float flightTime = 0.633f;
            if (discriminant >= 0.0f) {
                float T = (-vy0 + std::sqrt(discriminant)) / g;
                if (T > 0.1f) flightTime = T;
            }

            float targetVx = dx / flightTime;

            // Cap speed so hammers maintain an authentic parabolic arc without becoming laser missiles
            float maxVx = 400.0f;
            float minVx = 80.0f;
            targetVx = std::clamp(targetVx, -maxVx, maxVx);
            if (std::abs(targetVx) < minVx) {
                targetVx = (targetVx >= 0.0f) ? minVx : -minVx;
            }
            throwVel.x = targetVx;

            // Apply 10% inaccuracy variation to both horizontal and vertical launch velocities
            float variationVx = 1.0f + ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.20f - 0.10f);
            float variationVy = 1.0f + ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.20f - 0.10f);
            throwVel.x *= variationVx;
            throwVel.y *= variationVy;
        }

        if (m_throwVelCallback) {
            m_throwVelCallback(throwPos, throwVel);
        } else if (m_throwCallback) {
            m_throwCallback(throwPos, enemy.facingRight);
        }
        m_throwCooldownTimer = m_throwCooldownMax;
    }

    // Update jumping timer
    m_jumpCooldownTimer -= dt;
    if (m_jumpCooldownTimer <= 0.0f) {
        if (enemy.onGround) {
            enemy.velocity.y = -350.0f; // Jump upward force
            enemy.onGround = false;
        }
        m_jumpCooldownTimer = m_jumpCooldownMax;
    }
}

void HammerThrowStrategy::applyMovement(Enemy& enemy, float dt) {
    // Standard Hammer Bro shuffle back and forth
    float shuffleDir = (std::sin(m_jumpCooldownTimer * 2.0f) > 0.0f) ? 1.0f : -1.0f;
    enemy.velocity.x = shuffleDir * 30.0f;
}
