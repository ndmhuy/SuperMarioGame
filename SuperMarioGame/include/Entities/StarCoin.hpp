#pragma once

#include "Entities/Item.hpp"

class StarCoin : public Item {
public:
    StarCoin() = default;
    ~StarCoin() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
