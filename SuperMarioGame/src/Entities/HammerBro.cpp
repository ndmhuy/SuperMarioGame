#include "Entities/HammerBro.hpp"
#include "Entities/HammerThrowStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

HammerBro::HammerBro(sf::Vector2f position)
    : Enemy(position, 1000) {
    boundingBox.width = 32.0f;
    boundingBox.height = 48.0f;
    
    // Set AI platform patrolling and hammer throwing strategy
    setStrategy(std::make_unique<HammerThrowStrategy>());
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
