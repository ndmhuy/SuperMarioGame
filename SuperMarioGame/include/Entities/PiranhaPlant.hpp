#pragma once

#include "Entities/Enemy.hpp"

class PiranhaPlant : public Enemy {
public:
    explicit PiranhaPlant(sf::Vector2f position);
    ~PiranhaPlant() override = default;

    void onStomped() override;
    void onHitByFireball() override;
};
