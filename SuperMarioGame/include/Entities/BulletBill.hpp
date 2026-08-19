#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class BulletBill : public Enemy {
public:
    explicit BulletBill(sf::Vector2f position, float dirX = -1.0f);
    ~BulletBill() override = default;

    // Without this the base Entity::getTypeName() answered "unknown", which
    // LevelLoader wrote into saved levels and parseEntityTypeName then read
    // back as a Goomba — silent data loss in the map editor.
    std::string getTypeName() const override { return "bullet_bill"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }
};
