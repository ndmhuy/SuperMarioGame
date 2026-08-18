#pragma once

#include "Entities/Enemy.hpp"

class BulletBill : public Enemy {
public:
    explicit BulletBill(sf::Vector2f position, float dirX = -1.0f);
    ~BulletBill() override = default;

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }
};
