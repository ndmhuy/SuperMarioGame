#pragma once

#include "Entities/Item.hpp"

class Trampoline : public Item {
public:
    Trampoline() = default;
    ~Trampoline() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
