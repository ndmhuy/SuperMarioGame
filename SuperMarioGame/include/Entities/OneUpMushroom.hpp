#pragma once

#include "Entities/Item.hpp"

class OneUpMushroom : public Item {
public:
    explicit OneUpMushroom(sf::Vector2f pos);
    ~OneUpMushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;

private:
    bool m_movingRight = true;
};
