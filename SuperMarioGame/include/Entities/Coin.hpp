#pragma once

#include "Entities/Item.hpp"

class Coin : public Item {
public:
    Coin() = default;
    ~Coin() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
