#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class Thwomp : public Enemy {
public:
    explicit Thwomp(sf::Vector2f position);
    ~Thwomp() override = default;

    std::string getTypeName() const override { return "thwomp"; }

    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    // Stone: onStomped() damages the player.
    bool isStompSafe() const override { return false; }
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }

private:
    Animation m_dormantAnim;
    Animation m_activeAnim;
};
