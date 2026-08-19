#include "Entities/ChaseStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

void ChaseStrategy::calculateTarget(Enemy& enemy, float dt) {
    m_shouldChase = false;
    Player* player = Game::getInstance().getPlayer();
    if (!player) {
        return;
    }

    m_targetDx = player->position.x - enemy.position.x;
    m_targetDy = player->position.y - enemy.position.y;
    m_targetDist = std::sqrt(m_targetDx * m_targetDx + m_targetDy * m_targetDy);

    if (m_targetDist <= 250.0f && m_targetDist > 0.01f) {
        bool playerToLeft = (player->position.x < enemy.position.x);
        bool playerFacingEnemy = (playerToLeft && player->facingRight) || (!playerToLeft && !player->facingRight);
        
        if (!playerFacingEnemy) {
            m_shouldChase = true;
        }
    }
}

void ChaseStrategy::applyMovement(Enemy& enemy, float dt) {
    if (m_shouldChase && m_targetDist > 0.01f) {
        const float chaseSpeed = enemy.speed;
        sf::Vector2f dir(m_targetDx / m_targetDist, m_targetDy / m_targetDist);
        enemy.velocity = dir * chaseSpeed;
        enemy.facingRight = (enemy.velocity.x > 0.0f);
    } else {
        enemy.velocity = sf::Vector2f(0.0f, 0.0f);
    }
}
