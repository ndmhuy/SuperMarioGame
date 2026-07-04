#include "Entities/Mario.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Mario::Mario(sf::Vector2f pos) {
    position = pos;
    speed = Constants::WALK_SPEED;
    jumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    health = 1;
    
    // Set initial size and boundingBox
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    changeState(std::make_unique<SmallState>());
}

void Mario::update(float dt) {
    Player::update(dt);
}

void Mario::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color::Red);
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void Mario::shootFireball() {
    Player::shootFireball();
}
