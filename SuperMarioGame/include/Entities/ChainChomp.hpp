#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class ChainChomp : public Enemy {
public:
    explicit ChainChomp(sf::Vector2f position);
    ~ChainChomp() override = default;

    // Without this the base Entity::getTypeName() answered "unknown", which
    // LevelLoader wrote into saved levels and parseEntityTypeName then read
    // back as a Goomba — silent data loss in the map editor.
    std::string getTypeName() const override { return "chain_chomp"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;
    bool onPlayerTouch(Player& player, const CollisionInfo& info, bool stomped) override;
};
