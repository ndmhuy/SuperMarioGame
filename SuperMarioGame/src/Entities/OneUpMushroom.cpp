#include "Entities/OneUpMushroom.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

OneUpMushroom::OneUpMushroom(sf::Vector2f pos) : Item(pos, {32.0f, 32.0f}) {
    velocity = sf::Vector2f{80.0f, 0.0f};
    m_movingRight = true;
    setTargetSize({32.0f, 32.0f});
}

void OneUpMushroom::update(float dt) {
    if (!active) return;
    Item::update(dt);
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 80.0f : -80.0f;
    }
}

void OneUpMushroom::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("one_up_mushroom");
    m_animation.frameList = {{"mushroom_green", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void OneUpMushroom::render(sf::RenderTarget& target) {
    Item::render(target);
}

void OneUpMushroom::activate(Player& player) {
    player.gainLife();
}
