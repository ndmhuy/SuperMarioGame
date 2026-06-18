#pragma once

#include "Entities/Character.hpp"

class Enemy : public Character {
public:
    Enemy() = default;
    ~Enemy() override = default;

    virtual void onStomped() = 0;
    virtual void onHitByFireball() = 0;
};
