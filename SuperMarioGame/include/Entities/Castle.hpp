#pragma once

#include <string>

#include "Entities/Block.hpp"

// The castle at the end of a level.
//
// What it replaces
// ----------------
// The hand-authored levels ended at the flagpole with nothing behind it, and
// MapGenerator "built a castle" by stamping a 5x5 square of ordinary GROUND
// tiles with a one-tile hole punched in it — a solid brown block the player
// could stand on top of, which is not a building. Meanwhile the world atlas has
// shipped castle_end (112x176), castle_0, castle_small and castle_white since
// the beginning, and none of them had ever been drawn.
//
// This is a decoration, deliberately: it is not solid, has no gravity, and the
// player walks in front of it. Making the end-of-level castle collidable is how
// the generator's version ended up as a climbable brown box.
//
// The flag rises out of the battlements when the level is completed, the way it
// does in the series — that is the only state this entity has.
class Castle : public Block {
public:
    // `groundTop` is the world y of the surface the castle stands on, so the
    // door lines up with the floor the player is walking along rather than
    // floating or sinking. Levels place a castle by its base, not its corner.
    explicit Castle(sf::Vector2f position);
    ~Castle() override = default;

    std::string getTypeName() const override { return "castle"; }

    // Scenery: nothing bumps it, nothing stands on it, and it never falls.
    void onHitFromBelow(Player& player) override {}
    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Starts the flag climbing the pole. Idempotent, because LevelComplete can
    // be published more than once.
    void raiseFlag();
    bool isFlagRaised() const { return m_flagRaised; }

    // The castle's footprint, in tiles. Levels and the generator both need this
    // to leave room for it, and a magic 3.5 in two places is one place too many.
    static constexpr float WIDTH_TILES  = 3.5f;   // 112px
    static constexpr float HEIGHT_TILES = 5.5f;   // 176px

private:
    // How far up its short mast the flag has climbed, 0..1.
    float m_flagRise = 0.0f;
    bool m_flagRaised = false;
    // Block keeps an Animator but not the sheet behind it, and the flag is a
    // second frame drawn outside the animation, so the castle holds its own
    // borrowed pointer. Owned by PlayingState and outlives every entity.
    const SpriteSheet* m_sheet = nullptr;
};
