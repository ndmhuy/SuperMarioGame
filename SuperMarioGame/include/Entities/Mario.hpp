#pragma once

#include "Entities/Player.hpp"

class Mario : public Player {
public:
    Mario() = default;
    ~Mario() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void shootFireball() override;
};
