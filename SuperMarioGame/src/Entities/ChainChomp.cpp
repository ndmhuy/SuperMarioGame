#include "Entities/ChainChomp.hpp"
#include "Entities/TetheredChaseStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

ChainChomp::ChainChomp(sf::Vector2f position)
    : Enemy(position, 0) {
    boundingBox.width = 32.0f;
    boundingBox.height = 32.0f;
    
    // Set AI tethered chase strategy anchored at spawn post position
    setStrategy(std::make_unique<TetheredChaseStrategy>(position));
}

void ChainChomp::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("chain_chomp");
    m_animation.frameList = {{"chained_chomp_head_left_0", 0.15f}, {"chained_chomp_head_left_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void ChainChomp::onStomped() {
    // Spiky/Iron ball: cannot be stomped, inflicts damage
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        player->takeDamage(1);
    }
}

void ChainChomp::onHitByFireball() {
    // Immune to fireball
}
