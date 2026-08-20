#pragma once

#include "Entities/Entity.hpp"

class PhysicsEngine;
class CollisionResolver;

class Character : public Entity {
public:
    explicit Character(sf::Vector2f pos = {0.0f, 0.0f}, sf::Vector2f targetSize = {32.0f, 32.0f}) : Entity(pos, targetSize) {}
    ~Character() override = default;

    // Direct movement commands called by Input/AI
    virtual void moveLeft();
    virtual void moveRight();
    virtual void jump();
    virtual void takeDamage(int amount);

    // Read-only getters for external consumers
    int getHealth() const { return health; }
    float getSpeed() const { return speed; }
    float getJumpForce() const { return jumpForce; }
    bool isOnGround() const { return onGround; }
    // Set the grounded flag outside the collision pass — used when teleporting or
    // respawning a character, and by the regression tests. Prefer this to reaching
    // in through friendship.
    void setGrounded(bool grounded) { onGround = grounded; }
    bool isOnWall() const { return onWall; }
    // Companion to setGrounded(): lets a harness stage a wall contact without
    // reaching through friendship into a protected member.
    void setOnWall(bool touching) { onWall = touching; }
    bool isFacingRight() const { return facingRight; }
    void setFacingRight(bool facing) { facingRight = facing; }

    // Intent request flags for physics loop
    bool isMoveLeftRequested() const { return m_moveLeftRequested; }
    bool isMoveRightRequested() const { return m_moveRightRequested; }
    virtual void clearMovementRequests();

protected:
    // Friends for controlled physics write access
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

    int health = 1;
    float speed = 0.0f;
    float jumpForce = 0.0f;
    bool onGround = false;
    // What onGround was at the *start* of this frame.
    //
    // PhysicsEngine clears onGround before collision detection and only sets it
    // again during the Y pass, so anything running in the X pass — which happens
    // first — reads false for a character that is standing on solid ground. Code
    // that needs to ask "am I airborne?" mid-frame must ask this instead.
    bool wasOnGround = false;
    bool onWall = false;
    bool facingRight = true;

    bool m_moveLeftRequested = false;
    bool m_moveRightRequested = false;
};
