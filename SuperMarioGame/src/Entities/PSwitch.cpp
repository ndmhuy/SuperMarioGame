#include "Entities/PSwitch.hpp"
#include "Entities/Player.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cmath>

PSwitch::PSwitch(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void PSwitch::update(float dt) {
    Item::update(dt);
}

void PSwitch::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);

    // Idle animation: p_switch_normal
    m_animation = Animation("p_switch");
    m_animation.frameList = {{"p_switch_normal", 0.15f}};

    // Pressed animation: p_switch_pressed (static, stays on squished sprite)
    m_pressedAnimation = Animation("p_switch_pressed");
    m_pressedAnimation.frameList = {{"p_switch_pressed", 1.0f}};
    m_pressedAnimation.isLooping = false;

    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void PSwitch::render(sf::RenderTarget& target) {
    if (m_pressed || collected || !active) {
        if (m_spriteSheet) {
            sf::Sprite sprite = m_spriteSheet->getSprite("p_switch_pressed");
            sf::FloatRect bounds = sprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                float scale = std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);
                sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
                sprite.setScale(sf::Vector2f(scale, scale));
                sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f, boundingBox.y + m_targetSize.y));
                target.draw(sprite);
            }
        }
    } else {
        Item::render(target);
    }
}

void PSwitch::collect() {
    collected = true;
    // Keep active = true so PSwitch stays on screen in pressed/squished state
}

void PSwitch::activate(Player& player) {
    if (!m_pressed) {
        m_pressed = true;
        // Switch to squished/pressed sprite
        if (m_animator && m_hasAnimation) {
            m_animator->play(&m_pressedAnimation);
        }
    }
    // Bricks <-> Coins swap for 15 seconds. Trigger via EventBus.
    EventBus::getInstance().publish({EventType::PSwitchActivated, 15.0f});
}

ItemTouch PSwitch::onPlayerTouch(Player& player, const CollisionInfo& info) {
    // Touching a P-Switch from any direction presses it.
    (void)info;
    activate(player);
    collect();
    // Still solid afterwards, so the player lands on top of the squished switch
    // rather than dropping through where it used to be.
    return ItemTouch::Solid;
}
