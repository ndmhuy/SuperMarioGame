#pragma once

#include <string>

#include "Entities/Item.hpp"

class Coin : public Item {
public:
    explicit Coin(sf::Vector2f pos);
    ~Coin() override = default;

    std::string getTypeName() const override { return "coin"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
