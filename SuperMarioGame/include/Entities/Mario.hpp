#pragma once

#include "Entities/Player.hpp"

class Mario : public Player {
public:
    explicit Mario(sf::Vector2f pos);
    ~Mario() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void shootFireball() override;
};
