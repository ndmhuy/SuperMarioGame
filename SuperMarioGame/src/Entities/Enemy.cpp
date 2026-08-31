#include "Entities/Enemy.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>

#include "Utils/Constants.hpp"
#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"

float Enemy::fallDespawnPlaneY() {
    TileMap* tileMap = Game::getInstance().getTileMap();
    if (!tileMap) return Constants::WINDOW_HEIGHT + 100.0f;
    return (tileMap->getHeight() * Constants::TILE_SIZE) + 100.0f;
}

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
        if (position.y > fallDespawnPlaneY()) {
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
        // Sprites face left in the atlas, so facingRight is the flip case.
        drawSprite(target, m_animator->getSprite(), SpriteAnchor::BottomCenter,
                   /*flipX=*/facingRight, /*flipY=*/m_isFlipped);
    } else {
        drawPlaceholder(target, sf::Color::Red);
    }
}


void Enemy::setSpeed(float newSpeed) {
    if (newSpeed > 0.0f) speed = newSpeed;
}

void Enemy::applySpeedScale(float scale) {
    if (scale <= 0.0f) return;
    speed *= scale;
}

void Enemy::setStrategy(std::unique_ptr<IMovementStrategy> strategy) {
    m_aiStrategy = std::move(strategy);
}

IMovementStrategy* Enemy::getStrategy() const {
    return m_aiStrategy.get();
}

bool Enemy::onPlayerTouch(Player& player, const CollisionInfo& info, bool stomped) {
    // Most enemies have nothing special to say: stomp them and they die, touch
    // them any other way and they hurt you. The resolver owns both rules.
    (void)player;
    (void)info;
    (void)stomped;
    return false;
}

int Enemy::getScoreValue() const {
    return m_scoreValue;
}

void Enemy::setScoreValue(int value) {
    m_scoreValue = value;
}

bool Enemy::isCollidable() const {
    // Was a zero-sized AABB returned from getBoundingBox() (audit B-14).
    return !isDeadOrDying();
}
