#include "Core/InputManager.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Player.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>

Pipe::Pipe(sf::Vector2f position, int pipeId, sf::Vector2f exitPosition, std::string targetLevel, bool isEntrance, float rotationDegrees, std::string color)
    : Block(position, {WIDTH_PX, HEIGHT_PX}), m_pipeId(pipeId), m_exitPosition(exitPosition), m_targetLevel(std::move(targetLevel)), m_isEntrance(isEntrance), m_rotationDegrees(rotationDegrees), m_color(std::move(color)) {
    m_breakable = false;
}

void Pipe::onHitFromBelow(Player& player) {
    // Solid block, does nothing from below except play normal bump sound if necessary
}

void Pipe::configureWarp(int pipeId, bool isEntrance, std::string targetLevel,
                         sf::Vector2f exitPosition) {
    m_pipeId = pipeId;
    m_isEntrance = isEntrance;
    m_targetLevel = std::move(targetLevel);
    m_exitPosition = exitPosition;
}

void Pipe::setShaftRise(float pixels) {
    m_shaftRise = std::max(0.0f, pixels);
}

sf::Vector2f Pipe::getMouthCenter() const {
    switch (m_entryMode) {
        case EntryMode::SideLeft:
            return {position.x, position.y + boundingBox.height - mouthHeight() * 0.5f};
        case EntryMode::SideRight:
            return {position.x + boundingBox.width,
                    position.y + boundingBox.height - mouthHeight() * 0.5f};
        case EntryMode::Top:
        default:
            return {position.x + boundingBox.width * 0.5f, position.y};
    }
}

float Pipe::getSideApproachDirection() const {
    switch (m_entryMode) {
        case EntryMode::SideLeft:  return  1.0f;   // mouth faces west: walk east
        case EntryMode::SideRight: return -1.0f;
        case EntryMode::Top:
        default:                   return  0.0f;
    }
}

bool Pipe::isAtEntryPoint(const Player& player) const {
    const AABB box = player.getBoundingBox();

    if (m_entryMode == EntryMode::Top) {
        // Standing on top of the pipe, horizontally over it...
        const float playerCenterX = box.x + box.width / 2.0f;
        const bool withinHorizontalBounds =
            (playerCenterX >= position.x - 4.0f &&
             playerCenterX <= position.x + boundingBox.width + 4.0f);

        // ...and with their feet on or near its rim.
        const float playerFeetY = box.y + box.height;
        const bool onTop = (playerFeetY >= position.y - 6.0f &&
                            playerFeetY <= position.y + 10.0f);
        return withinHorizontalBounds && onTop;
    }

    // Side entry: standing on the floor beside the mouth.
    //
    // Grounded is required, not incidental. The mouth is the bottom
    // mouthHeight() of the collider, so an airborne player brushing the
    // shaft three tiles up must not count — and a pipe is solid, so a player
    // walking into one comes to rest with their leading edge flush against
    // this face, which is what the small tolerances below are sized for.
    if (!player.isOnGround()) return false;

    const float mouthTop    = position.y + boundingBox.height - mouthHeight();
    const float mouthBottom = position.y + boundingBox.height;
    const float feetY = box.y + box.height;
    // Half a tile of slack below the arm covers a floor that the pipe's foot
    // sinks into rather than resting exactly on.
    if (feetY < mouthTop + 8.0f || feetY > mouthBottom + 16.0f) return false;

    if (m_entryMode == EntryMode::SideLeft) {
        const float face = position.x;
        return (box.x + box.width) >= face - 6.0f && box.x <= face + Constants::TILE_SIZE;
    }
    const float face = position.x + boundingBox.width;
    return box.x <= face + 6.0f && (box.x + box.width) >= face - Constants::TILE_SIZE;
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
    if (!isAtEntryPoint(player)) return false;

    InputManager& input = InputManager::getInstance();
    const int pad = player.getPlayerIndex();

    if (m_entryMode == EntryMode::Top) {
        // Asks for the bound crouch key or player crouched state
        return player.isCrouched() || input.isActionHeld("crouch", pad) ||
               input.isActionHeld("groundpound", pad);
    }

    // Walk INTO the mouth. Crouch is not accepted here on purpose: an up-pipe
    // has no opening on its top, so pressing Down beside one must do nothing.
    return input.isActionHeld(m_entryMode == EntryMode::SideLeft ? "right" : "left", pad);
}

void Pipe::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_spriteSheet = spriteSheet;
    m_hasAnimation = (spriteSheet != nullptr);
}

PipeRenderer::Shape Pipe::artShape() const {
    switch (m_entryMode) {
        case EntryMode::SideLeft:  return PipeRenderer::Shape::LBendMouthWest;
        case EntryMode::SideRight: return PipeRenderer::Shape::LBendMouthEast;
        case EntryMode::Top:
        default:                   return PipeRenderer::Shape::VerticalTop;
    }
}

void Pipe::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_spriteSheet) {
        // A Pipe is one whole pipe, never a body segment stacked under another
        // Pipe: no level does that, and the 4-tile collider is there so none
        // has to. What the art has to say instead is which WAY this pipe goes,
        // and that is the EntryMode's job — the renderer is told the shape, not
        // left to guess it from the collider's aspect ratio. Guessing is what
        // shipped a 2x4 warp pipe drawn from an L-bend with no rim on it.
        PipeRenderer::draw(target, m_spriteSheet,
                           sf::Vector2f(boundingBox.x, boundingBox.y),
                           sf::Vector2f(boundingBox.width, boundingBox.height),
                           m_rotationDegrees, artShape(), m_color, m_shaftRise);
    } else {
        Block::render(target);
    }
}
