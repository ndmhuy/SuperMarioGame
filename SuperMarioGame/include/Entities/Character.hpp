#pragma once

#include "Entities/Entity.hpp"

class Character : public Entity {
public:
    Character() = default;
    ~Character() override = default;

    // Direct movement commands called by Input/AI
    virtual void moveLeft();
    virtual void moveRight();
    virtual void jump();
    virtual void takeDamage(int amount);

    // Common character fields
    int health = 1;
    float speed = 0.0f;
    float jumpForce = 0.0f;
    bool onGround = false;
    bool onWall = false;
    bool facingRight = true;
};
