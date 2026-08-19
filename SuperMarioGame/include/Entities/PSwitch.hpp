#pragma once

#include <string>

#include "Entities/Item.hpp"

class PSwitch : public Item {
public:
    explicit PSwitch(sf::Vector2f pos);
    ~PSwitch() override = default;

    std::string getTypeName() const override { return "pswitch"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void collect() override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    bool m_pressed = false;
    Animation m_pressedAnimation;
};
