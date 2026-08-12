#include "Entities/MovingPlatform.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MovingPlatform::MovingPlatform(sf::Vector2f position, sf::Vector2f travelRange, float speed)
    : Block(position, {64.0f, 16.0f}), m_startPos(position), m_travelRange(travelRange), m_speed(speed),
      m_rangeLen(std::sqrt(m_travelRange.x * m_travelRange.x + m_travelRange.y * m_travelRange.y)) {
    m_breakable = false;
}

void MovingPlatform::onHitFromBelow(Player& player) {
    // Moving platform hit from below: standard bump behavior or nothing
}

void MovingPlatform::update(float dt) {
    sf::Vector2f oldPos = position;
    sf::Vector2f newPos = position;

    if (m_rangeLen > 0.0f) {
        float speedFraction = m_speed / m_rangeLen;
        if (m_movingForward) {
            m_progress += speedFraction * dt;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_movingForward = false;
            }
        } else {
            m_progress -= speedFraction * dt;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_movingForward = true;
            }
        }
        newPos = m_startPos + m_travelRange * m_progress;
        
        // Update velocity representation
        sf::Vector2f dir = m_travelRange / m_rangeLen;
        velocity = dir * (m_movingForward ? m_speed : -m_speed);
    } else {
        velocity = sf::Vector2f(0.0f, 0.0f);
    }

    sf::Vector2f displacement = newPos - oldPos;
    setPosition(newPos);

    // Apply carrying logic if player is standing on top
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        AABB pBox = player->getBoundingBox();
        AABB platBox = getBoundingBox();

        // 1. Horizontal overlap check
        bool xOverlap = (pBox.x + pBox.width > platBox.x) && (pBox.x < platBox.x + platBox.width);
        
        // 2. Vertical bottom edge close to platform top check (3px tolerance)
        bool yOverlap = std::abs((pBox.y + pBox.height) - platBox.y) < 3.0f;
        
        // 3. Downward/horizontal movement or resting on platform
        bool resting = player->getVelocity().y >= 0.0f;

        if (xOverlap && yOverlap && resting) {
            player->setPosition(player->getPosition() + displacement);
        }
    }
}

void MovingPlatform::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("moving_platform");
    m_animation.frameList = {{"platform_medium", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void MovingPlatform::render(sf::RenderTarget& target) {
    Block::render(target);
}
