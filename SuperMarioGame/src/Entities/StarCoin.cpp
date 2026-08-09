#include "Entities/StarCoin.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

StarCoin::StarCoin(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void StarCoin::update(float dt) {
    Item::update(dt);
}

void StarCoin::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("star_coin");
    m_animation.frameList = {{"big_coin", 0.15f}, {"big_coin_outline", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void StarCoin::render(sf::RenderTarget& target) {
    Item::render(target);
}

void StarCoin::activate(Player& player) {
    player.addScore(1000);
    EventBus::getInstance().publish({EventType::StarCoinCollected, this});
}
