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
    bool isOnWall() const { return onWall; }
    bool isFacingRight() const { return facingRight; }

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
    friend int main();

    int health = 1;
    float speed = 0.0f;
    float jumpForce = 0.0f;
    bool onGround = false;
    bool onWall = false;
    bool facingRight = true;

    bool m_moveLeftRequested = false;
    bool m_moveRightRequested = false;
};
