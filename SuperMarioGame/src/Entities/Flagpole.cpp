#include "Entities/Flagpole.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include <algorithm>

Flagpole::Flagpole(sf::Vector2f position, float poleHeight)
    : Block(position, {24.0f, 168.0f}), m_poleHeight(poleHeight > 0.0f ? poleHeight : 168.0f), m_triggered(false) {
    m_breakable = false;
    setTargetSize({24.0f, 168.0f});
    boundingBox = AABB{position.x, position.y, 24.0f, m_poleHeight};
}

void Flagpole::update(float dt) {
    Block::update(dt);
}

void Flagpole::onHitFromBelow(Player& player) {
    // Flagpole is not hit from below
}

void Flagpole::onPlayerCollision(Player& player, float collisionY) {
    if (m_triggered) return;
    m_triggered = true;

    // Calculate height caught relative to flagpole top
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

    // Swap to the descent animation so the flag visibly slides down the pole.
    m_animation = m_descentAnimation;
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Flagpole::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);

    // These used to name "flag_white" and "flag_reddish_white", which are in no
    // atlas — so the flagpole drew nothing at all. The world atlas carries a
    // raised flag (pole_flag_green) and a five-frame descent
    // (full_flag_pole_0..4), which is the animation the flag actually needs.
    m_raisedAnimation = Animation("flagpole_raised");
    m_raisedAnimation.frameList = {{"pole_flag_green", 0.20f}};
    m_raisedAnimation.isLooping = true;

    m_descentAnimation = Animation("flagpole_descent");
    m_descentAnimation.frameList = {
        {"full_flag_pole_0", 0.18f}, {"full_flag_pole_1", 0.18f},
        {"full_flag_pole_2", 0.18f}, {"full_flag_pole_3", 0.18f},
        {"full_flag_pole_4", 0.18f}
    };
    // The flag comes to rest at the bottom; it must not loop back to the top.
    m_descentAnimation.isLooping = false;

    m_animation = m_raisedAnimation;
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Flagpole::render(sf::RenderTarget& target) {
    Block::render(target);
}
