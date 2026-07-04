#include "Entities/Luigi.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Luigi::Luigi(sf::Vector2f pos) {
    position = pos;
    speed = Constants::WALK_SPEED * Constants::LUIGI_SPEED_MULT;
    float baseJumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    jumpForce = baseJumpForce * Constants::LUIGI_JUMP_MULT;
    health = 1;
    
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    changeState(std::make_unique<SmallState>());
}

void Luigi::update(float dt) {
    Player::update(dt);
    if (onGround) {
        m_hasDoubleJumped = false;
    }
}

void Luigi::render(sf::RenderTarget& target) {
    if (!active) return;
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color::Green);
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}

void Luigi::jump() {
    if (onGround || onWall) {
        Player::jump();
        m_hasDoubleJumped = false;
    } else if (!m_hasDoubleJumped) {
        doubleJump();
    }
}

void Luigi::doubleJump() {
    this->velocity.y = -this->jumpForce;
    m_hasDoubleJumped = true;
}

float Luigi::getGravityMultiplier() const {
    return Constants::LUIGI_GRAVITY_MULT;
}
