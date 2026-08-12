#include "Entities/Goomba.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Utils/Constants.hpp"
#include "Core/EventBus.hpp"

Goomba::Goomba(sf::Vector2f position, bool isRed)
    : Enemy(position, 100), m_isRed(isRed) {
    speed = Constants::ENEMY_GOOMBA_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };
    
    // Set PatrolStrategy: ledge-aware if red goomba
    setStrategy(std::make_unique<PatrolStrategy>(m_isRed, false));
}

void Goomba::update(float dt) {
    if (m_isSquished) {
        velocity = sf::Vector2f(0.0f, 0.0f);
        m_squishTimer -= dt;
        if (m_squishTimer <= 0.0f) {
            destroy();
        }
    } else if (m_isFlipped) {
        // Fall off screen: gravity in px/s^2 (GRAVITY = 0.5 px/frame^2 = 1800 px/s^2)
        velocity.y += 1800.0f * dt;
        position += velocity * dt;
        if (position.y > Constants::WINDOW_HEIGHT + 100.0f) {
            destroy();
        }
    } else {
        Enemy::update(dt);
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }
}

void Goomba::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("goomba_move");
    m_animation.frameList = {{"goomba_brown_move_0", 0.15f}, {"goomba_brown_move_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Goomba::render(sf::RenderTarget& target) {
    Enemy::render(target);
}

void Goomba::onStomped() {
    if (m_isSquished || m_isFlipped) return;
    
    m_isSquished = true;
    m_squishTimer = Constants::GOOMBA_SQUISH_DURATION;
    velocity = sf::Vector2f(0.0f, 0.0f);
    
    // Publish EnemyDefeated event
    GameEvent event;
    event.type = EventType::EnemyDefeated;
    event.data = m_scoreValue;
    EventBus::getInstance().publish(event);
}

void Goomba::onHitByFireball() {
    if (m_isSquished || m_isFlipped) return;
    
    m_isFlipped = true;
    velocity = sf::Vector2f(100.0f, -300.0f); // Fly up and forward
    
    // Publish EnemyDefeated event
    GameEvent event;
    event.type = EventType::EnemyDefeated;
    event.data = m_scoreValue;
    EventBus::getInstance().publish(event);
}

const AABB& Goomba::getBoundingBox() const {
    if (m_isSquished || m_isFlipped) {
        static const AABB emptyBox{ 0.0f, 0.0f, 0.0f, 0.0f };
        return emptyBox;
    }
    return boundingBox;
}
