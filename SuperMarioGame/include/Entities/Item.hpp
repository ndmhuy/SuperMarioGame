#pragma once

#include "Entities/Entity.hpp"

class Player;

class Item : public Entity {
public:
    explicit Item(sf::Vector2f pos);
    ~Item() override = default;

    // Collect/Apply powerup callbacks
    virtual void activate(Player& player);
    virtual void collect();

    // Read-only getter
    bool isCollected() const { return collected; }

protected:
    bool collected = false;
};

