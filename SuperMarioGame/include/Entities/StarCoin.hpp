#pragma once

#include <string>

#include "Entities/Item.hpp"

class StarCoin : public Item {
public:
    explicit StarCoin(sf::Vector2f pos);
    ~StarCoin() override = default;

    std::string getTypeName() const override { return "star_coin"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
};
