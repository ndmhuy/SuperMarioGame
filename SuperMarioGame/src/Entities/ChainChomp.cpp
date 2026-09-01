#include "Entities/ChainChomp.hpp"
#include "Utils/Constants.hpp"
#include "Entities/TetheredChaseStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <cmath>

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
    Player* player = Game::getInstance().getNearestPlayer(getPosition());
    if (player) {
        player->takeDamage(1);
    }
}

void ChainChomp::onHitByFireball() {
    // Immune to fireball
}

bool ChainChomp::onPlayerTouch(Player& player, const CollisionInfo& info, bool stomped) {
    (void)info;
    (void)stomped;
    // Chain Chomp is an iron ball with sharp teeth: cannot be stomped, inflicts damage
    if (player.getInvincibilityTimer() > 0.0f) return true;

    // 1. Inflict damage on player
    player.takeDamage(1);

    // 2. Knockback player away from Chain Chomp
    float dx = player.getBoundingBox().getCenter().x - getBoundingBox().getCenter().x;
    float dir = (dx >= 0.0f) ? 1.0f : -1.0f;
    player.setVelocity(sf::Vector2f(dir * Constants::KNOCKBACK_FORCE_X, -Constants::KNOCKBACK_FORCE_Y));

    // 3. Knockback Chain Chomp away from player / towards anchor
    sf::Vector2f recoilDir = getBoundingBox().getCenter() - player.getBoundingBox().getCenter();
    float len = std::sqrt(recoilDir.x * recoilDir.x + recoilDir.y * recoilDir.y);
    if (len > 0.01f) {
        recoilDir /= len;
    } else {
        recoilDir = sf::Vector2f(-dir, -0.5f);
    }

    if (auto* tether = dynamic_cast<TetheredChaseStrategy*>(getStrategy())) {
        tether->triggerRecoil(recoilDir);
    } else {
        velocity = recoilDir * 160.0f;
    }

    return true; // Handled
}
