#include "Entities/HammerBro.hpp"
#include "Entities/HammerThrowStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

HammerBro::HammerBro(sf::Vector2f position)
    : Enemy(position, 1000, {32.0f, 48.0f}) {
    // Set AI platform patrolling and hammer throwing strategy
    setStrategy(std::make_unique<HammerThrowStrategy>());
}

void HammerBro::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("hammer_bro");
    m_animation.frameList = {{"hammer_bros_green_move_left_0", 0.15f}, {"hammer_bros_green_move_left_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void HammerBro::onStomped() {
    SoundManager::getInstance().playSound("stomp");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}

void HammerBro::onHitByFireball() {
    SoundManager::getInstance().playSound("kick");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}
