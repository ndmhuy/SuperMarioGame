#include "Entities/Mushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Mushroom::Mushroom(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
}

void Mushroom::update(float dt) {
    if (!active) return;
    
    // Check if horizontal velocity was cancelled to 0 by wall collision
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void Mushroom::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(255, 140, 0)); // Dark Orange
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void Mushroom::activate(Player& player) {
    player.powerUp(0); // Mushroom type = 0
    player.addScore(1000);
}
