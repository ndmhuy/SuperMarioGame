#pragma once

#include "Entities/Enemy.hpp"

class HammerBro : public Enemy {
public:
    explicit HammerBro(sf::Vector2f position);
    ~HammerBro() override = default;

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;
};
