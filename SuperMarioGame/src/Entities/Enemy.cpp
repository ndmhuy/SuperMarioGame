#include "Entities/Enemy.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>

#include "Utils/Constants.hpp"

Enemy::Enemy(sf::Vector2f position, int scoreValue, sf::Vector2f targetSize)
    : Character(position, targetSize), m_scoreValue(scoreValue) {
}

void Enemy::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
}

void Enemy::triggerFlipDeath(sf::Vector2f launchVel) {
    if (m_isFlipped || m_isDyingDownward) return;
    m_isFlipped = true;
    velocity = launchVel;
}

void Enemy::triggerDownwardDeath(sf::Vector2f launchVel) {
    if (m_isFlipped || m_isDyingDownward) return;
    m_isDyingDownward = true;
    velocity = launchVel;
}

void Enemy::update(float dt) {
    if (m_isFlipped || m_isDyingDownward) {
        velocity.y += 3500.0f * dt; // Plunge down out of the world relatively fast
        position += velocity * dt;
        if (position.y > Constants::WINDOW_HEIGHT + 100.0f) {
            destroy();
        }
    } else {
        if (m_aiStrategy) {
            m_aiStrategy->execute(*this, dt);
        }
        if (m_animator && m_hasAnimation) {
            m_animator->update(dt);
        }
    }
}

void Enemy::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        sf::Sprite sprite = m_animator->getSprite();
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            float scale = std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);
            float scaledW = bounds.size.x * scale;
            float scaledH = bounds.size.y * scale;

            // Base AABB remains locked to m_targetSize during all animation frames
            boundingBox.width = m_targetSize.x;
            boundingBox.height = m_targetSize.y;

            // Set origin and position
            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
            float scaleX = facingRight ? -scale : scale; // horizontal flip if facing left/right
            float scaleY = m_isFlipped ? -scale : scale; // vertical flip if defeated/flipped
            sprite.setScale(sf::Vector2f(scaleX, scaleY));
            sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f, boundingBox.y + m_targetSize.y));

            target.draw(sprite);
        }
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color::Red);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}


void Enemy::setStrategy(std::unique_ptr<IMovementStrategy> strategy) {
    m_aiStrategy = std::move(strategy);
}

IMovementStrategy* Enemy::getStrategy() const {
    return m_aiStrategy.get();
}

int Enemy::getScoreValue() const {
    return m_scoreValue;
}

void Enemy::setScoreValue(int value) {
    m_scoreValue = value;
}

const AABB& Enemy::getBoundingBox() const {
    if (isDeadOrDying()) {
        static const AABB emptyBox{0.0f, 0.0f, 0.0f, 0.0f};
        return emptyBox;
    }
    return boundingBox;
}
