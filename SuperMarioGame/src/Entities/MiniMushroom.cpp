#include "Entities/MiniMushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MiniMushroom::MiniMushroom(sf::Vector2f pos) : Item(pos, {32.0f, 32.0f}) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
    setTargetSize({32.0f, 32.0f});
}

void MiniMushroom::update(float dt) {
    if (!active) return;
    Item::update(dt);
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void MiniMushroom::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("mini_mushroom");
    m_animation.frameList = {{"mushroom_blue", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void MiniMushroom::render(sf::RenderTarget& target) {
    Item::render(target);
}

void MiniMushroom::activate(Player& player) {
    player.powerUp(3); // MiniMushroom type = 3
    player.addScore(1000);
}
