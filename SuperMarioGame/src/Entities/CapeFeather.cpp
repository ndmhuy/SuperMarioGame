#include "Entities/CapeFeather.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

CapeFeather::CapeFeather(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void CapeFeather::update(float dt) {
    Item::update(dt);
}

void CapeFeather::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("cape_feather");
    m_animation.frameList = {{"cape_feather", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void CapeFeather::render(sf::RenderTarget& target) {
    Item::render(target);
}

void CapeFeather::activate(Player& player) {
    player.powerUp(2); // CapeFeather type = 2
    player.addScore(1000);
}
