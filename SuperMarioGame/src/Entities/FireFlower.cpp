#include "Entities/FireFlower.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

FireFlower::FireFlower(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void FireFlower::update(float dt) {
    Item::update(dt);
}

void FireFlower::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("fire_flower");
    m_animation.frameList = {
        {"fire_flower_green_0", 0.15f},
        {"fire_flower_green_1", 0.15f},
        {"fire_flower_green_2", 0.15f},
        {"fire_flower_green_3", 0.15f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void FireFlower::render(sf::RenderTarget& target) {
    Item::render(target);
}

void FireFlower::activate(Player& player) {
    player.powerUp(1); // FireFlower type = 1
    player.addScore(1000);
}
