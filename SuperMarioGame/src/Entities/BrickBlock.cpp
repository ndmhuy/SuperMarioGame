#include "Entities/BrickBlock.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

BrickBlock::BrickBlock(sf::Vector2f position, int coins)
    : Block(position), m_coinsLeft(coins), m_isEmpty(coins == 0) {
    m_breakable = true;
}

void BrickBlock::onHitFromBelow(Player& player) {
    // Check if player is Super size or larger (height >= 48px, since Small Mario is 32px)
    bool isSuperOrLarger = (player.getBoundingBox().height >= 48.0f);

    if (isSuperOrLarger && m_isEmpty) {
        // Break the block!
        SoundManager::getInstance().playSound("break_block");
        EventBus::getInstance().publish({EventType::BlockBroken, 100}); // 100 points
        player.addScore(100);
        this->active = false;
    } else {
        // Just bump/bounce the block
        if (!m_isHit) {
            m_isHit = true;
            m_bumpTimer = 0.15f;
            m_originalPosition = position;

            if (m_coinsLeft > 0) {
                m_coinsLeft--;
                player.addCoins(1);
                player.addScore(200);
                SoundManager::getInstance().playSound("coin");
                if (m_coinsLeft == 0) {
                    m_isEmpty = true;
                }
            } else {
                SoundManager::getInstance().playSound("bump");
            }
        }
    }
}

void BrickBlock::render(sf::RenderTarget& target) {
    // Render blank or empty block if m_isEmpty, otherwise brick texture
}
