#pragma once

#include "Entities/Player.hpp"

class Peach : public Player {
public:
    explicit Peach(sf::Vector2f pos);
    ~Peach() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void floatHover();
    float getGravityMultiplier() const override;

private:
    float m_hoverTimer = 0.0f;
    bool m_isHovering = false;
};
