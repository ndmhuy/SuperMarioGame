#include "Entities/FireFlower.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

FireFlower::FireFlower(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void FireFlower::update(float dt) {
    // Stationary, nothing to update
}

void FireFlower::render(sf::RenderTarget& target) {
    if (!active) return;
    
    // Draw a visual representing a flower (a white/orange circle)
    sf::CircleShape flower(boundingBox.width / 2.0f);
    flower.setPosition(position);
    flower.setFillColor(sf::Color(255, 69, 0)); // Red-Orange
    flower.setOutlineColor(sf::Color::White);
    flower.setOutlineThickness(2.0f);
    target.draw(flower);
}

void FireFlower::activate(Player& player) {
    player.powerUp(1); // FireFlower type = 1
    player.addScore(1000);
}
