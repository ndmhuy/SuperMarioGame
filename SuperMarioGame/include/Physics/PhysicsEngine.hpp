#pragma once

#include <vector>
#include "Physics/SpatialHash.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Physics/CollisionResolver.hpp"

class Entity;
class TileMap;

class PhysicsEngine {
public:
    PhysicsEngine() = default;
    ~PhysicsEngine() = default;

    // Standard physics updates
    void applyGravity(Entity& entity, float dt);
    void integrateVelocity(Entity& entity, float dt);

    // Main pipeline update
    void update(std::vector<Entity*>& entities, TileMap& tileMap, float dt);

private:
    SpatialHash m_spatialHash;
    CollisionDetector m_detector;
    CollisionResolver m_resolver;
};
