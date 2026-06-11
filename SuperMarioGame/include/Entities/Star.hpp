#pragma once

#include "Entities/Item.hpp"

class Star : public Item {
public:
    Star() = default;
    ~Star() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
