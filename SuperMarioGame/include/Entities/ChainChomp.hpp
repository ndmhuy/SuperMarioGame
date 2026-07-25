#pragma once

#include "Entities/Enemy.hpp"

class ChainChomp : public Enemy {
public:
    explicit ChainChomp(sf::Vector2f position);
    ~ChainChomp() override = default;

    void onStomped() override;
    void onHitByFireball() override;
};
