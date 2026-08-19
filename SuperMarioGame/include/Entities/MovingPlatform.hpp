#pragma once

#include <string>

#include "Entities/Block.hpp"

class MovingPlatform : public Block {
public:
    MovingPlatform(sf::Vector2f position, sf::Vector2f travelRange, float speed = 50.0f);
    ~MovingPlatform() override = default;

    std::string getTypeName() const override { return "moving_platform"; }

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

private:
    sf::Vector2f m_startPos;
    sf::Vector2f m_travelRange;
    const float m_rangeLen;
    float m_speed;
    float m_progress = 0.0f;
    bool m_movingForward = true;
};
