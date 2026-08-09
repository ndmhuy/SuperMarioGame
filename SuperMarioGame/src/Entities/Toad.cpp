#include "Entities/Toad.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Toad::Toad(sf::Vector2f pos) : Player(pos) {
    position = pos;
    speed = Constants::WALK_SPEED * 1.3f;
    float baseJumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    jumpForce = baseJumpForce * 0.8f;
    health = 1;
    
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    changeState(std::make_unique<SmallState>());
}

void Toad::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
    m_animation = Animation("toad_small_idle");
    m_animation.frameList = {{"toad_small_idle", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
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
    if (m_animator && m_hasAnimation) {
        Player::render(target);
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color(0, 150, 255)); // Light Blue
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}
