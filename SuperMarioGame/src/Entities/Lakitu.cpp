#include "Entities/Lakitu.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Core/GameSnapshot.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Player.hpp"
#include "Entities/Spiny.hpp"
#include "Core/Game.hpp"
#include "Utils/Constants.hpp"

#include <cmath>

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

bool Lakitu::isEngaged() const {
    const Player* player = Game::getInstance().getNearestPlayer(getPosition());
    // No player in the world is not "the player is far away" — it is a world
    // with no encounter in it, so there is nothing to wind up for.
    if (!player) return false;
    return std::abs(player->getPosition().x - position.x) <= ENGAGE_RANGE_X;
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

    // Nothing winds up while the player is somewhere else. The egg clock used
    // to start at level load along with FlyStrategy's unbounded tracking, so a
    // Lakitu parked 5500px down the level spent its whole allowance throwing
    // eggs at nobody before the player was ever in the room (R21 D8).
    if (!isEngaged()) return;

    // Concurrent, not lifetime: three Spinies in play at once, replenished as
    // the player clears them. See MAX_SPINIES for why the lifetime version was
    // the defect. The timer is deliberately held rather than accumulated while
    // capped, so clearing the field buys a fresh wind-up instead of an instant
    // volley of the drops that were banked meanwhile.
    if (Spiny::liveCount() >= MAX_SPINIES) return;

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
