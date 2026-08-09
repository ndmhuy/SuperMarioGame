#include "Entities/Mushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Mushroom::Mushroom(sf::Vector2f pos) : Item(pos, {32.0f, 32.0f}) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
    setTargetSize({32.0f, 32.0f});
}

void Mushroom::update(float dt) {
    if (!active) return;
    Item::update(dt);
    
    // Check if horizontal velocity was cancelled to 0 by wall collision
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void Mushroom::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("mushroom");
    m_animation.frameList = {{"mushroom_red", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Mushroom::render(sf::RenderTarget& target) {
    Item::render(target);
}

void Mushroom::activate(Player& player) {
    player.powerUp(0); // Mushroom type = 0
    player.addScore(1000);
}
