#include "Entities/KoopaParatroopa.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Utils/Constants.hpp"
#include "Core/EventBus.hpp"
#include <string>

KoopaParatroopa::KoopaParatroopa(sf::Vector2f position, bool isRed)
    : KoopaTroopa(position, isRed) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_PARATROOPA_SPEED;
    setScoreValue(400);
    m_hasWings = true;

    // Set initial FlyStrategy
    if (isRed) {
        setStrategy(std::make_unique<FlyStrategy>(FlyMode::VerticalBounce, false));
    } else {
        setStrategy(std::make_unique<FlyStrategy>(FlyMode::SinusoidalPatrol, false));
    }
}

void KoopaParatroopa::setupAnimations(const SpriteSheet* spriteSheet) {
    KoopaTroopa::setupAnimations(spriteSheet);
    // KoopaTroopa::setupAnimations already honours m_isRed; the fly frames were
    // the one place still hardcoded to green (audit B-12).
    const std::string colour = m_isRed ? "red" : "green";
    m_flyAnim = Animation("koopa_fly_" + colour);
    m_flyAnim.frameList = {{"koopa_" + colour + "_fly_left_0", 0.15f},
                           {"koopa_" + colour + "_fly_left_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_flyAnim);
        m_hasAnimation = true;
    }
}

void KoopaParatroopa::update(float dt) {
    if (m_transformInvincibilityTimer > 0.0f) {
        m_transformInvincibilityTimer -= dt;
    }

    if (m_hasWings) {
        if (m_isFlipped) {
            KoopaTroopa::update(dt);
        } else {
            Enemy::update(dt);
            boundingBox.x = position.x;
            boundingBox.y = position.y;
        }
    } else {
        KoopaTroopa::update(dt);
    }
}

void KoopaParatroopa::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        sf::Sprite sprite = m_animator->getSprite();

        // Flicker while the wings-lost transformation grants brief immunity.
        if (m_transformInvincibilityTimer > 0.0f) {
            const bool dim = (static_cast<int>(m_transformInvincibilityTimer * 30.0f) % 2 == 0);
            sprite.setColor(sf::Color(255, 255, 255, dim ? 100 : 255));
        }

        drawSprite(target, sprite, SpriteAnchor::BottomCenter,
                   /*flipX=*/facingRight, /*flipY=*/m_isFlipped);
    }
}

void KoopaParatroopa::onStomped() {
    if (m_isFlipped || m_transformInvincibilityTimer > 0.0f) return;

    if (m_hasWings) {
        m_hasWings = false;
        m_transformInvincibilityTimer = 1.0f; // 1 second transformation invincibility grace period
        setScoreValue(200); // Subsequent stomps give standard Koopa Troopa points
        
        // Change strategy to PatrolStrategy (ledge-aware if red Koopa)
        setStrategy(std::make_unique<PatrolStrategy>(/*ledgeAware=*/true, false));
        if (m_animator) {
            m_animator->play(&m_animation); // Switch to ground walking animation
        }

        // Publish EnemyDefeated event for Paratroopa stomp
        GameEvent event;
        event.type = EventType::EnemyDefeated;
        event.data = 400; // 400 points
        EventBus::getInstance().publish(event);
    } else {
        KoopaTroopa::onStomped();
    }
}

void KoopaParatroopa::onHitByFireball() {
    if (m_isFlipped) return;
    
    // Set score value to 400 so KoopaTroopa's fireball handler awards paratroopa points
    if (m_hasWings) {
        setScoreValue(400);
    }
    KoopaTroopa::onHitByFireball();
}

float KoopaParatroopa::getGravityMultiplier() const {
    return m_hasWings ? 0.0f : 1.0f;
}
