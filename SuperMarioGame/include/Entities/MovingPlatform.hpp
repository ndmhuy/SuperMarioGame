#pragma once

#include <string>

#include "Entities/Block.hpp"

class MovingPlatform : public Block {
public:
    MovingPlatform(sf::Vector2f position, sf::Vector2f travelRange, float speed = 50.0f);
    ~MovingPlatform() override = default;

    std::string getTypeName() const override { return "moving_platform"; }

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Kinematic: this platform owns its own position along a parametric path,
    // so the physics engine must neither integrate it nor tile-resolve it.
    //
    // It did both. PhysicsEngine pushed the platform out of a wall it had been
    // driven into, and update() then overwrote that push-out with setPosition()
    // on the same frame — re-teleporting it back inside, every frame, forever.
    // The same overlap also made the player carry below dead: entities update
    // before physics, so `newPos - oldPos` measured the push-out rather than the
    // platform's own step and read as zero in steady state. Carrying worked only
    // in verify_blocks_new.cpp, which runs the platform with no engine at all
    // (R21 D5).
    bool isPhysicsDriven() const override { return false; }

private:
    // The platform's footprint if it stood at `pos`, for probing a destination
    // before committing to it.
    AABB footprintAt(sf::Vector2f pos) const;

    sf::Vector2f m_startPos;
    sf::Vector2f m_travelRange;
    const float m_rangeLen;
    float m_speed;
    float m_progress = 0.0f;
    bool m_movingForward = true;

    // The sweep actually available, in progress units. Starts as the whole
    // configured range and is shortened inwards the first time terrain is found
    // at an end. Reversing alone is not enough: the same destination would be
    // probed again the very next frame and the platform would shudder against
    // the wall instead of patrolling what is left.
    float m_minProgress = 0.0f;
    float m_maxProgress = 1.0f;

    // A platform whose *start* is already inside terrain has nowhere legal to
    // go in either direction; flipping it would only jitter it in place, so it
    // holds still. Probed on the first update rather than in the constructor,
    // because the level installs its tilemap after building its entities.
    bool m_startBlocked = false;
    bool m_terrainProbed = false;
};
