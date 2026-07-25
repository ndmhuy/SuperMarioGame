#pragma once

#include "Entities/Block.hpp"

class BrickBlock : public Block {
public:
    explicit BrickBlock(sf::Vector2f position, int coins = 0);
    ~BrickBlock() override = default;

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;

    int getCoinsLeft() const { return m_coinsLeft; }
    bool isEmpty() const { return m_isEmpty; }

private:
    int m_coinsLeft = 0;
    bool m_isEmpty = false;
};
