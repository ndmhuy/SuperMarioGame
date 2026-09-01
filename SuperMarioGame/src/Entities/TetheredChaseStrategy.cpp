#include "Entities/TetheredChaseStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

TetheredChaseStrategy::TetheredChaseStrategy(sf::Vector2f anchorPos, float tetherRadius)
    : m_anchorPos(anchorPos),
      m_tetherRadius(tetherRadius),
      m_timer(0.0f),
      m_anchorInitialized(anchorPos != sf::Vector2f(0.f, 0.f)),
      m_isLunging(false),
      m_lungeDir(0.f, 0.f),
      m_lungeTimer(0.0f),
      m_cooldownTimer(0.0f),
      m_recoilTimer(0.0f) {}

sf::Vector2f TetheredChaseStrategy::getAnchorPos() const {
    return m_anchorPos;
}

void TetheredChaseStrategy::setAnchorPos(sf::Vector2f anchorPos) {
    m_anchorPos = anchorPos;
    m_anchorInitialized = true;
}

float TetheredChaseStrategy::getTetherRadius() const {
    return m_tetherRadius;
}

void TetheredChaseStrategy::setTetherRadius(float radius) {
    m_tetherRadius = radius;
}

void TetheredChaseStrategy::triggerRecoil(sf::Vector2f recoilDir) {
    m_isLunging = false;
    m_lungeTimer = 0.0f;
    m_recoilTimer = 0.35f;
    m_cooldownTimer = 1.5f;
    m_lungeDir = recoilDir;
}

void TetheredChaseStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_anchorInitialized) {
        m_anchorPos = enemy.position;
        m_anchorInitialized = true;
    }

    if (m_recoilTimer > 0.0f) {
        m_recoilTimer -= dt;
        return;
    }

    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer -= dt;
    }

    if (m_isLunging) {
        m_lungeTimer -= dt;
        if (m_lungeTimer <= 0.0f) {
            m_isLunging = false;
            m_cooldownTimer = 1.2f; // Rest between lunges
        }
        return;
    }

    Player* player = Game::getInstance().getNearestPlayer(enemy.getPosition());
    if (player) {
        float dx = player->position.x - enemy.position.x;
        float dy = player->position.y - enemy.position.y;
        float distToPlayer = std::sqrt(dx * dx + dy * dy);

        float adx = player->position.x - m_anchorPos.x;
        float ady = player->position.y - m_anchorPos.y;
        float distToAnchor = std::sqrt(adx * adx + ady * ady);

        // Turn to face the player when anywhere in range
        if (distToPlayer < 350.0f) {
            enemy.facingRight = (dx >= 0.0f);
        }

        // If player is within approach radius and cooldown expired, trigger a forward lunge
        if (distToAnchor <= m_tetherRadius + 60.0f && m_cooldownTimer <= 0.0f) {
            if (distToPlayer > 0.01f) {
                m_lungeDir = sf::Vector2f(dx / distToPlayer, dy / distToPlayer);
            }
            m_isLunging = true;
            m_lungeTimer = 0.55f; // 0.55s lunge forward
        }
    }
}

void TetheredChaseStrategy::applyMovement(Enemy& enemy, float dt) {
    m_timer += dt;

    if (m_recoilTimer > 0.0f) {
        // Recoil back from hit
        enemy.velocity = m_lungeDir * 160.0f;
        enemy.facingRight = (enemy.velocity.x > 0.f);
        return;
    }

    Player* player = Game::getInstance().getNearestPlayer(enemy.getPosition());
    if (m_isLunging) {
        // Fast aggressive bite lunge towards player
        const float lungeSpeed = (enemy.speed > 0.0f ? enemy.speed : 90.0f) * 1.4f;
        enemy.velocity = m_lungeDir * lungeSpeed;
        enemy.facingRight = (m_lungeDir.x >= 0.f);
    } else if (player) {
        float dx = player->position.x - enemy.position.x;
        float dy = player->position.y - enemy.position.y;
        float distToPlayer = std::sqrt(dx * dx + dy * dy);

        float adx = player->position.x - m_anchorPos.x;
        float ady = player->position.y - m_anchorPos.y;
        float distToAnchor = std::sqrt(adx * adx + ady * ady);

        if (distToAnchor <= m_tetherRadius + 100.0f && distToPlayer > 8.0f) {
            // Actively chase / follow and strain toward the player at steady speed (~55 px/s)
            sf::Vector2f dir(dx / distToPlayer, dy / distToPlayer);
            enemy.velocity = dir * 55.0f;
            enemy.facingRight = (dx >= 0.f);
        } else {
            // Gentle idle bobbing near anchor
            enemy.velocity.x = std::cos(m_timer * 2.0f) * 20.0f;
            enemy.velocity.y = std::sin(m_timer * 4.0f) * 10.0f;
            enemy.facingRight = (enemy.velocity.x >= 0.f);
        }
    } else {
        // Gentle idle bobbing near anchor
        enemy.velocity.x = std::cos(m_timer * 2.0f) * 20.0f;
        enemy.velocity.y = std::sin(m_timer * 4.0f) * 10.0f;
        enemy.facingRight = (enemy.velocity.x >= 0.f);
    }
}

void TetheredChaseStrategy::checkConstraints(Enemy& enemy, float dt) {
    (void)dt;
    sf::Vector2f relativePos = enemy.position - m_anchorPos;
    float currentDist = std::sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);

    if (currentDist > m_tetherRadius && currentDist > 0.01f) {
        // Clamp position to max tether radius
        enemy.position = m_anchorPos + (relativePos / currentDist) * m_tetherRadius;
        
        // Rebound back toward anchor when chain goes taut
        enemy.velocity = -(relativePos / currentDist) * 50.0f;
        if (m_isLunging) {
            m_isLunging = false;
            m_lungeTimer = 0.0f;
            m_cooldownTimer = 1.0f;
        }
    }
}
