#include "Entities/Star.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Star::Star(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{100.0f, 0.0f};
    m_movingRight = true;
}

void Star::update(float dt) {
    if (!active) return;
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 100.0f : -100.0f;
    }
    
    // Check ground bounce (velocity.y cancelled to 0 by floor collision)
    if (std::abs(velocity.y) < 0.01f) {
        velocity.y = -250.0f; // Bounce upward
    }
}

void Star::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(255, 255, 0)); // Bright Yellow
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void Star::activate(Player& player) {
    player.powerUp(4); // Star type = 4
    player.addScore(1000);
}
