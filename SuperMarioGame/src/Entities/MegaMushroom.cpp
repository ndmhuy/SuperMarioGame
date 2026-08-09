#include "Entities/MegaMushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MegaMushroom::MegaMushroom(sf::Vector2f pos) : Item(pos, {64.0f, 64.0f}) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
}

void MegaMushroom::update(float dt) {
    if (!active) return;
    Item::update(dt);
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void MegaMushroom::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("mega_mushroom");
    m_animation.frameList = {{"mushroom_red", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void MegaMushroom::render(sf::RenderTarget& target) {
    Item::render(target);
}

void MegaMushroom::activate(Player& player) {
    player.powerUp(5); // MegaMushroom type = 5
    player.addScore(1000);
}
