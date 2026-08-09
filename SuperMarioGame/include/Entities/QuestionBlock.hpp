#pragma once

#include "Entities/Block.hpp"

class QuestionBlock : public Block {
public:
    // itemType: 0 = Coin, 1 = Mushroom, 2 = FireFlower, 3 = Star, 4 = OneUp
    explicit QuestionBlock(sf::Vector2f position, int itemType = 0);
    ~QuestionBlock() override = default;

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    int getItemType() const { return m_containedItemType; }
    bool isEmpty() const { return m_isEmpty; }

private:
    int m_containedItemType = 0;
    bool m_isEmpty = false;
};
