#include "Entities/Thwomp.hpp"
#include "Utils/Constants.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

Thwomp::Thwomp(sf::Vector2f position)
    : Enemy(position, 0, {48.0f, 64.0f}) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_THWOMP_SLAM_SPEED;

    // Proximity trigger slam strategy
    setStrategy(std::make_unique<ProximityTriggerStrategy>());
}

void Thwomp::update(float dt) {
    Enemy::update(dt);
    if (!m_animator) return;

    // Ask the state machine what it is doing. This used to infer the sprite from
    // `velocity.y > 0 || position.y > 140.0f` — a magic number that made a
    // Thwomp placed low in a level render as permanently slamming, and one in a
    // tall level never wake up (task 9.1).
    ProximityState state = ProximityState::Idle;
    if (const auto* proximity = dynamic_cast<const ProximityTriggerStrategy*>(getStrategy())) {
        state = proximity->getState();
    }

    const bool awake = (state == ProximityState::Slamming || state == ProximityState::Resting);
    m_animator->play(awake ? &m_activeAnim : &m_dormantAnim);
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
