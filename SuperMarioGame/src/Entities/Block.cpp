#include "Entities/Block.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>
#include <algorithm>

Block::Block(sf::Vector2f position, sf::Vector2f targetSize) : Entity(position, targetSize) {
    this->m_originalPosition = position;
    this->active = true;
}

BlockTouch Block::onCharacterTouch(Character& character, const CollisionInfo& info) {
    // An ordinary block is solid from every direction.
    (void)character;
    (void)info;
    return BlockTouch::Solid;
}

void Block::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
}

void Block::update(float dt) {
    if (m_isHit && m_bumpTimer > 0.0f) {
        m_bumpTimer -= dt;
        if (m_bumpTimer <= 0.0f) {
            m_bumpTimer = 0.0f;
            m_isHit = false;
            position.y = m_originalPosition.y;
        } else {
            // Parabolic bump offset: moves up 8px over 0.1s and returns over 0.1s (total 0.20s duration)
            float t = m_bumpTimer / 0.20f; // 1.0 down to 0.0
            float offset = -8.0f * (4.0f * t * (1.0f - t)); // Peak offset of -8 pixels at t = 0.5 (0.1s)
            position.y = m_originalPosition.y + offset;
        }
        boundingBox.y = position.y;
    }
    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}

void Block::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        drawSprite(target, m_animator->getSprite(), SpriteAnchor::TopLeft);
    } else {
        drawPlaceholder(target, sf::Color(180, 100, 30));
    }
}
