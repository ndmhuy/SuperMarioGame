#include "Entities/PatrolStrategy.hpp"
#include "Entities/Enemy.hpp"

PatrolStrategy::PatrolStrategy(bool ledgeAware, bool movingRight)
    : m_ledgeAware(ledgeAware), m_movingRight(movingRight) {}

bool PatrolStrategy::isLedgeAware() const {
    return m_ledgeAware;
}

void PatrolStrategy::setLedgeAware(bool ledgeAware) {
    m_ledgeAware = ledgeAware;
}

bool PatrolStrategy::isMovingRight() const {
    return m_movingRight;
}

void PatrolStrategy::setMovingRight(bool movingRight) {
    m_movingRight = movingRight;
}

void PatrolStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (enemy.onWall) {
        m_movingRight = !m_movingRight;
        enemy.onWall = false; // Reset wall flag after handling
        enemy.facingRight = m_movingRight;
    }

    // Airborne enemies have nothing under them by definition; testing the tile
    // below a falling enemy would flip it every frame of the fall — which is why
    // Enemy::hasFloorAhead() answers true when the enemy is not on solid ground
    // and this still gates on onGround as well.
    //
    // The probe itself used to be written out here, and being written out here
    // was the defect: it was the codebase's only ledge check, so nothing else
    // could use it (R21 D10). It now lives on Enemy, shared with
    // HammerThrowStrategy.
    if (m_ledgeAware && enemy.onGround) {
        if (!enemy.hasFloorAhead(m_movingRight ? 4.0f : -4.0f)) {
            m_movingRight = !m_movingRight;
            enemy.facingRight = m_movingRight;
        }
    }
}

void PatrolStrategy::applyMovement(Enemy& enemy, float dt) {
    // enemy.speed is the single source of truth now; every enemy sets it in its
    // constructor, so there is nothing to substitute for.
    const float patrolSpeed = enemy.speed;
    enemy.velocity.x = (m_movingRight ? 1.0f : -1.0f) * patrolSpeed;
    enemy.facingRight = m_movingRight;
}
