#pragma once

#include "Entities/Item.hpp"

class MegaMushroom : public Item {
public:
    MegaMushroom() = default;
    ~MegaMushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
