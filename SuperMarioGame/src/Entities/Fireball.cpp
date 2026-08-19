#include "Entities/Fireball.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Fireball::Fireball(sf::Vector2f pos, sf::Vector2f vel)
    : Entity() {
    position = pos;
    velocity = vel;
    boundingBox.x = pos.x;
    boundingBox.y = pos.y;
    // Collision box is deliberately smaller than the drawn flame.
    setTargetSize({12.0f, 12.0f});
}

void Fireball::bounce() {
    velocity.y = -240.0f; // Ground bounce velocity impulse
    m_bouncesLeft--;
    if (m_bouncesLeft <= 0) {
        destroy();
    }
}

void Fireball::update(float dt) {
    if (!active) return;

    m_animTimer += dt * 15.0f;
    m_lifetime -= dt;
    if (m_lifetime <= 0.0f) {
        destroy();
        return;
    }

    // Apply gravity to fireball
    velocity.y += 1200.0f * dt;
    position += velocity * dt;
}

void Fireball::render(sf::RenderTarget& target) {
    if (!active) return;

    sf::Vector2f center = position + sf::Vector2f(6.0f, 6.0f);

    // 1. Outer Flame Glow Aura (Orange-Red)
    sf::CircleShape outerGlow(8.0f);
    outerGlow.setOrigin({8.0f, 8.0f});
    outerGlow.setPosition(center);
    outerGlow.setFillColor(sf::Color(255, 69, 0, 200)); // Flame Orange
    outerGlow.setOutlineColor(sf::Color(255, 215, 0, 220)); // Gold outline
    outerGlow.setOutlineThickness(1.5f);
    target.draw(outerGlow);

    // 2. Inner Hot Core (Bright Yellow)
    sf::CircleShape innerCore(4.0f);
    innerCore.setOrigin({4.0f, 4.0f});
    innerCore.setPosition(center);
    innerCore.setFillColor(sf::Color(255, 255, 128)); // Bright Yellow Core
    target.draw(innerCore);

    // 3. Orbiting Flame Sparks (Spinning effect)
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
