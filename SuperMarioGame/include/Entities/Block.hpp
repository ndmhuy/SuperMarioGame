#pragma once

#include "Entities/Entity.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Player;
class Character;
struct CollisionInfo;

// What a character's touch does to a block.
//
// Two blocks are not simply solid: an unrevealed HiddenBlock is solid only from
// underneath, and a Flagpole is a trigger that never blocks anyone. The
// resolver used to name both by dynamic_cast (audit A-10 / D8).
enum class BlockTouch {
    Solid,   // the ordinary block: push the character out and cancel velocity
    Pass     // no physical response; the block has answered the contact itself
};

class Block : public Entity {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    explicit Block(sf::Vector2f position, sf::Vector2f targetSize = {32.0f, 32.0f});
    ~Block() override = default;

    // Triggered when hit from below by a player
    virtual void onHitFromBelow(Player& player) = 0;

    // Answer a character's touch on this block's own terms.
    //
    // Called by CollisionResolver once per contact, before any physical
    // response. An override may act on the character and must then say whether
    // the block still pushes it out. Companion to onHitFromBelow(), which stays
    // the hook for the ceiling-strike case of an ordinary solid block.
    virtual BlockTouch onCharacterTouch(Character& character, const CollisionInfo& info);

    // Overrides Entity lifecycle & physics
    float getGravityMultiplier() const override { return 0.0f; }
    EntityCategory getCategory() const override { return EntityCategory::Block; }
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    bool isBreakable() const { return m_breakable; }


protected:
    bool m_breakable = false;
    bool m_isHit = false;
    float m_bumpTimer = 0.0f;
    sf::Vector2f m_originalPosition;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};

