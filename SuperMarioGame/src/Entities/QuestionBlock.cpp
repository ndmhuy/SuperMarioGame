#include "Entities/QuestionBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

QuestionBlock::QuestionBlock(sf::Vector2f position, int itemType)
    : Block(position), m_containedItemType(itemType), m_isEmpty(false) {
    m_breakable = false;
}

void QuestionBlock::onHitFromBelow(Player& player) {
    if (!m_isHit) {
        m_isHit = true;
        m_bumpTimer = 0.20f;
        m_originalPosition = position;
    }

    if (m_isEmpty) {
        SoundManager::getInstance().playSound("bump");
        return;
    }

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

void QuestionBlock::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("question_block");
    m_animation.frameList = {
        {"question_block_0", 0.15f},
        {"question_block_1", 0.15f},
        {"question_block_2", 0.15f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void QuestionBlock::render(sf::RenderTarget& target) {
    Block::render(target);
}
