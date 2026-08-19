#include "Entities/Fireball.hpp"
#include "Entities/Enemy.hpp"
#include "Graphics/ParticleSystem.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

namespace {
// One pass of the three-frame burst.
constexpr float IMPACT_DURATION = 0.24f;
}

Fireball::Fireball(sf::Vector2f pos, sf::Vector2f vel)
    // Collision box is deliberately smaller than the drawn flame.
    : Projectile(pos, {12.0f, 12.0f}) {
    velocity = vel;
}

void Fireball::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);

    // The atlas has shipped a four-frame spin and a three-frame burst for the
    // fire flower all along. The fireball was drawn with hand-rolled
    // CircleShapes instead, which is why it never matched anything else on
    // screen.
    m_flightAnim = Animation("fireball_flight");
    m_flightAnim.frameList = {{"flower_fireball_0", 0.06f}, {"flower_fireball_1", 0.06f},
                              {"flower_fireball_2", 0.06f}, {"flower_fireball_3", 0.06f}};

    m_impactAnim = Animation("fireball_impact");
    m_impactAnim.frameList = {{"flower_fireball_hit_0", 0.08f},
                              {"flower_fireball_hit_1", 0.08f},
                              {"flower_fireball_hit_2", 0.08f}};
    m_impactAnim.isLooping = false;

    if (spriteSheet->hasFrame("flower_fireball_0")) {
        m_animator->play(&m_flightAnim);
        m_hasAnimation = true;
    }
}

void Fireball::beginImpact() {
    if (m_impactTimer > 0.0f) return;   // already bursting

    m_impactTimer = IMPACT_DURATION;
    velocity = {0.0f, 0.0f};

    if (m_animator && m_hasAnimation) {
        m_animator->play(&m_impactAnim);
    }

    // A short spray of embers, so a hit reads even at the edge of the screen.
    const sf::Vector2f centre = position + sf::Vector2f(6.0f, 6.0f);
    for (int i = 0; i < 8; ++i) {
        const float angle = static_cast<float>(i) * (6.28318f / 8.0f);
        ParticleData ember;
        ember.position = centre;
        ember.velocity = {std::cos(angle) * 90.0f, std::sin(angle) * 90.0f - 40.0f};
        ember.acceleration = {0.0f, 420.0f};
        ember.startColor = sf::Color(255, 200, 60, 255);
        ember.endColor = sf::Color(200, 60, 0, 0);
        ember.lifetime = 0.35f;
        ember.startScale = 1.4f;
        ember.endScale = 0.2f;
        ParticleSystem::getInstance().emit(ember);
    }
}

void Fireball::onHitEnemy(Enemy& enemy) {
    enemy.onHitByFireball();
    beginImpact();
}

void Fireball::bounce() {
    velocity.y = -240.0f; // Ground bounce velocity impulse
    m_bouncesLeft--;
    if (m_bouncesLeft <= 0) {
        beginImpact();
    }
}

void Fireball::update(float dt) {
    if (!active) return;

    m_animTimer += dt * 15.0f;

    if (m_impactTimer > 0.0f) {
        // Burning out: no movement, no gravity, and gone once the burst ends.
        m_impactTimer -= dt;
        if (m_animator && m_hasAnimation) {
            m_animator->update(dt);
        }
        if (m_impactTimer <= 0.0f) {
            destroy();
        }
        return;
    }

    m_lifetime -= dt;
    if (m_lifetime <= 0.0f) {
        beginImpact();
        return;
    }

    // Apply gravity to fireball
    velocity.y += 1200.0f * dt;
    position += velocity * dt;

    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}

void Fireball::render(sf::RenderTarget& target) {
    if (!active) return;

    if (m_animator && m_hasAnimation) {
        // Drawn here rather than through Entity::drawSprite: that helper anchors
        // top-left or bottom-centre and cannot rotate, and a fireball needs to
        // spin about its own centre.
        sf::Sprite sprite = m_animator->getSprite();
        const auto bounds = sprite.getLocalBounds();
        if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;

        sprite.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});

        // Source frames are 7-8px. The flame is scaled to the 12px collision box
        // so what is drawn is what kills; the burst is drawn larger than its box
        // on purpose, because it no longer collides with anything.
        const float scale = (m_impactTimer > 0.0f) ? 2.2f : (12.0f / bounds.size.x);
        sprite.setScale({scale, scale});
        sprite.setPosition(position + sf::Vector2f(6.0f, 6.0f));

        // Spin only while flying: a burst that spins reads as a mistake.
        if (m_impactTimer <= 0.0f) {
            sprite.setRotation(sf::degrees(m_animTimer * 60.0f));
        }
        target.draw(sprite);
        return;
    }

    // Fallback for a missing atlas: the original hand-drawn flame, so the shot
    // is still visible rather than invisible.
    sf::Vector2f center = position + sf::Vector2f(6.0f, 6.0f);

    sf::CircleShape outerGlow(8.0f);
    outerGlow.setOrigin({8.0f, 8.0f});
    outerGlow.setPosition(center);
    outerGlow.setFillColor(sf::Color(255, 69, 0, 200));
    outerGlow.setOutlineColor(sf::Color(255, 215, 0, 220));
    outerGlow.setOutlineThickness(1.5f);
    target.draw(outerGlow);

    sf::CircleShape innerCore(4.0f);
    innerCore.setOrigin({4.0f, 4.0f});
    innerCore.setPosition(center);
    innerCore.setFillColor(sf::Color(255, 255, 128));
    target.draw(innerCore);

    for (int i = 0; i < 2; ++i) {
        float angle = m_animTimer + (i * 3.14159f);
        sf::Vector2f sparkOffset{ std::cos(angle) * 7.0f, std::sin(angle) * 7.0f };
        sf::CircleShape spark(2.0f);
        spark.setOrigin({2.0f, 2.0f});
        spark.setPosition(center + sparkOffset);
        spark.setFillColor(sf::Color(255, 140, 0));
        target.draw(spark);
    }
}
