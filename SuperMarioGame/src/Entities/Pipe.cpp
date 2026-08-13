#include "Entities/Pipe.hpp"
#include "Entities/Player.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

Pipe::Pipe(sf::Vector2f position, int pipeId, sf::Vector2f exitPosition, std::string targetLevel, bool isEntrance, float rotationDegrees)
    : Block(position, {64.0f, 64.0f}), m_pipeId(pipeId), m_exitPosition(exitPosition), m_targetLevel(targetLevel), m_isEntrance(isEntrance), m_rotationDegrees(rotationDegrees) {
    m_breakable = false;
}

void Pipe::onHitFromBelow(Player& player) {
    // Solid block, does nothing from below except play normal bump sound if necessary
}

bool Pipe::checkWarp(Player& player) const {
    if (!m_isEntrance) return false;

    // Check if player is standing on top of the pipe horizontally
    float playerCenterX = player.getBoundingBox().x + player.getBoundingBox().width / 2.0f;
    bool withinHorizontalBounds = (playerCenterX >= position.x && playerCenterX <= position.x + boundingBox.width);

    // Check if player is on top of the pipe vertically
    float playerFeetY = player.getBoundingBox().y + player.getBoundingBox().height;
    bool onTop = (std::abs(playerFeetY - position.y) <= 4.0f);

    if (withinHorizontalBounds && onTop) {
        // In real gameplay, warp is triggered by pressing down (S or Down key)
        bool downPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || 
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
        if (downPressed) {
            // Trigger warp!
            SoundManager::getInstance().playSound("pipe");
            if (m_targetLevel.empty()) {
                // Same level teleportation
                player.setPosition(m_exitPosition);
                player.setVelocity({0.0f, 0.0f});
            } else {
                // Publish warp event for level switching
                EventBus::getInstance().publish({EventType::CheckpointActivated, m_pipeId}); // Warp event type mapping
            }
            return true;
        }
    }
    return false;
}

void Pipe::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_spriteSheet = spriteSheet;
    m_hasAnimation = (spriteSheet != nullptr);
}

void Pipe::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_spriteSheet) {
        PipeRenderer::draw(target, m_spriteSheet, sf::Vector2f(boundingBox.x, boundingBox.y), sf::Vector2f(boundingBox.width, boundingBox.height), m_rotationDegrees, true, "green");
    } else {
        Block::render(target);
    }
}
