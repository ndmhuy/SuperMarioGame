#pragma once

#include "Entities/Item.hpp"

class CapeFeather : public Item {
public:
    CapeFeather() = default;
    ~CapeFeather() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
};
