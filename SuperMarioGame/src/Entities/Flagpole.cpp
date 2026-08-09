#include "Entities/Flagpole.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include <algorithm>

Flagpole::Flagpole(sf::Vector2f position, float poleHeight)
    : Block(position, {32.0f, 300.0f}), m_poleHeight(poleHeight), m_triggered(false) {
    m_breakable = false;
    boundingBox.height = m_poleHeight;
}

void Flagpole::onHitFromBelow(Player& player) {
    // Flagpole is not hit from below
}

void Flagpole::onPlayerCollision(Player& player, float collisionY) {
    if (m_triggered) return;
    m_triggered = true;

    // Calculate height caught relative to flagpole top
    // position.y is the top of the flagpole
    // flagpole bottom is position.y + m_poleHeight
    float distanceSelfFromTop = collisionY - position.y;
    float heightFromBottom = m_poleHeight - distanceSelfFromTop;

    // Clamp between 0 and m_poleHeight
    heightFromBottom = std::max(0.0f, std::min(heightFromBottom, m_poleHeight));

    float percentage = heightFromBottom / m_poleHeight;
    int points = 100;
    if (percentage >= 0.8f) {
        points = 5000;
    } else if (percentage >= 0.6f) {
        points = 2000;
    } else if (percentage >= 0.4f) {
        points = 800;
    } else if (percentage >= 0.2f) {
        points = 400;
    }

    player.addScore(points);
    SoundManager::getInstance().playSound("flagpole");
    EventBus::getInstance().publish({EventType::LevelComplete, points});
}

void Flagpole::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("flagpole");
    m_animation.frameList = {{"flag_white", 0.15f}, {"flag_reddish_white", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Flagpole::render(sf::RenderTarget& target) {
    Block::render(target);
}
