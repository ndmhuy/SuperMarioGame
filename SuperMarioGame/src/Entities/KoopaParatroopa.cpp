#include "Entities/KoopaParatroopa.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Entities/PatrolStrategy.hpp"
#include "Utils/Constants.hpp"
#include "Core/EventBus.hpp"

KoopaParatroopa::KoopaParatroopa(sf::Vector2f position, bool isRed)
    : KoopaTroopa(position, isRed) {
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
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("koopa_fly");
    m_animation.frameList = {{"koopa_green_fly_left_0", 0.15f}, {"koopa_green_fly_left_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void KoopaParatroopa::update(float dt) {
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

void KoopaParatroopa::onStomped() {
    if (m_isFlipped) return;

    if (m_hasWings) {
        m_hasWings = false;
        setScoreValue(200); // Subsequent stomps give standard Koopa Troopa points
        
        // Change strategy to PatrolStrategy (ledge-aware if red Koopa)
        setStrategy(std::make_unique<PatrolStrategy>(m_isRed, false));

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
