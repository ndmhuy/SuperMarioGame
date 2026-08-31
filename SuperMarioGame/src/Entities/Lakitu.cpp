#include "Entities/Lakitu.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/EntityFactory.hpp"
#include "Utils/Constants.hpp"

Lakitu::Lakitu(sf::Vector2f position)
    : Enemy(position, 800) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_LAKITU_SPEED;
    setTargetSize({32.0f, 32.0f});

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

    // Drop a Spiny on a timer. This used to increment a counter and play a
    // sound without ever creating anything, so Lakitu was a hovering sprite
    // (audit B-7). Entities cannot reach the world's entity list, so it asks.
    if (m_spawnCount >= MAX_SPINIES) return;

    m_eggTimer += dt;
    if (m_eggTimer >= 4.0f) {
        m_eggTimer = 0.0f;
        m_spawnCount++;

        EntitySpawnRequest request;
        request.type = static_cast<int>(EntityType::Spiny);
        request.position = position + sf::Vector2f(0.0f, Constants::TILE_SIZE);
        request.velocity = sf::Vector2f(facingRight ? 40.0f : -40.0f, 0.0f);
        EventBus::getInstance().publish({EventType::EntitySpawnRequested, request});

        SoundManager::getInstance().playSound("kick");
    }
}
