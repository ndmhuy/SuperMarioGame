#include "Entities/IceBlock.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

IceBlock::IceBlock(sf::Vector2f position)
    : Block(position) {
    m_breakable = false;
}

void IceBlock::onHitFromBelow(Player& player) {
    // Ice block hit from below
}

void IceBlock::update(float dt) {
    Block::update(dt);
}

void IceBlock::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("ice_block");
    m_animation.frameList = {{"solid_block_blue", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void IceBlock::render(sf::RenderTarget& target) {
    Block::render(target);
}
