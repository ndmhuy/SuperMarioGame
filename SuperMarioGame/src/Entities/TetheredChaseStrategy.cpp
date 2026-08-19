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
      m_lungeDir(0.f, 0.f) {}

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

void TetheredChaseStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_anchorInitialized) {
        m_anchorPos = enemy.position;
        m_anchorInitialized = true;
    }

    Player* player = Game::getInstance().getPlayer();
    if (player) {
        // Calculate player distance to anchor post
        float dx = player->position.x - m_anchorPos.x;
        float dy = player->position.y - m_anchorPos.y;
        float distToAnchor = std::sqrt(dx * dx + dy * dy);

        // If player is close to anchor, prepare a lunge
        if (distToAnchor <= 150.0f) {
            m_isLunging = true;
            float edx = player->position.x - enemy.position.x;
            float edy = player->position.y - enemy.position.y;
            float edist = std::sqrt(edx * edx + edy * edy);
            if (edist > 0.01f) {
                m_lungeDir = sf::Vector2f(edx / edist, edy / edist);
            } else {
                m_lungeDir = sf::Vector2f(0.f, 0.f);
            }
        } else {
            m_isLunging = false;
        }
    } else {
        m_isLunging = false;
    }
}

void TetheredChaseStrategy::applyMovement(Enemy& enemy, float dt) {
    m_timer += dt;

    if (m_isLunging) {
        // Was a literal 250; Chain Chomp now carries it, so difficulty scales it.
        const float lungeSpeed = enemy.speed > 0.0f ? enemy.speed : 250.0f;
        enemy.velocity = m_lungeDir * lungeSpeed;
        enemy.facingRight = (enemy.velocity.x > 0.f);
    } else {
        // Wander around anchor
        enemy.velocity.x = std::cos(m_timer * 4.0f) * 40.0f;
        enemy.velocity.y = std::sin(m_timer * 8.0f) * 20.0f;
        enemy.facingRight = (enemy.velocity.x > 0.f);
    }
}

void TetheredChaseStrategy::checkConstraints(Enemy& enemy, float dt) {
    sf::Vector2f relativePos = enemy.position - m_anchorPos;
    float currentDist = std::sqrt(relativePos.x * relativePos.x + relativePos.y * relativePos.y);

    if (currentDist > m_tetherRadius && currentDist > 0.01f) {
        // Clamp position to max tether radius
        enemy.position = m_anchorPos + (relativePos / currentDist) * m_tetherRadius;
        
        // Bounce back with dampening when reaching the chain limit
        enemy.velocity = -enemy.velocity * 0.5f;
    }
}
