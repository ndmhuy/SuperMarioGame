#include "Entities/Trampoline.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Trampoline::Trampoline(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void Trampoline::update(float dt) {
    // Trampolines are stationary
}

void Trampoline::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(192, 192, 192)); // Silver/grey trampoline
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.5f);
    target.draw(rect);
}

void Trampoline::activate(Player& player) {
    // Bounce player ~6 tiles high (using calculated velocity 831.4 px/s)
    sf::Vector2f vel = player.getVelocity();
    vel.y = -831.4f;
    player.setVelocity(vel);
}

void Trampoline::collect() {
    // Trampoline is a reusable block, it is not consumed/collected.
    // Override base class collect to do nothing.
}
