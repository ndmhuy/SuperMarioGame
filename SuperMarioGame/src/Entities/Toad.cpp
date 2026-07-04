#include "Entities/Toad.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Toad::Toad(sf::Vector2f pos) {
    position = pos;
    speed = Constants::WALK_SPEED * 1.3f;
    float baseJumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    jumpForce = baseJumpForce * 0.8f;
    health = 1;
    
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    changeState(std::make_unique<SmallState>());
}

void Toad::update(float dt) {
    float prevVx = velocity.x;
    Player::update(dt);
    if (sliding) {
        // Bypass slide friction deceleration delay
        velocity.x = prevVx;
    }
}

void Toad::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(0, 150, 255)); // Light Blue
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}
