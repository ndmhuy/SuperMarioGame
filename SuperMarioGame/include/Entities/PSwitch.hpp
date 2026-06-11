#pragma once

#include "Entities/Item.hpp"

class PSwitch : public Item {
public:
    PSwitch() = default;
    ~PSwitch() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
