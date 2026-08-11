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
    // Use big_coin_0/1/2 from world_scenery_item for the spinning coin look.
    // big_coin_outline is HUD-only and should NOT appear in the world.
    m_animation = Animation("star_coin");
    m_animation.frameList = {{"big_coin_0", 0.12f}, {"big_coin_1", 0.12f}, {"big_coin_2", 0.12f}};
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
