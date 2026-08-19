#pragma once

#include <string>

#include "Entities/Block.hpp"

class ConveyorBelt : public Block {
public:
    explicit ConveyorBelt(sf::Vector2f position, bool pushRight = true, float pushSpeed = 100.0f);
    ~ConveyorBelt() override = default;

    // Without this the base Entity::getTypeName() answered "unknown", which
    // LevelLoader wrote into saved levels and parseEntityTypeName then read
    // back as a Goomba — silent data loss in the map editor.
    std::string getTypeName() const override { return "conveyor_belt"; }

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
