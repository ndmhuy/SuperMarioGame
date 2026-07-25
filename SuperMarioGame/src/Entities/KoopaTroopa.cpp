#include "Entities/KoopaTroopa.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Utils/Constants.hpp"
#include "Core/EventBus.hpp"

KoopaTroopa::KoopaTroopa(sf::Vector2f position, bool isRed)
    : Enemy(position, 200), m_isRed(isRed) {
    speed = Constants::ENEMY_KOOPA_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };
    
    // Start with PatrolStrategy
    setStrategy(std::make_unique<PatrolStrategy>(m_isRed, false));
}

void KoopaTroopa::update(float dt) {
    if (m_isFlipped) {
        // Fall off screen
        velocity.y += 1800.0f * dt;
        position += velocity * dt;
        if (position.y > Constants::WINDOW_HEIGHT + 100.0f) {
            destroy();
        }
    } else {
        if (m_state == KoopaState::Walking) {
            Enemy::update(dt);
        } else if (m_state == KoopaState::ShellIdle) {
            velocity.x = 0.0f;
            m_shellTimer -= dt;
            if (m_shellTimer <= 0.0f) {
                m_state = KoopaState::Walking;
                speed = Constants::ENEMY_KOOPA_SPEED;
                setStrategy(std::make_unique<PatrolStrategy>(m_isRed, false));
            }
        } else if (m_state == KoopaState::ShellKicked) {
            if (onWall) {
                velocity.x = -velocity.x;
                onWall = false;
            }
        }
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }
}

void KoopaTroopa::render(sf::RenderTarget& target) {
    // Visual rendering will be implemented in Phase 5
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
}

const AABB& KoopaTroopa::getBoundingBox() const {
    if (m_isFlipped) {
        static const AABB emptyBox{ 0.0f, 0.0f, 0.0f, 0.0f };
        return emptyBox;
    }
    return boundingBox;
}
