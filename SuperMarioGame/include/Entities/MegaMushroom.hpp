#pragma once

#include "Entities/Item.hpp"

class MegaMushroom : public Item {
public:
    explicit MegaMushroom(sf::Vector2f pos);
    ~MegaMushroom() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_movingRight = true;
};
