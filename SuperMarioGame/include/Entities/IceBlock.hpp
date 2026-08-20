#pragma once

#include <string>

#include "Entities/Block.hpp"

class IceBlock : public Block {
public:
    explicit IceBlock(sf::Vector2f position);
    ~IceBlock() override = default;

    // Without this the base Entity::getTypeName() answered "unknown", which
    // LevelLoader wrote into saved levels and parseEntityTypeName then read
    // back as a Goomba — silent data loss in the map editor.
    std::string getTypeName() const override { return "ice_block"; }

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    float getFriction() const { return m_friction; }

private:
    float m_friction = 0.1f; // Standard friction is 1.0f, ice is 0.1f (low friction)
};
