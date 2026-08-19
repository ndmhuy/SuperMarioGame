#include "Entities/BossFireball.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

BossFireball::BossFireball(sf::Vector2f position, sf::Vector2f velocity)
    : Projectile(position, {24.0f, 12.0f}) {
    this->velocity = velocity;
    m_travellingRight = velocity.x > 0.0f;
}

void BossFireball::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);

    // The atlas carries a directional pair, so the sprite is chosen by travel
    // direction rather than mirrored.
    const std::string side = m_travellingRight ? "right" : "left";
    m_animation = Animation("boss_fireball");
    m_animation.frameList = {{"bowser_fire_" + side + "_0", 0.10f},
                             {"bowser_fire_" + side + "_1", 0.10f}};
    if (spriteSheet->hasFrame(m_animation.frameList.front().frameName)) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void BossFireball::update(float dt) {
    if (!active) return;

    m_lifetime -= dt;
    if (m_lifetime <= 0.0f) {
        destroy();
        return;
    }

    // No gravity: getGravityMultiplier() is zero, so the physics engine leaves
    // the vertical velocity alone and this stays level.
    position += velocity * dt;
    boundingBox.x = position.x;
    boundingBox.y = position.y;

    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}

void BossFireball::onHitPlayer(Player& player) {
    player.takeDamage(1);
    destroy();
}

void BossFireball::render(sf::RenderTarget& target) {
    if (!active) return;

    if (m_animator && m_hasAnimation) {
        drawSprite(target, m_animator->getSprite(), SpriteAnchor::BottomCenter);
        return;
    }

    // Fallback so the attack is still readable if the atlas is missing.
    sf::RectangleShape flame({24.0f, 10.0f});
    flame.setPosition(position);
    flame.setFillColor(sf::Color(255, 90, 0, 230));
    flame.setOutlineColor(sf::Color(255, 220, 90));
    flame.setOutlineThickness(1.5f);
    target.draw(flame);
}
