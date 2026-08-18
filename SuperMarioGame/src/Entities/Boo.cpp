#include "Entities/Boo.hpp"
#include "Entities/ChaseStrategy.hpp"
#include "Utils/Constants.hpp"

Boo::Boo(sf::Vector2f position)
    : Enemy(position, 0) { // Boo gives 0 points
    speed = Constants::BOO_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };
    
    setStrategy(std::make_unique<ChaseStrategy>());
}

void Boo::update(float dt) {
    Enemy::update(dt);
    boundingBox.x = position.x;
    boundingBox.y = position.y;

    if (m_animator) {
        if (velocity.x == 0.0f && velocity.y == 0.0f) {
            m_animator->play(&m_seenAnim);
        } else {
            m_animator->play(&m_moveAnim);
        }
    }
}

void Boo::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_seenAnim = Animation("boo_seen");
    m_seenAnim.frameList = {{"boo_seen_0", 0.15f}};
    m_moveAnim = Animation("boo_move");
    m_moveAnim.frameList = {{"boo_move_0", 0.15f}, {"boo_move_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_moveAnim);
        m_hasAnimation = true;
    }
}

void Boo::render(sf::RenderTarget& target) {
    Enemy::render(target);
}

void Boo::onStomped() {
    // Boo is invulnerable to stomp
}

void Boo::onHitByFireball() {
    // Boo is invulnerable to fireballs
}
