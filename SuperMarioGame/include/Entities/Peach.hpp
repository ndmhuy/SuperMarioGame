#pragma once

#include <string>

#include "Entities/Player.hpp"

class Peach : public Player {
public:
    explicit Peach(sf::Vector2f pos);
    ~Peach() override = default;

    std::string getTypeName() const override { return "peach"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void floatHover();
    float getGravityMultiplier() const override;
    float getRunSpeed() const override;
    std::string getCharacterName() const override;

private:
    float m_hoverTimer = 0.0f;
    bool m_isHovering = false;
};
