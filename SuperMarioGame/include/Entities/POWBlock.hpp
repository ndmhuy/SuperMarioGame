#pragma once

#include <string>

#include "Entities/Item.hpp"

// The POW block is a *solid* item you strike, not a pickup you walk into.
//
// It used to fall through Item's default collision path, so brushing it
// sideways "collected" it: one touch, one flip's worth of nothing (the event
// had no listener), and the block vanished. It now takes three strikes — from
// below, or a stomp from above — and stands as a platform in between, which is
// what makes it usable as terrain as well as a weapon.
class POWBlock : public Item {
public:
    explicit POWBlock(sf::Vector2f pos);
    ~POWBlock() override = default;

    std::string getTypeName() const override { return "pow_block"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    ItemTouch onPlayerTouch(Player& player, const CollisionInfo& info) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Strikes left before the block is spent. Three is the arcade figure, and it
    // is what lets a POW cover a whole wave rather than one enemy.
    int getChargesLeft() const { return m_charges; }
    bool isSpent() const { return m_charges <= 0; }

private:
    static constexpr int MAX_CHARGES = 3;
    int m_charges = MAX_CHARGES;
    // Contact persists for several frames, so without this one strike would
    // spend every charge in a fifth of a second.
    float m_strikeCooldown = 0.0f;
};
