#include "Entities/PSwitch.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

PSwitch::PSwitch(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void PSwitch::update(float dt) {
    Item::update(dt);
}

void PSwitch::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("p_switch");
    m_animation.frameList = {{"p_switch_normal", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void PSwitch::render(sf::RenderTarget& target) {
    Item::render(target);
}

void PSwitch::activate(Player& player) {
    // Bricks <-> Coins swap for 15 seconds. Trigger via EventBus.
    EventBus::getInstance().publish({EventType::PSwitchActivated, 15.0f});
}
