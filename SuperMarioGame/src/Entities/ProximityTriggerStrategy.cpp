#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include <cmath>

ProximityTriggerStrategy::ProximityTriggerStrategy(sf::Vector2f homePos)
    : m_homePos(homePos),
      m_state(ProximityState::Idle),
      m_timer(0.0f),
      m_homeInitialized(homePos != sf::Vector2f(0.f, 0.f)) {}

sf::Vector2f ProximityTriggerStrategy::getHomePos() const {
    return m_homePos;
}

void ProximityTriggerStrategy::setHomePos(sf::Vector2f homePos) {
    m_homePos = homePos;
    m_homeInitialized = true;
}

ProximityState ProximityTriggerStrategy::getState() const {
    return m_state;
}

void ProximityTriggerStrategy::setState(ProximityState state) {
    m_state = state;
}

std::string ProximityTriggerStrategy::getDebugState() const {
    switch (m_state) {
        case ProximityState::Idle:     return "Idle";
        case ProximityState::Slamming: return "Slamming";
        case ProximityState::Resting:  return "Resting";
        case ProximityState::Rising:   return "Rising";
    }
    return "?";
}

void ProximityTriggerStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_homeInitialized) {
        m_homePos = enemy.position;
        m_homeInitialized = true;
    }

    if (m_state == ProximityState::Idle) {
        Player* player = Game::getInstance().getNearestPlayer(enemy.getPosition());
        if (player) {
            const AABB& eBox = enemy.getBoundingBox();
            const AABB& pBox = player->getBoundingBox();

            // Slam down only when player is directly below the Thwomp (overlapping column)
            bool hOverlap = (pBox.x + pBox.width >= eBox.x - 4.0f) && (pBox.x <= eBox.x + eBox.width + 4.0f);
            bool playerBelow = (pBox.y + 4.0f >= eBox.y);

            if (hOverlap && playerBelow) {
                m_state = ProximityState::Slamming;
                m_timer = 0.0f;
            }
        }
    }
    else if (m_state == ProximityState::Slamming) {
        m_timer += dt;
        // In case there is no ground in broadphase/test, transition after 1.5s
        if (enemy.onGround || m_timer >= 1.5f) {
            m_state = ProximityState::Resting;
            m_timer = 0.0f;
            enemy.onGround = false; // Reset flag
            EventBus::getInstance().publish({EventType::ThwompSlam, &enemy});
        }
    }
    else if (m_state == ProximityState::Resting) {
        m_timer += dt;
        if (m_timer >= 1.0f) { // Rest for 1 second
            m_state = ProximityState::Rising;
            m_timer = 0.0f;
        }
    }
    else if (m_state == ProximityState::Rising) {
        if (enemy.position.y <= m_homePos.y) {
            enemy.position.y = m_homePos.y;
            m_state = ProximityState::Idle;
            m_timer = 0.0f;
        }
    }
}

void ProximityTriggerStrategy::applyMovement(Enemy& enemy, float dt) {
    switch (m_state) {
        case ProximityState::Idle:
        case ProximityState::Resting:
            enemy.velocity = sf::Vector2f(0.0f, 0.0f);
            break;
        case ProximityState::Slamming:
            // 75 px/s slam, carried by the Thwomp itself (halved from 150).
            enemy.velocity = sf::Vector2f(0.0f, enemy.speed > 0.0f ? enemy.speed : 75.0f);
            break;
        case ProximityState::Rising:
            enemy.velocity = sf::Vector2f(0.0f, -80.0f); // Faster recovery climb back home
            break;
    }
}

void ProximityTriggerStrategy::checkConstraints(Enemy& enemy, float dt) {
    // Prevent rising above home position
    if (m_state == ProximityState::Rising && enemy.position.y <= m_homePos.y) {
        enemy.position.y = m_homePos.y;
        enemy.velocity.y = 0.0f;
        m_state = ProximityState::Idle;
    }
}
