#include "Entities/KoopaTroopa.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include <algorithm>
#include <cmath>

KoopaTroopa::KoopaTroopa(sf::Vector2f position, bool isRed)
    : Enemy(position, 200), m_isRed(isRed) {
    speed = Constants::ENEMY_KOOPA_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };
    
    // Start with PatrolStrategy
    setStrategy(std::make_unique<PatrolStrategy>(/*ledgeAware=*/true, false));
}

void KoopaTroopa::update(float dt) {
    if (m_kickGrace > 0.0f) {
        m_kickGrace = std::max(0.0f, m_kickGrace - dt);
    }

    if (m_isFlipped) {
        Enemy::update(dt);
    } else {
        if (m_state == KoopaState::Walking) {
            Enemy::update(dt);
        } else if (m_state == KoopaState::ShellHeld) {
            if (m_holder && m_holder->isActive()) {
                // Position held shell cleanly directly on top of player's bounding box
                float holderCenterX = m_holder->getBoundingBox().x + m_holder->getBoundingBox().width * 0.5f;
                position.x = holderCenterX - m_targetSize.x * 0.5f;
                position.y = m_holder->getBoundingBox().y - m_targetSize.y;
                velocity = sf::Vector2f(0.0f, 0.0f);
                facingRight = m_holder->isFacingRight();
                boundingBox.x = position.x;
                boundingBox.y = position.y;
            } else {
                m_holder = nullptr;
                m_state = KoopaState::ShellIdle;
                m_shellTimer = Constants::KOOPA_SHELL_WAKE_TIME;
            }
        } else if (m_state == KoopaState::ShellIdle) {
            velocity.x = 0.0f;
            m_shellTimer -= dt;
            if (m_shellTimer <= 0.0f) {
                m_state = KoopaState::Walking;
                speed = Constants::ENEMY_KOOPA_SPEED;
                setStrategy(std::make_unique<PatrolStrategy>(/*ledgeAware=*/true, false));
            }
        } else if (m_state == KoopaState::ShellKicked) {
            if (onWall) {
                velocity.x = -velocity.x;
                onWall = false;
            }
            // Hold the speed. PhysicsEngine applies friction to every Character,
            // and an Enemy is a Character — but a shelled Koopa runs no strategy,
            // so nothing re-asserted its velocity and the decay was unopposed. A
            // kicked shell slid a short way, stopped, and then the player walked
            // back into a "moving shell" and took damage from it. Shell chains
            // could not work either: the shell never reached the next enemy.
            const float direction = (velocity.x >= 0.0f) ? 1.0f : -1.0f;
            velocity.x = direction * Constants::KOOPA_SHELL_KICK_SPEED;
        }
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }

    if (m_animator && m_hasAnimation) {
        if (m_state == KoopaState::ShellIdle || m_state == KoopaState::ShellHeld || m_state == KoopaState::ShellKicked) {
            m_animator->play(&m_shellAnim);
        } else if (m_state == KoopaState::Walking) {
            m_animator->play(&m_animation);
        }
    }
}

void KoopaTroopa::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    std::string moveKey0 = m_isRed ? "koopa_red_move_left_0" : "koopa_green_move_left_0";
    std::string moveKey1 = m_isRed ? "koopa_red_move_left_1" : "koopa_green_move_left_1";
    m_animation = Animation("koopa_move");
    m_animation.frameList = {{moveKey0, 0.15f}, {moveKey1, 0.15f}};

    std::string shellKey = m_isRed ? "koopa_red_shell" : "koopa_green_shell";
    m_shellAnim = Animation("koopa_shell");
    m_shellAnim.frameList = {{shellKey, 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void KoopaTroopa::render(sf::RenderTarget& target) {
    Enemy::render(target);
}

void KoopaTroopa::onStomped() {
    if (m_isFlipped) return;

    if (m_state == KoopaState::Walking) {
        m_state = KoopaState::ShellIdle;
        velocity = sf::Vector2f(0.0f, velocity.y);
        m_shellTimer = Constants::KOOPA_SHELL_WAKE_TIME;

        // Publish EnemyDefeated event
        GameEvent event;
        event.type = EventType::EnemyDefeated;
        event.data = m_scoreValue;
        EventBus::getInstance().publish(event);
    }
    else if (m_state == KoopaState::ShellIdle) {
        // Kick the shell
        float kickDir = facingRight ? 1.0f : -1.0f;
        kick(sf::Vector2f(kickDir * Constants::KOOPA_SHELL_KICK_SPEED, velocity.y));
    }
    else if (m_state == KoopaState::ShellKicked) {
        // Stop the shell
        m_state = KoopaState::ShellIdle;
        velocity = sf::Vector2f(0.0f, velocity.y);
        m_shellTimer = Constants::KOOPA_SHELL_WAKE_TIME;
    }
}

void KoopaTroopa::onHitByFireball() {
    if (m_isFlipped) return;
    
    m_isFlipped = true;
    m_state = KoopaState::ShellIdle;
    velocity = sf::Vector2f(100.0f, -300.0f);

    // Publish EnemyDefeated event
    GameEvent event;
    event.type = EventType::EnemyDefeated;
    event.data = m_scoreValue;
    EventBus::getInstance().publish(event);
}

void KoopaTroopa::kick(sf::Vector2f velocity) {
    m_state = KoopaState::ShellKicked;
    this->velocity = velocity;
    // Long enough for the shell to clear the player who launched it.
    m_kickGrace = 0.35f;
}

void KoopaTroopa::pickUp(Player* holder) {
    if (!holder) return;
    m_holder = holder;
    m_state = KoopaState::ShellHeld;
    velocity = sf::Vector2f(0.0f, 0.0f);
    if (m_animator && m_hasAnimation) {
        m_animator->play(&m_shellAnim);
    }
}

void KoopaTroopa::release() {
    m_holder = nullptr;
    m_state = KoopaState::ShellIdle;
    velocity = sf::Vector2f(0.0f, 0.0f);
    m_shellTimer = Constants::KOOPA_SHELL_WAKE_TIME;
}

void KoopaTroopa::throwShell(float speed, float angleDeg) {
    float throwDir = m_holder ? (m_holder->isFacingRight() ? 1.0f : -1.0f) : (facingRight ? 1.0f : -1.0f);
    m_holder = nullptr;
    m_state = KoopaState::ShellKicked;
    
    float rad = angleDeg * 3.14159265f / 180.0f;
    float vx = throwDir * speed * std::cos(rad);
    float vy = -speed * std::sin(rad); // Upward in SFML is negative Y
    
    kick(sf::Vector2f(vx, vy));
    SoundManager::getInstance().playSound("kick");
}

bool KoopaTroopa::isCollidable() const {
    // Was a zero-sized AABB returned from getBoundingBox() (audit B-14).
    return !isDeadOrDying();
}
