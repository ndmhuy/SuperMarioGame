#pragma once

#include "Entities/Player.hpp"

class Luigi : public Player {
public:
    Luigi() = default;
    ~Luigi() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void doubleJump();

private:
    bool m_hasDoubleJumped = false;
};
