#pragma once

#include "Entities/Item.hpp"

class Coin : public Item {
public:
    explicit Coin(sf::Vector2f pos);
    ~Coin() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
