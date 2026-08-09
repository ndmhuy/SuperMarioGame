#pragma once

#include "Entities/Enemy.hpp"

class Boo : public Enemy {
public:
    explicit Boo(sf::Vector2f position);
    virtual ~Boo() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void onStomped() override;
    void onHitByFireball() override;
};
