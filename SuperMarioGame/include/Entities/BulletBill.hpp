#pragma once

#include "Entities/Enemy.hpp"

class BulletBill : public Enemy {
public:
    explicit BulletBill(sf::Vector2f position, float dirX = -1.0f);
    ~BulletBill() override = default;

    void onStomped() override;
    void onHitByFireball() override;
};
