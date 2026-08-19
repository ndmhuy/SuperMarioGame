#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class PiranhaPlant : public Enemy {
public:
    explicit PiranhaPlant(sf::Vector2f position);
    ~PiranhaPlant() override = default;

    std::string getTypeName() const override { return "piranha_plant"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }
};
