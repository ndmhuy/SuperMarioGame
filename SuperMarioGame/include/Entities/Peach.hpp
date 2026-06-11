#pragma once

#include "Entities/Player.hpp"

class Peach : public Player {
public:
    Peach() = default;
    ~Peach() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void floatHover();

private:
    float m_hoverTimer = 0.0f;
    bool m_isHovering = false;
};
