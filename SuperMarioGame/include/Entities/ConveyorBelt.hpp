#pragma once

#include "Entities/Block.hpp"

class ConveyorBelt : public Block {
public:
    explicit ConveyorBelt(sf::Vector2f position, bool pushRight = true, float pushSpeed = 100.0f);
    ~ConveyorBelt() override = default;

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    bool isPushingRight() const { return m_pushRight; }
    float getPushSpeed() const { return m_pushSpeed; }

private:
    bool m_pushRight = true;
    float m_pushSpeed = 100.0f;
};
