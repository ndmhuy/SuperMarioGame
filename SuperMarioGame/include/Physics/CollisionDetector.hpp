#pragma once

#include <vector>
#include "Physics/AABB.hpp"

class Entity;
class TileMap;
class SpatialHash;

struct CollisionInfo {
    bool collided = false;
    sf::Vector2f overlap{0.0f, 0.0f};
    sf::Vector2f normal{0.0f, 0.0f};
    Entity* other = nullptr;
};

class CollisionDetector {
public:
    CollisionDetector() = default;
    ~CollisionDetector() = default;

    // Direct collision checks
    CollisionInfo checkEntityVsEntity(Entity& e1, Entity& e2);
    std::vector<CollisionInfo> checkEntityVsTileMap(Entity& entity, TileMap& tileMap);
};
