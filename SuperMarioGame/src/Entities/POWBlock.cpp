#include "Entities/POWBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

POWBlock::POWBlock(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void POWBlock::update(float dt) {
    Item::update(dt);
}

void POWBlock::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("pow_block");
    m_animation.frameList = {
        {"pow_block_0", 0.08f},
        {"pow_block_1", 0.08f},
        {"pow_block_2", 0.08f},
        {"pow_block_3", 0.08f},
        {"pow_block_4", 0.08f},
        {"pow_block_5", 0.08f},
        {"pow_block_6", 0.08f},
        {"pow_block_7", 0.08f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void POWBlock::render(sf::RenderTarget& target) {
    Item::render(target);
}

void POWBlock::activate(Player& player) {
    // Triggers a screen-shake and flips all grounded enemies via EventBus
    EventBus::getInstance().publish({EventType::POWBlockHit, this});
}
