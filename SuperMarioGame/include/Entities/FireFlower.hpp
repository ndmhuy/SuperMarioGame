#pragma once

#include "Entities/Item.hpp"

class FireFlower : public Item {
public:
    explicit FireFlower(sf::Vector2f pos);
    ~FireFlower() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
