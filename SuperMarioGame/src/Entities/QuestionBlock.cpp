#include "Entities/QuestionBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

QuestionBlock::QuestionBlock(sf::Vector2f position, int itemType)
    : Block(position), m_containedItemType(itemType), m_isEmpty(false) {
    m_breakable = false;
}

void QuestionBlock::onHitFromBelow(Player& player) {
    if (m_isEmpty) {
        SoundManager::getInstance().playSound("bump");
        return;
    }

    if (!m_isHit) {
        m_isHit = true;
        m_bumpTimer = 0.15f;
        m_originalPosition = position;

        if (m_containedItemType == 0) {
            // Coin
            player.addCoins(1);
            player.addScore(200);
            SoundManager::getInstance().playSound("coin");
        } else {
            // Powerup
            SoundManager::getInstance().playSound("powerup_appears");
            // Publish event to notify EntityFactory/PlayingState to spawn the item
            EventBus::getInstance().publish({EventType::PowerUpCollected, m_containedItemType});
            player.addScore(1000);
        }

        m_isEmpty = true;
    }
}

void QuestionBlock::render(sf::RenderTarget& target) {
    // Render ? block texture or empty block texture based on m_isEmpty
}
