#pragma once

#include "Entities/Entity.hpp"

class PhysicsEngine;
class CollisionResolver;

class Character : public Entity {
public:
    Character() = default;
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

protected:
    // Friends for controlled physics write access
    friend class PhysicsEngine;
    friend class CollisionResolver;

    int health = 1;
    float speed = 0.0f;
    float jumpForce = 0.0f;
    bool onGround = false;
    bool onWall = false;
    bool facingRight = true;
};
