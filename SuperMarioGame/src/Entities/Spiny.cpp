#include "Entities/Spiny.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

Spiny::Spiny(sf::Vector2f position, bool isEgg)
    : Enemy(position, 200), m_isEgg(isEgg) {
    speed = Constants::ENEMY_SPINY_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };

    // PatrolStrategy: Spiny patrols back and forth on platforms/ground
    setStrategy(std::make_unique<PatrolStrategy>(false, false));
}

void Spiny::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("spiny");
    m_animation.frameList = {{"spiny_move_left_0", 0.15f}, {"spiny_move_left_1", 0.15f}};
    m_eggAnim = Animation("spiny_egg");
    m_eggAnim.frameList = {{"spiny_ball_0", 0.15f}, {"spiny_ball_1", 0.15f}};
    if (m_animator) {
        m_animator->play(m_isEgg ? &m_eggAnim : &m_animation);
        m_hasAnimation = true;
    }
}

void Spiny::update(float dt) {
    if (m_isFlipped) {
        // Fall off screen: gravity in px/s^2 (GRAVITY = 0.5 px/frame^2 = 1800 px/s^2)
        velocity.y += 1800.0f * dt;
        position += velocity * dt;
        if (position.y > Constants::WINDOW_HEIGHT + 100.0f) {
            destroy();
        }
    } else {
        if (m_isEgg) {
            if (onGround) {
                m_isEgg = false; // Hatch upon landing!
                if (m_animator) {
                    m_animator->play(&m_animation);
                }
            } else if (m_animator) {
                m_animator->play(&m_eggAnim);
            }
        }
        Enemy::update(dt);
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }
}

void Spiny::onStomped() {
    if (m_isFlipped) return;

    // Spiky: Stomping deals damage to the player
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        player->takeDamage(1);
    }
}

void Spiny::onHitByFireball() {
    if (m_isFlipped) return;

    m_isFlipped = true;
    velocity = sf::Vector2f(100.0f, -300.0f); // Fly up and forward
    SoundManager::getInstance().playSound("kick");

    // Publish EnemyDefeated event
    GameEvent event;
    event.type = EventType::EnemyDefeated;
    event.data = m_scoreValue;
    EventBus::getInstance().publish(event);
}

const AABB& Spiny::getBoundingBox() const {
    static const AABB emptyBox{0.f, 0.f, 0.f, 0.f};
    if (m_isFlipped) {
        return emptyBox;
    }
    return boundingBox;
}
