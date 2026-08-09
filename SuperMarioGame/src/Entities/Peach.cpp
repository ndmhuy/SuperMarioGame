#include "Entities/Peach.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/InputManager.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Peach::Peach(sf::Vector2f pos) : Player(pos) {
    position = pos;
    speed = Constants::WALK_SPEED * 0.9f;
    jumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::JUMP_HEIGHT);
    health = 1;
    
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    changeState(std::make_unique<SmallState>());
}

void Peach::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
    m_animation = Animation("peach_small_idle");
    m_animation.frameList = {{"peach_small_idle", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Peach::update(float dt) {
    if (onGround) {
        m_hoverTimer = 0.0f;
        m_isHovering = false;
    } else {
        bool isP1 = (this == InputManager::getInstance().getPlayer(0));
        bool isP2 = (this == InputManager::getInstance().getPlayer(1));
        
        bool jumpHeld = false;
        if (isP1) {
            jumpHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
        } else if (isP2) {
            jumpHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
        }

        if (jumpHeld && m_hoverTimer < 1.5f && velocity.y >= 0.0f) {
            m_isHovering = true;
            m_hoverTimer += dt;
            velocity.y = 0.0f; // Float/hover: zero downward speed
        } else {
            m_isHovering = false;
            if (m_hoverTimer > 0.0f && !jumpHeld) {
                m_hoverTimer = 1.5f; // Exhaust hover for this flight if jump released
            }
        }
    }
    
    Player::update(dt);
}

void Peach::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        Player::render(target);
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color(255, 105, 180)); // Hot Pink
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}

void Peach::floatHover() {
    // Handled in update loop
}

float Peach::getGravityMultiplier() const {
    if (m_isHovering) {
        return 0.0f;
    }
    return 1.0f;
}
