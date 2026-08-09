#pragma once

#include "Entities/Item.hpp"

class MiniMushroom : public Item {
public:
    explicit MiniMushroom(sf::Vector2f pos);
    ~MiniMushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_movingRight = true;
};
