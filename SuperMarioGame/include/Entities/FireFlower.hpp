#pragma once

#include "Entities/Item.hpp"

class FireFlower : public Item {
public:
    FireFlower() = default;
    ~FireFlower() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
