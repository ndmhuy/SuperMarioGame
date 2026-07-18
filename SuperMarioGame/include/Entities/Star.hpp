#pragma once

#include "Entities/Item.hpp"

class Star : public Item {
public:
    explicit Star(sf::Vector2f pos);
    ~Star() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;

private:
    bool m_movingRight = true;
};
