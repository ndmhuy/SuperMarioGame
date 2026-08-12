#pragma once

#include "Entities/Enemy.hpp"

class ChainChomp : public Enemy {
public:
    explicit ChainChomp(sf::Vector2f position);
    ~ChainChomp() override = default;

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;
};
