#include "Entities/MegaMushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MegaMushroom::MegaMushroom(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
}

void MegaMushroom::update(float dt) {
    if (!active) return;
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void MegaMushroom::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(255, 127, 80)); // Coral/Yellow-Red
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.5f);
    target.draw(rect);
}

void MegaMushroom::activate(Player& player) {
    player.powerUp(5); // MegaMushroom type = 5
    player.addScore(1000);
}
