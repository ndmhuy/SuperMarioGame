#include "Entities/Block.hpp"
#include <cmath>

Block::Block(sf::Vector2f position) {
    this->position = position;
    this->m_originalPosition = position;
    this->boundingBox.x = position.x;
    this->boundingBox.y = position.y;
    this->boundingBox.width = 32.0f;  // Standard tile size
    this->boundingBox.height = 32.0f;
    this->active = true;
}

void Block::update(float dt) {
    if (m_isHit && m_bumpTimer > 0.0f) {
        m_bumpTimer -= dt;
        if (m_bumpTimer <= 0.0f) {
            m_bumpTimer = 0.0f;
            m_isHit = false;
            position.y = m_originalPosition.y;
        } else {
            // Parabolic bump offset: peak at t = 0.5 (midpoint of 0.15s duration)
            float t = m_bumpTimer / 0.15f; // 1.0 down to 0.0
            float offset = -8.0f * (4.0f * t * (1.0f - t)); // Peak offset of -8 pixels
            position.y = m_originalPosition.y + offset;
        }
        boundingBox.y = position.y;
    }
}
