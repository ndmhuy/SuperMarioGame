#pragma once

#include "Entities/Block.hpp"

class IceBlock : public Block {
public:
    explicit IceBlock(sf::Vector2f position);
    ~IceBlock() override = default;

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    float getFriction() const { return m_friction; }

private:
    float m_friction = 0.1f; // Standard friction is 1.0f, ice is 0.1f (low friction)
};
