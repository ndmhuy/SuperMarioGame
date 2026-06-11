#pragma once

#include "Entities/Item.hpp"

class Mushroom : public Item {
public:
    Mushroom() = default;
    ~Mushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
