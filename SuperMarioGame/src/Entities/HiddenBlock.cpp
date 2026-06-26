#include "Entities/HiddenBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

HiddenBlock::HiddenBlock(sf::Vector2f position, int itemType)
    : Block(position), m_containedItemType(itemType), m_isRevealed(false) {
    m_breakable = false;
}

void HiddenBlock::onHitFromBelow(Player& player) {
    if (!m_isRevealed) {
        m_isRevealed = true;
        m_isHit = true;
        m_bumpTimer = 0.15f;
        m_originalPosition = position;

        if (m_containedItemType == 0) {
            player.addCoins(1);
            player.addScore(200);
            SoundManager::getInstance().playSound("coin");
        } else {
            SoundManager::getInstance().playSound("powerup_appears");
            player.addScore(1000);
        }
    }
}

void HiddenBlock::update(float dt) {
    if (m_isRevealed) {
        Block::update(dt);
    }
}

void HiddenBlock::render(sf::RenderTarget& target) {
    if (m_isRevealed) {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(position);
        rect.setFillColor(sf::Color(100, 100, 100)); // Revealed block is solid gray
        target.draw(rect);
    }
}

const AABB& HiddenBlock::getBoundingBox() const {
    static const AABB emptyBox{0.f, 0.f, 0.f, 0.f};
    if (!m_isRevealed) {
        return emptyBox;
    }
    return boundingBox;
}
