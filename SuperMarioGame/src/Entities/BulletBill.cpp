#include "Entities/BulletBill.hpp"
#include "Entities/LinearStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

BulletBill::BulletBill(sf::Vector2f position, float dirX)
    : Enemy(position, 200) {
    boundingBox.width = 32.0f;
    boundingBox.height = 32.0f;
    
    // Straight line movement horizontally
    setStrategy(std::make_unique<LinearStrategy>(200.0f, sf::Vector2f(dirX, 0.0f)));
}

void BulletBill::onStomped() {
    SoundManager::getInstance().playSound("stomp");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}

void BulletBill::onHitByFireball() {
    // Immune to fireball
}
