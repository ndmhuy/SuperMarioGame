#pragma once

#include "Physics/AABB.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>

class PhysicsEngine;
class CollisionResolver;

class Entity {
public:
    explicit Entity(sf::Vector2f pos = {0.0f, 0.0f}, sf::Vector2f targetSize = {32.0f, 32.0f});
    virtual ~Entity() = default;

    // Pure virtual lifecycle methods
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;

    // Virtual physics/state queries
    virtual const AABB& getBoundingBox() const;
    virtual bool isActive() const;
    virtual void destroy();
    virtual float getGravityMultiplier() const { return 1.0f; }
    virtual bool collidesWithTiles() const { return true; }

    // Getters/Setters for external access
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    sf::Vector2f getTargetSize() const { return m_targetSize; }

    // Stable per-instance handle, assigned at construction and never reused.
    // Rewind snapshots key on this rather than on the entity's index in
    // PlayingState::m_entities, which shifts whenever an entity is pruned or
    // spawned mid-sequence (audit A-5).
    std::uint32_t getId() const { return m_id; }

    // Setters for coordinates
    void setPosition(sf::Vector2f pos);
    void setVelocity(sf::Vector2f vel);
    void setTargetSize(sf::Vector2f size) { m_targetSize = size; boundingBox.width = size.x; boundingBox.height = size.y; }

protected:
    // Where a sprite's origin sits relative to the entity's bounding box.
    enum class SpriteAnchor {
        TopLeft,       // blocks: sprite fills the box from its top-left corner
        BottomCenter   // characters and items: feet planted, centred horizontally
    };

    // Aspect-fit a sprite into m_targetSize and draw it against the bounding box.
    //
    // Player, Enemy, Block and Item each carried their own copy of this maths,
    // including the same two dead locals in every copy (audit X-6). They differed
    // only in anchor and flip, which are parameters here.
    //
    // `overrideScale > 0` pins the scale instead of recomputing it, for callers
    // that cache a base scale so animation frames of differing size do not make
    // the entity pulse.
    void drawSprite(sf::RenderTarget& target,
                    sf::Sprite sprite,
                    SpriteAnchor anchor,
                    bool flipX = false,
                    bool flipY = false,
                    float overrideScale = 0.0f) const;

    // Debug rectangle used when no sprite sheet is wired up.
    void drawPlaceholder(sf::RenderTarget& target, sf::Color fill) const;

    // Friends are allowed direct write access to coordinate updates
    friend class PhysicsEngine;
    friend class CollisionResolver;
    friend class PlayingState;
    friend class IMovementStrategy;
    friend class PatrolStrategy;
    friend class ChaseStrategy;
    friend class FlyStrategy;
    friend class TimerEmergenceStrategy;
    friend class LinearStrategy;
    friend class HammerThrowStrategy;
    friend class TetheredChaseStrategy;
    friend class ProximityTriggerStrategy;
    friend int main();

    sf::Vector2f position;
    sf::Vector2f velocity;
    bool active = true;
    AABB boundingBox;
    sf::Vector2f m_targetSize{32.0f, 32.0f};

private:
    std::uint32_t m_id;
    static std::uint32_t s_nextId;
};

