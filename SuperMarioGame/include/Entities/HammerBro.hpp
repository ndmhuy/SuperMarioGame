#pragma once

#include "Entities/Enemy.hpp"

class HammerBro : public Enemy {
public:
    explicit HammerBro(sf::Vector2f position);
    ~HammerBro() override = default;

    void onStomped() override;
    void onHitByFireball() override;
};
