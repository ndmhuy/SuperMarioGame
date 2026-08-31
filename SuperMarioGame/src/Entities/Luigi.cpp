#include "Entities/Luigi.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Luigi::Luigi(sf::Vector2f pos) : Player(pos) {
    position = pos;
    speed = Constants::WALK_SPEED * Constants::LUIGI_SPEED_MULT;
    float baseJumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    jumpForce = baseJumpForce * Constants::LUIGI_JUMP_MULT;
    health = 1;
    
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    // Starting form, not a form *change*: the constructor's position is the one
    // the caller asked for and must be preserved. changeState() here shifted it
    // by the placeholder-box height difference (2px), so no character ever
    // spawned exactly where it was constructed.
    setStartingForm(Form::Small);
}

void Luigi::setupAnimations(const SpriteSheet* spriteSheet) {
    Player::setupCharacterAnimations(spriteSheet, "luigi_small");
}

void Luigi::update(float dt) {
    Player::update(dt);
    if (onGround) {
        m_hasDoubleJumped = false;
    }
}

void Luigi::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        Player::render(target);
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color::Green);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}

void Luigi::jump() {
    // Ground jump, or within the coyote grace window — Player::jump handles both.
    if (onGround || getCoyoteFramesLeft() > 0) {
        Player::jump();
        m_hasDoubleJumped = false;
    } else if (!m_hasDoubleJumped) {
        doubleJump();
    } else {
        // Both jumps spent: fall through to Player::jump, which buffers the
        // request so it fires on landing instead of being dropped.
        Player::jump();
    }
}

void Luigi::doubleJump() {
    this->velocity.y = -this->jumpForce;
    m_hasDoubleJumped = true;
}

float Luigi::getGravityMultiplier() const {
    return Constants::LUIGI_GRAVITY_MULT;
}

float Luigi::getRunSpeed() const {
    // Luigi is the slower of the two brothers, in a sprint as well as a walk.
    return Constants::RUN_SPEED * Constants::LUIGI_SPEED_MULT;
}

std::string Luigi::getCharacterName() const {
    return "luigi";
}
