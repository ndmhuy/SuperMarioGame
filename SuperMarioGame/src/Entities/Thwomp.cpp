#include "Entities/Thwomp.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

Thwomp::Thwomp(sf::Vector2f position)
    : Enemy(position, 0, {48.0f, 64.0f}) {
    // Proximity trigger slam strategy
    setStrategy(std::make_unique<ProximityTriggerStrategy>());
}

void Thwomp::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("thwomp");
    m_animation.frameList = {{"thwomper_dormant", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
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
