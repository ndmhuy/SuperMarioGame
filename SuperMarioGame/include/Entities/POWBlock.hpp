#pragma once

#include "Entities/Item.hpp"

class POWBlock : public Item {
public:
    POWBlock() = default;
    ~POWBlock() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
