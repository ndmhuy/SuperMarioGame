#include "Entities/Thwomp.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

Thwomp::Thwomp(sf::Vector2f position)
    : Enemy(position, 0, {48.0f, 64.0f}) {
    // Proximity trigger slam strategy
    setStrategy(std::make_unique<ProximityTriggerStrategy>());
}

void Thwomp::update(float dt) {
    Enemy::update(dt);
    if (m_animator) {
        if (velocity.y > 0.0f || (position.y > 140.0f && velocity.y == 0.0f)) {
            m_animator->play(&m_activeAnim);
        } else {
            m_animator->play(&m_dormantAnim);
        }
    }
}

void Thwomp::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_dormantAnim = Animation("thwomp_dormant");
    m_dormantAnim.frameList = {{"thwomper_dormant", 0.15f}};
    m_activeAnim = Animation("thwomp_active");
    m_activeAnim.frameList = {{"thwomper_active", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_dormantAnim);
        m_hasAnimation = true;
    }
}

void Thwomp::onStomped() {
    // Stone: cannot be stomped, inflicts damage
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        player->takeDamage(1);
    }
}

void Thwomp::onHitByFireball() {
    // Immune to fireball
}
