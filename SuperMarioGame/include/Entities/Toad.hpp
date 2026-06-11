#pragma once

#include "Entities/Player.hpp"

class Toad : public Player {
public:
    Toad() = default;
    ~Toad() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
};
