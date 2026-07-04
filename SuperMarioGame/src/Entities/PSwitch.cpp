#include "Entities/PSwitch.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

PSwitch::PSwitch(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void PSwitch::update(float dt) {
    // PSwitches are stationary
}

void PSwitch::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(70, 130, 180)); // Steel Blue
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.5f);
    target.draw(rect);
}

void PSwitch::activate(Player& player) {
    // Bricks <-> Coins swap for 15 seconds. Trigger via EventBus.
    EventBus::getInstance().publish({EventType::PSwitchActivated, 15.0f});
}
