#include "Entities/QuestionBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Core/GameSnapshot.hpp"
#include "Utils/Constants.hpp"

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
        // Powerup: ask for an item to be spawned on top of this block.
        //
        // This used to publish PowerUpCollected, which is the *pickup*
        // notification — nothing listened for it as a spawn request, so all 59
        // question blocks in the game awarded points and produced nothing, and
        // no power-up was reachable anywhere (audit B-2).
        SoundManager::getInstance().playSound("powerup_appears");
        PowerUpRequest request;
        request.itemType = m_containedItemType;
        request.spawnPosition = position - sf::Vector2f(0.0f, Constants::TILE_SIZE);
        EventBus::getInstance().publish({EventType::PowerUpRequested, request});
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
