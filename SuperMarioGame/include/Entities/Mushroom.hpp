#pragma once

#include "Entities/Item.hpp"

class Mushroom : public Item {
public:
    explicit Mushroom(sf::Vector2f pos);
    ~Mushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_movingRight = true;
};
