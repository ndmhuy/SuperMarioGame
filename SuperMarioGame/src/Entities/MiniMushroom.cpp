#include "Entities/MiniMushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MiniMushroom::MiniMushroom(sf::Vector2f pos) : Item(pos) {
    // Override bounding box to be half-size (16x16 px)
    boundingBox = AABB{pos.x, pos.y, 16.0f, 16.0f};
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
}

void MiniMushroom::update(float dt) {
    if (!active) return;
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void MiniMushroom::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(30, 144, 255)); // Dodger Blue (distinct small box)
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void MiniMushroom::activate(Player& player) {
    player.powerUp(3); // MiniMushroom type = 3
    player.addScore(1000);
}
