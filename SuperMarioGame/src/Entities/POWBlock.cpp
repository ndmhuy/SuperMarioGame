#include "Entities/POWBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

POWBlock::POWBlock(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void POWBlock::update(float dt) {
    // Stationary interactive block
}

void POWBlock::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(105, 105, 105)); // Dim Grey
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(2.0f);
    target.draw(rect);
}

void POWBlock::activate(Player& player) {
    // Triggers a screen-shake and flips all grounded enemies via EventBus
    EventBus::getInstance().publish({EventType::BlockBroken, this});
}
