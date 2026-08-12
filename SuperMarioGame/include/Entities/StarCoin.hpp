#pragma once

#include "Entities/Item.hpp"

class StarCoin : public Item {
public:
    explicit StarCoin(sf::Vector2f pos);
    ~StarCoin() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
