#pragma once

#include "Entities/Item.hpp"

class CapeFeather : public Item {
public:
    explicit CapeFeather(sf::Vector2f pos);
    ~CapeFeather() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
