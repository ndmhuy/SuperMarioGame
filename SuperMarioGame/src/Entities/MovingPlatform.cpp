#include "Entities/MovingPlatform.hpp"
#include "Entities/Player.hpp"
#include "Entities/TerrainProbe.hpp"
#include "Core/Game.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

MovingPlatform::MovingPlatform(sf::Vector2f position, sf::Vector2f travelRange, float speed)
    // Initialiser order must match the declaration order in the header, or
    // m_rangeLen reads m_travelRange before it is set (-Wreorder-ctor).
    : Block(position, {64.0f, 16.0f}), m_startPos(position), m_travelRange(travelRange),
      m_rangeLen(std::sqrt(travelRange.x * travelRange.x + travelRange.y * travelRange.y)),
      m_speed(speed) {
    m_breakable = false;
}

void MovingPlatform::onHitFromBelow(Player& player) {
    // Moving platform hit from below: standard bump behavior or nothing
}

AABB MovingPlatform::footprintAt(sf::Vector2f pos) const {
    return AABB{pos.x, pos.y, boundingBox.width, boundingBox.height};
}

void MovingPlatform::update(float dt) {
    // MovingPlatform fully overrides Block::update() to own its kinematic
    // motion, so the base implementation — the only place that advances
    // m_animator — never ran. The platform moved correctly but its texture
    // animation was permanently frozen on frame 0. Run it unconditionally,
    // before the early-return paths below, so a blocked/stationary platform
    // still animates (m_bumpTimer stays inert here: MovingPlatform never
    // calls onHitFromBelow's bump path, so this only drives the animator).
    Block::update(dt);

    if (!m_terrainProbed) {
        m_terrainProbed = true;
        m_startBlocked = TerrainProbe::overlapsSolid(footprintAt(position));
    }

    if (m_rangeLen <= 0.0f || m_startBlocked) {
        velocity = sf::Vector2f(0.0f, 0.0f);
        return;
    }

    const bool forward = m_movingForward;
    const float from = m_progress;
    float to = from + (forward ? 1.0f : -1.0f) * (m_speed / m_rangeLen) * dt;

    bool reachedEnd = false;
    if (forward && to >= m_maxProgress) {
        to = m_maxProgress;
        reachedEnd = true;
    } else if (!forward && to <= m_minProgress) {
        to = m_minProgress;
        reachedEnd = true;
    }

    // The path is parametric and nothing ever asked the tilemap about it, so a
    // platform whose configured sweep crossed a wall drove into that wall and
    // stayed there (R21 D5). Probe the destination before committing to it.
    if (to != from && TerrainProbe::overlapsSolid(footprintAt(m_startPos + m_travelRange * to))) {
        if (forward) {
            m_maxProgress = from;
        } else {
            m_minProgress = from;
        }
        m_movingForward = !forward;
        to = from;   // hold this frame; the shortened range takes over next one
    } else if (reachedEnd) {
        m_movingForward = !forward;
    }

    // Measured from the parametric step rather than from position, so it stays
    // the platform's own motion whatever else has touched the entity.
    const sf::Vector2f displacement = m_travelRange * (to - from);
    m_progress = to;
    setPosition(m_startPos + m_travelRange * to);
    velocity = (dt > 0.0f) ? displacement / dt : sf::Vector2f(0.0f, 0.0f);

    // Apply carrying logic if player is standing on top
    Player* player = Game::getInstance().getNearestPlayer(getPosition());
    if (player) {
        AABB pBox = player->getBoundingBox();
        AABB platBox = getBoundingBox();

        // 1. Horizontal overlap check
        bool xOverlap = (pBox.x + pBox.width > platBox.x) && (pBox.x < platBox.x + platBox.width);
        
        // 2. Vertical bottom edge close to platform top check (3px tolerance)
        bool yOverlap = std::abs((pBox.y + pBox.height) - platBox.y) < 3.0f;
        
        // 3. Downward/horizontal movement or resting on platform
        bool resting = player->getVelocity().y >= 0.0f;

        if (xOverlap && yOverlap && resting) {
            player->setPosition(player->getPosition() + displacement);
        }
    }
}

void MovingPlatform::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("moving_platform");
    // This named "platform_medium", which world_scenery_item does not contain.
    // setupAnimations() sets m_hasAnimation whether or not the frames exist,
    // and drawSprite() returns early on a zero-size sprite, so the moving platform
    // drew nothing at all — not even the placeholder rectangle.
    m_animation.frameList = {{"half_platform_long", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void MovingPlatform::render(sf::RenderTarget& target) {
    Block::render(target);
}
