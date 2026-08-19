#pragma once

#include <string>

#include "Entities/Item.hpp"

class FireFlower : public Item {
public:
    explicit FireFlower(sf::Vector2f pos);
    ~FireFlower() override = default;

    std::string getTypeName() const override { return "fire_flower"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
