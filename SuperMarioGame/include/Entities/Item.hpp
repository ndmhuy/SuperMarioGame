#pragma once

#include "Entities/Entity.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Player;
struct CollisionInfo;

// What a player's touch does to an item.
//
// The resolver used to name Trampoline, POWBlock and PSwitch by hand, with one
// dynamic_cast each, to find out which of these three answers applied (audit
// A-10 / D8). The item now answers for itself, and the resolver still owns
// every line that moves the player.
enum class ItemTouch {
    Collect,   // the ordinary powerup: activate() then collect()
    Solid,     // terrain: push the player out and cancel velocity along the normal
    Consumed   // the item has fully answered the contact; no physical response
};

class Item : public Entity {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    explicit Item(sf::Vector2f pos, sf::Vector2f targetSize = {32.0f, 32.0f});
    ~Item() override = default;

    // Collect/Apply powerup callbacks
    virtual void activate(Player& player);
    virtual void collect();

    // Answer a player's touch on this item's own terms.
    //
    // Called by CollisionResolver once per contact, before any physical
    // response. An override may act on the player (activate it, bounce it) and
    // must then say what the resolver should do about the overlap. An item with
    // new contact rules overrides this; the resolver never learns its name.
    virtual ItemTouch onPlayerTouch(Player& player, const CollisionInfo& info);
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    EntityCategory getCategory() const override { return EntityCategory::Item; }

    // Ground status & read-only getter
    bool isCollected() const { return collected; }
    bool isOnGround() const { return m_onGround; }
    void setOnGround(bool grounded) { m_onGround = grounded; }

protected:
    bool collected = false;
    bool m_onGround = false;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
    float m_baseScale = 0.0f;
    const SpriteSheet* m_spriteSheet = nullptr;
};


