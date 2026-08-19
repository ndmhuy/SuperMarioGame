#pragma once

#include <string>

#include "Entities/Item.hpp"

class Star : public Item {
public:
    explicit Star(sf::Vector2f pos);
    ~Star() override = default;

    std::string getTypeName() const override { return "star"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_movingRight = true;
};
