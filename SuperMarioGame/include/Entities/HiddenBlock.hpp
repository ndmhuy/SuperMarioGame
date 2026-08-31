#pragma once

#include <string>

#include "Entities/Block.hpp"

class HiddenBlock : public Block {
public:
    explicit HiddenBlock(sf::Vector2f position, int itemType = 0);
    ~HiddenBlock() override = default;

    // Without this the base Entity::getTypeName() answered "unknown", which
    // LevelLoader wrote into saved levels and parseEntityTypeName then read
    // back as a Goomba — silent data loss in the map editor.
    std::string getTypeName() const override { return "hidden_block"; }

    void onHitFromBelow(Player& player) override;
    BlockTouch onCharacterTouch(Character& character, const CollisionInfo& info) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    bool isCollidable() const override;

    bool isRevealed() const { return m_isRevealed; }

private:
    bool m_isRevealed = false;
    int m_containedItemType = 0; // 0 for Coin, other for items
};
