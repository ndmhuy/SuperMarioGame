#pragma once

#include "Entities/Item.hpp"

class MiniMushroom : public Item {
public:
    MiniMushroom() = default;
    ~MiniMushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
