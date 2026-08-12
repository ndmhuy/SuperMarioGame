#include "Entities/Star.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

Star::Star(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{100.0f, 0.0f};
    m_movingRight = true;
}

void Star::update(float dt) {
    if (!active) return;
    Item::update(dt);
    
    // Check wall collision (velocity.x cancelled to 0)
    if (std::abs(velocity.x) < 0.01f) {
        m_movingRight = !m_movingRight;
        velocity.x = m_movingRight ? 100.0f : -100.0f;
    }
    
    // Check ground bounce (velocity.y cancelled to 0 by floor collision)
    if (std::abs(velocity.y) < 0.01f) {
        velocity.y = -250.0f; // Bounce upward
    }
}

void Star::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("star");
    m_animation.frameList = {
        {"star_1", 0.15f},
        {"star_2", 0.15f},
        {"star_3", 0.15f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Star::render(sf::RenderTarget& target) {
    Item::render(target);
}

void Star::activate(Player& player) {
    player.powerUp(4); // Star type = 4
    player.addScore(1000);
}
