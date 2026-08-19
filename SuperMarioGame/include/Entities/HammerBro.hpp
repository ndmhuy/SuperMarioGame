#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class HammerBro : public Enemy {
public:
    explicit HammerBro(sf::Vector2f position);
    ~HammerBro() override = default;

    std::string getTypeName() const override { return "hammer_bro"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;
};
