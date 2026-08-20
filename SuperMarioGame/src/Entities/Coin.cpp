#include "Entities/Coin.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <algorithm>
#include <cmath>

Coin::Coin(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void Coin::update(float dt) {
    Item::update(dt);
}

void Coin::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("coin");
    m_animation.frameList = {
        {"coin_0", 0.15f},
        {"coin_1", 0.15f},
        {"coin_2", 0.15f},
        {"coin_3", 0.15f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
        sf::FloatRect bounds = m_animator->getSprite().getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            m_baseScale = std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);
        }
    }
}

void Coin::render(sf::RenderTarget& target) {
    Item::render(target);
}

void Coin::activate(Player& player) {
    player.addCoins(1);
    player.addScore(200);
}
