#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

ProximityTriggerStrategy::ProximityTriggerStrategy(sf::Vector2f homePos)
    : m_homePos(homePos),
      m_state(0),
      m_timer(0.0f),
      m_homeInitialized(homePos != sf::Vector2f(0.f, 0.f)) {}

sf::Vector2f ProximityTriggerStrategy::getHomePos() const {
    return m_homePos;
}

void ProximityTriggerStrategy::setHomePos(sf::Vector2f homePos) {
    m_homePos = homePos;
    m_homeInitialized = true;
}

int ProximityTriggerStrategy::getState() const {
    return m_state;
}

void ProximityTriggerStrategy::setState(int state) {
    m_state = state;
}

void ProximityTriggerStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_homeInitialized) {
        m_homePos = enemy.position;
        m_homeInitialized = true;
    }

    if (m_state == 0) {
        Player* player = Game::getInstance().getPlayer();
        if (player) {
            float hDist = std::abs(player->position.x - enemy.position.x);
            bool playerBelow = (player->position.y > enemy.position.y);
            
            // Slam down if player walks beneath (horizontal distance <= 3 tiles / 96px)
            if (hDist <= 96.0f && playerBelow) {
                m_state = 1;
                m_timer = 0.0f;
            }
        }
    }
    else if (m_state == 1) {
        m_timer += dt;
        // In case there is no ground in broadphase/test, transition after 1.5s
        if (enemy.onGround || m_timer >= 1.5f) {
            m_state = 2;
            m_timer = 0.0f;
            enemy.onGround = false; // Reset flag
        }
    }
    else if (m_state == 2) {
        m_timer += dt;
        if (m_timer >= 1.0f) { // Rest for 1 second
            m_state = 3;
            m_timer = 0.0f;
        }
    }
    else if (m_state == 3) {
        if (enemy.position.y <= m_homePos.y) {
            enemy.position.y = m_homePos.y;
            m_state = 0;
            m_timer = 0.0f;
        }
    }
}

void ProximityTriggerStrategy::applyMovement(Enemy& enemy, float dt) {
    switch (m_state) {
        case 0: // Idle
            enemy.velocity = sf::Vector2f(0.0f, 0.0f);
            break;
        case 1: // Slamming
            enemy.velocity = sf::Vector2f(0.0f, 600.0f); // 600 px/s rapid slam
            break;
        case 2: // Resting
            enemy.velocity = sf::Vector2f(0.0f, 0.0f);
            break;
        case 3: // Rising
            enemy.velocity = sf::Vector2f(0.0f, -50.0f); // 50 px/s slow rise
            break;
    }
}

void ProximityTriggerStrategy::checkConstraints(Enemy& enemy, float dt) {
    // Prevent rising above home position
    if (m_state == 3 && enemy.position.y <= m_homePos.y) {
        enemy.position.y = m_homePos.y;
        enemy.velocity.y = 0.0f;
        m_state = 0;
    }
}
