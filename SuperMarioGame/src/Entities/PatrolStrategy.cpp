#include "Entities/PatrolStrategy.hpp"
#include "Entities/Enemy.hpp"
#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"

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

    if (m_ledgeAware) {
        TileMap* tileMap = Game::getInstance().getTileMap();
        if (tileMap) {
            float checkOffset = m_movingRight ? (enemy.boundingBox.width + 4.0f) : -4.0f;
            float nextX = enemy.position.x + checkOffset;
            // Check just below the bottom of the bounding box
            float checkY = enemy.position.y + enemy.boundingBox.height + 4.0f;

            TileType currentUnderTile = tileMap->getTileAt(enemy.position.x + enemy.boundingBox.width / 2.0f, checkY);
            TileType nextTile = tileMap->getTileAt(nextX, checkY);

            // If we are currently on a solid tile but the next step is empty, reverse
            if (currentUnderTile != TileType::Empty && nextTile == TileType::Empty) {
                m_movingRight = !m_movingRight;
                enemy.facingRight = m_movingRight;
            }
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
