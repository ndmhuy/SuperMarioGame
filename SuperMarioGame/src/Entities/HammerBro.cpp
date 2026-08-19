#include "Entities/HammerBro.hpp"
#include "Entities/HammerThrowStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/EntityFactory.hpp"

HammerBro::HammerBro(sf::Vector2f position)
    : Enemy(position, 1000, {32.0f, 48.0f}) {
    auto strategy = std::make_unique<HammerThrowStrategy>();

    // HammerThrowStrategy has always had this hook and nothing ever set it, so
    // the throw branch was dead and no Hammer class existed to spawn (audit
    // B-6). Ask the world to create the projectile.
    strategy->setThrowCallback([](sf::Vector2f origin, bool faceRight) {
        EntitySpawnRequest request;
        request.type = static_cast<int>(EntityType::Hammer);
        request.position = origin;
        // Up and forward, so it arcs over a short gap.
        request.velocity = sf::Vector2f(faceRight ? 220.0f : -220.0f, -420.0f);
        EventBus::getInstance().publish({EventType::EntitySpawnRequested, request});
    });

    setStrategy(std::move(strategy));
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
    triggerFlipDeath({100.0f, -300.0f});
}

void HammerBro::onHitByFireball() {
    SoundManager::getInstance().playSound("kick");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    triggerFlipDeath({100.0f, -300.0f});
}
