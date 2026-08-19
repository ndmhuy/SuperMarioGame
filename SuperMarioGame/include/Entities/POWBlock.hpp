#pragma once

#include <string>

#include "Entities/Item.hpp"

class POWBlock : public Item {
public:
    explicit POWBlock(sf::Vector2f pos);
    ~POWBlock() override = default;

    std::string getTypeName() const override { return "pow_block"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
