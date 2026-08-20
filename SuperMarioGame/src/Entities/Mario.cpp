#include "Entities/Mario.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/ResourceManager.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

Mario::Mario(sf::Vector2f pos) : Player(pos) {
    position = pos;
    speed = Constants::WALK_SPEED;
    jumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    health = 1;
    
    // Set initial size and boundingBox
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    // Starting form, not a form *change*: the constructor's position is the one
    // the caller asked for and must be preserved. changeState() here shifted it
    // by the placeholder-box height difference (2px), so no character ever
    // spawned exactly where it was constructed.
    setStartingForm(Form::Small);
}

void Mario::setupAnimations(const SpriteSheet* spriteSheet) {
    Player::setupCharacterAnimations(spriteSheet, "mario_small");
}

void Mario::update(float dt) {
    Player::update(dt);
}

void Mario::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        Player::render(target);
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color::Red);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}

std::string Mario::getCharacterName() const {
    return "mario";
}
