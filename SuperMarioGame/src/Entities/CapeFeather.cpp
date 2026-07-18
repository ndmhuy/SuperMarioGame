#include "Entities/CapeFeather.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

CapeFeather::CapeFeather(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void CapeFeather::update(float dt) {
    // CapeFeather stays stationary or floats gently. Let's keep it simple and stationary.
}

void CapeFeather::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(255, 235, 150)); // Light Yellow-Feather color
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void CapeFeather::activate(Player& player) {
    player.powerUp(2); // CapeFeather type = 2
    player.addScore(1000);
}
