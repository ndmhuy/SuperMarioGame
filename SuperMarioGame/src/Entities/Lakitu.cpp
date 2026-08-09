#include "Entities/Lakitu.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

Lakitu::Lakitu(sf::Vector2f position)
    : Enemy(position, 800) {
    boundingBox.width = 32.0f;
    boundingBox.height = 32.0f;

    // Lakitu hovers and follows player horizontally
    setStrategy(std::make_unique<FlyStrategy>(FlyMode::FollowPlayer));
}

void Lakitu::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("lakitu");
    m_animation.frameList = {{"lakitu_left", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Lakitu::onStomped() {
    SoundManager::getInstance().playSound("stomp");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}

void Lakitu::onHitByFireball() {
    SoundManager::getInstance().playSound("kick");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}

void Lakitu::update(float dt) {
    // Execute FlyStrategy to update velocity
    Enemy::update(dt);

    // Synchronize bounding box with position
    boundingBox.x = position.x;
    boundingBox.y = position.y;

    // Update egg timer
    m_eggTimer += dt;
    if (m_eggTimer >= 4.0f) {
        m_eggTimer = 0.0f;
        m_spawnCount++;
        SoundManager::getInstance().playSound("kick"); // or throw sound
    }
}
