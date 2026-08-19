#include "Entities/Hammer.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

Hammer::Hammer(sf::Vector2f pos, sf::Vector2f vel) : Entity(pos, {16.0f, 16.0f}) {
    position = pos;
    velocity = vel;
    setTargetSize({16.0f, 16.0f});
}

void Hammer::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
    m_animation = Animation("hammer_spin");
    m_animation.frameList = {
        {"hammer_black_0", 0.06f}, {"hammer_black_1", 0.06f},
        {"hammer_black_2", 0.06f}, {"hammer_black_3", 0.06f}
    };
    m_animator->play(&m_animation);
    m_hasAnimation = true;
}

void Hammer::update(float dt) {
    if (!active) return;

    m_lifetime -= dt;
    if (m_lifetime <= 0.0f) {
        destroy();
        return;
    }

    // Arc under gravity. Physics skips this entity's tile pass
    // (collidesWithTiles() == false) but still integrates position, so only the
    // vertical acceleration is applied here.
    velocity.y += Constants::GRAVITY * Constants::GRAVITY_SCALE * dt;

    m_spin += dt * 720.0f;   // degrees/sec, purely visual

    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}

void Hammer::render(sf::RenderTarget& target) {
    if (!active) return;

    if (m_animator && m_hasAnimation) {
        drawSprite(target, m_animator->getSprite(), SpriteAnchor::BottomCenter);
        return;
    }

    // Placeholder before the atlas is wired.
    sf::CircleShape blob(6.0f);
    blob.setFillColor(sf::Color(40, 40, 40));
    blob.setOutlineColor(sf::Color(200, 200, 200));
    blob.setOutlineThickness(1.0f);
    blob.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
    target.draw(blob);
}

void Hammer::onHitPlayer(Player& player) {
    if (!active) return;
    player.takeDamage(1);
    destroy();
}
