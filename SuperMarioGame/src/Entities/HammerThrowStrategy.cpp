#include "Entities/HammerThrowStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

HammerThrowStrategy::HammerThrowStrategy(float throwCooldown, float jumpCooldown)
    : m_throwCooldownTimer(throwCooldown),
      m_jumpCooldownTimer(jumpCooldown),
      m_throwCooldownMax(throwCooldown),
      m_jumpCooldownMax(jumpCooldown) {}

void HammerThrowStrategy::setThrowCallback(std::function<void(sf::Vector2f position, bool faceRight)> callback) {
    m_throwCallback = callback;
}

void HammerThrowStrategy::calculateTarget(Enemy& enemy, float dt) {
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        enemy.facingRight = (player->position.x > enemy.position.x);
    }

    // Update throwing timer
    m_throwCooldownTimer -= dt;
    if (m_throwCooldownTimer <= 0.0f) {
        if (m_throwCallback) {
            sf::Vector2f throwPos = enemy.position + sf::Vector2f(enemy.boundingBox.width / 2.0f, -8.0f);
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
