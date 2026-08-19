#pragma once

#include <string>

#include "Entities/Item.hpp"

class OneUpMushroom : public Item {
public:
    explicit OneUpMushroom(sf::Vector2f pos);
    ~OneUpMushroom() override = default;

    std::string getTypeName() const override { return "oneup_mushroom"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_movingRight = true;
};
