#pragma once

#include "Entities/Block.hpp"

class HiddenBlock : public Block {
public:
    explicit HiddenBlock(sf::Vector2f position, int itemType = 0);
    ~HiddenBlock() override = default;

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    bool isCollidable() const override;

    bool isRevealed() const { return m_isRevealed; }

private:
    bool m_isRevealed = false;
    int m_containedItemType = 0; // 0 for Coin, other for items
};
