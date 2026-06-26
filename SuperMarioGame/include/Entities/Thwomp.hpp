#pragma once

#include "Entities/Enemy.hpp"

class Thwomp : public Enemy {
public:
    explicit Thwomp(sf::Vector2f position);
    ~Thwomp() override = default;

    void onStomped() override;
    void onHitByFireball() override;
};
