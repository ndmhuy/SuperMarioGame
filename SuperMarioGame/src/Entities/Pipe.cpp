#include "Core/InputManager.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Player.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

Pipe::Pipe(sf::Vector2f position, int pipeId, sf::Vector2f exitPosition, std::string targetLevel, bool isEntrance, float rotationDegrees, std::string color)
    : Block(position, {WIDTH_PX, HEIGHT_PX}), m_pipeId(pipeId), m_exitPosition(exitPosition), m_targetLevel(std::move(targetLevel)), m_isEntrance(isEntrance), m_rotationDegrees(rotationDegrees), m_color(std::move(color)) {
    m_breakable = false;
}

void Pipe::onHitFromBelow(Player& player) {
    // Solid block, does nothing from below except play normal bump sound if necessary
}

bool Pipe::checkWarp(const Player& player) const {
    // Deliberately side-effect free.
    //
    // This used to play the pipe sound, teleport the player and publish an event
    // from inside a const query that PlayingState called once per pipe per
    // frame. Holding Down on a pipe re-fired all of it every frame, and the
    // event it published was CheckpointActivated — so every trip down a pipe
    // also set a respawn point, ran an auto-save and played the checkpoint
    // jingle. Two overlapping sounds a frame is what "a madness of music" was.
    if (!m_isEntrance) return false;

    // Standing on top of the pipe, horizontally over it...
    const float playerCenterX = player.getBoundingBox().x + player.getBoundingBox().width / 2.0f;
    const bool withinHorizontalBounds =
        (playerCenterX >= position.x - 4.0f && playerCenterX <= position.x + boundingBox.width + 4.0f);

    // ...and with their feet on or near its rim.
    const float playerFeetY = player.getBoundingBox().y + player.getBoundingBox().height;
    const bool onTop = (playerFeetY >= position.y - 6.0f && playerFeetY <= position.y + 10.0f);
    if (!withinHorizontalBounds || !onTop) return false;

    // Asks for the bound crouch key or player crouched state
    InputManager& input = InputManager::getInstance();
    const int pad = player.getPlayerIndex();
    return player.isCrouched() || input.isActionHeld("crouch", pad) || input.isActionHeld("groundpound", pad);
}

void Pipe::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_spriteSheet = spriteSheet;
    m_hasAnimation = (spriteSheet != nullptr);
}

void Pipe::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_spriteSheet) {
        // hasHead is always true: a Pipe is a whole pipe, not a body segment.
        // Stacking two Pipe entities to make a taller one would need the upper
        // one to own the head — no level does that, and the 2x4 collider is
        // there so none has to.
        PipeRenderer::draw(target, m_spriteSheet, sf::Vector2f(boundingBox.x, boundingBox.y), sf::Vector2f(boundingBox.width, boundingBox.height), m_rotationDegrees, true, m_color);
    } else {
        Block::render(target);
    }
}
