#include "Entities/HiddenBlock.hpp"
#include "Entities/Player.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Core/SoundManager.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

HiddenBlock::HiddenBlock(sf::Vector2f position, int itemType)
    // Initialiser order must match the declaration order in the header
    // (-Wreorder-ctor).
    : Block(position), m_isRevealed(false), m_containedItemType(itemType) {
    m_breakable = false;
}

BlockTouch HiddenBlock::onCharacterTouch(Character& character, const CollisionInfo& info) {
    // An unrevealed hidden block is solid only from underneath. Any other
    // approach passes straight through, so nobody bumps into invisible geometry
    // while running or falling past it.
    (void)character;
    if (!m_isRevealed && info.normal.y != 1.0f) return BlockTouch::Pass;
    return BlockTouch::Solid;
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

void HiddenBlock::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("hidden_block");
    m_animation.frameList = {{"solid_block_brown", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void HiddenBlock::render(sf::RenderTarget& target) {
    if (m_isRevealed) {
        Block::render(target);
    }
}

bool HiddenBlock::isCollidable() const {
    // A hidden block is ALWAYS collidable — that is the whole mechanic. It was
    // returning a zero-sized AABB while unrevealed, so it never collided, so
    // onHitFromBelow never fired, so it could never be revealed: a deadlock that
    // made every hidden block in the game unreachable (audit B-3).
    //
    // Invisibility is a rendering concern, handled in render(). Solidity from
    // above is handled in resolveCharacterVsBlock, which ignores unrevealed
    // blocks so the player does not bump an invisible ceiling while running.
    return true;
}
