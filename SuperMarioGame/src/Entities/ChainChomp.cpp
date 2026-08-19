#include "Entities/ChainChomp.hpp"
#include "Utils/Constants.hpp"
#include "Entities/TetheredChaseStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

ChainChomp::ChainChomp(sf::Vector2f position)
    : Enemy(position, 0) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_CHAIN_CHOMP_SPEED;
    setTargetSize({32.0f, 32.0f});
    
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
