#pragma once

#include "Entities/Enemy.hpp"

class PiranhaPlant : public Enemy {
public:
    explicit PiranhaPlant(sf::Vector2f position);
    ~PiranhaPlant() override = default;

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;
};
