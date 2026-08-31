#pragma once

#include "Physics/AABB.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <string>

class PhysicsEngine;
class CollisionResolver;
struct CollisionInfo;

// Broad kind of an entity, reported by the entity itself.
//
// CollisionResolver used to identify both sides of a collision with up to twelve
// sequential dynamic_casts, per colliding pair, per frame (audit A-10). Asking
// the object what it is turns that into two virtual calls and a switch.
//
// This is a category, not a type id: it answers "how does this collide?", which
// is exactly the question the resolver asks. Anything needing the concrete type
// should use a virtual of its own instead of widening this enum.
enum class EntityCategory {
    Unknown,
    Player,
    Enemy,
    Item,
    Block,
    Projectile
};

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

    // Brings a destroyed entity back. The object pool needs this (a recycled
    // object is reactivated rather than reconstructed) and so do the harnesses
    // that respawn a test player, both of which used to assign to the protected
    // `active` flag through friendship.
    void revive() { active = true; }
    virtual float getGravityMultiplier() const { return 1.0f; }

    // Is this entity resting on something solid?
    //
    // Characters and Items each already answered this, with the same name and
    // signature, but neither answer was reachable through an Entity& — so
    // PhysicsEngine::applyGravity ran a dynamic_cast to Character and another to
    // Item just to ask (audit A-10 / D8). Everything else (blocks, projectiles)
    // never rests on anything, so the base answer is "no" and gravity applies.
    virtual bool isOnGround() const { return false; }

    // Does the physics engine own this entity's motion this frame?
    //
    // A dead or held enemy is driven by its own death or carry animation, so
    // gravity and tile collision must leave it alone. PhysicsEngine asked this
    // three times per frame — once in the gravity pass and once in each of the
    // X and Y integration passes — with a dynamic_cast to Enemy each time.
    virtual bool isPhysicsDriven() const { return true; }

    // What kind of thing this is, for collision dispatch. Overridden once per
    // base class (Player, Enemy, Item, Block, Fireball) — concrete subclasses
    // inherit it.
    virtual EntityCategory getCategory() const { return EntityCategory::Unknown; }

    // Stable serialisation name for this concrete type, e.g. "goomba".
    // Replaces a 30-deep dynamic_cast chain in SerializationUtils (audit A-10).
    // As a virtual it is also correct for derived types: the cast chain tested
    // KoopaTroopa before KoopaParatroopa, so every Paratroopa was written out as
    // a plain "koopa_troopa".
    virtual std::string getTypeName() const { return "unknown"; }
    virtual bool collidesWithTiles() const { return true; }

    // Answer a collision with the tile map on this entity's own terms.
    //
    // Returns true if the entity consumed the impact, in which case the
    // resolver's push-out does not run. A fireball bursts against a wall and
    // bounces off a floor; PhysicsEngine used to encode that itself, behind a
    // dynamic_cast to Fireball in each of the X and Y passes (audit A-10 / D8).
    virtual bool onTileImpact(const CollisionInfo& info) { (void)info; return false; }

    // Whether this entity takes part in entity-vs-entity collision right now.
    //
    // Five classes used to express "ignore me" by returning a zero-sized AABB
    // from getBoundingBox(). That worked only because AABB::intersects uses a
    // strict comparison, it inserted every such entity into spatial-hash cell
    // (0,0) each frame, and it lied to anything that asked where they were
    // (audit B-14). Say it directly instead.
    virtual bool isCollidable() const { return true; }

    // Does this entity only ever act as a moving hazard to the human player?
    //
    // Shadow Mario is a Player subclass so that it moves, animates and collides
    // with the level exactly as the player it is replaying. That makes it a
    // Player to the collision dispatcher too, which would let it stomp Goombas,
    // eat mushrooms and punch question blocks — clearing the level on behalf of
    // the player it is supposed to be chasing. A hazard is resolved against the
    // human and ignored by everything else.
    //
    // A virtual rather than another EntityCategory: the category answers "how
    // does this collide", and a shadow collides as a Player. This answers a
    // narrower question, which is what EntityCategory's own comment asks callers
    // to do instead of widening the enum.
    virtual bool isContactHazard() const { return false; }

    // Told to a contact hazard when it has just hurt the player. Lets the hazard
    // record that it was the cause, rather than leaving anything downstream to
    // infer causation from proximity — which is wrong: a player killed by a
    // Goomba while standing next to Shadow Mario was reported as having been
    // caught by their shadow.
    virtual void onContactWithPlayer() {}

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

    // Will this entity draw real artwork, or fall back to drawPlaceholder()?
    //
    // Every render() in the codebase branches on "animator && m_hasAnimation",
    // but each subclass tree keeps its own copy of that flag, so there was no
    // way to ask. Without it, a missing sprite is only discoverable by looking
    // at the screen — which is how moving platforms shipped as brown rectangles.
    virtual bool hasArtwork() const { return false; }

    // The size of the sprite this entity would draw right now, or (0,0) if it
    // would draw nothing. hasArtwork() only says a frame list was installed —
    // setupAnimations() sets that flag whether or not the frame names it asked
    // for exist in the atlas. A moving platform named "platform_medium", the
    // atlas had no such frame, and drawSprite() bails on a zero-size sprite, so
    // the platform drew *nothing at all* — not even the placeholder rectangle.
    virtual sf::Vector2f artworkSize() const { return {0.0f, 0.0f}; }

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

    sf::Vector2f position;
    sf::Vector2f velocity;
    bool active = true;
    AABB boundingBox;
    sf::Vector2f m_targetSize{32.0f, 32.0f};

private:
    std::uint32_t m_id;
    static std::uint32_t s_nextId;
};

