#pragma once

#include <string>

#include "Entities/Item.hpp"

// The axe at the end of Bowser's bridge.
//
// Why this exists
// ---------------
// Bowser was reported as "nearly impossible": he is immune to fire, he carries
// a second of invulnerability after every hit during which touching him hurts
// you, and he breathes fire the whole time. A player who cannot land five
// clean descending stomps under that pressure has no way through the level at
// all.
//
// The series has always answered this with a second, non-combat solution — you
// do not have to beat Bowser, you have to get past him and reach the axe. This
// is that axe. Touching it publishes BridgeChopped; PlayingState drops the
// bridge tiles into the lava and takes the boss with them.
//
// The atlas has shipped axe_0..axe_2 since the beginning and nothing ever drew
// them.
class BridgeAxe : public Item {
public:
    explicit BridgeAxe(sf::Vector2f pos);
    ~BridgeAxe() override = default;

    std::string getTypeName() const override { return "bridge_axe"; }

    void update(float dt) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    bool isSwung() const { return m_swung; }

private:
    // One chop. Contact persists for several frames and the handler tears down
    // a bridge; running it twice would publish a second collapse into a level
    // that no longer has one.
    bool m_swung = false;
};
