#include "Entities/BulletBill.hpp"
#include "Utils/Constants.hpp"
#include "Entities/LinearStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

BulletBill::BulletBill(sf::Vector2f position, float dirX)
    : Enemy(position, 200) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_BULLET_BILL_SPEED;
    setTargetSize({32.0f, 32.0f});
    
    // Straight line movement horizontally
    setStrategy(std::make_unique<LinearStrategy>(200.0f, sf::Vector2f(dirX, 0.0f)));
}

void BulletBill::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("bullet_bill");
    m_animation.frameList = {{"bullet_bill_bullet_left", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void BulletBill::onStomped() {
    SoundManager::getInstance().playSound("stomp");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    triggerFlipDeath({100.0f, -300.0f});
}

void BulletBill::onHitByFireball() {
    // Immune to fireball
}
