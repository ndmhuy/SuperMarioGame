#pragma once

#include "Entities/Enemy.hpp"

class Thwomp : public Enemy {
public:
    explicit Thwomp(sf::Vector2f position);
    ~Thwomp() override = default;

    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }

private:
    Animation m_dormantAnim;
    Animation m_activeAnim;
};
