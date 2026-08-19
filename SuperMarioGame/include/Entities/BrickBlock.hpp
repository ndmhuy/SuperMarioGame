#pragma once

#include <string>

#include "Entities/Block.hpp"

class BrickBlock : public Block {
public:
    explicit BrickBlock(sf::Vector2f position, int coins = 0);
    ~BrickBlock() override = default;

    std::string getTypeName() const override { return "brick_block"; }

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    int getCoinsLeft() const { return m_coinsLeft; }
    bool isEmpty() const { return m_isEmpty; }

private:
    int m_coinsLeft = 0;
    bool m_isEmpty = false;
};
