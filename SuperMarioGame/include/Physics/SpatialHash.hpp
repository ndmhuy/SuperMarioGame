#pragma once

#include <vector>
#include <unordered_map>
#include "Physics/AABB.hpp"

class Entity;

class SpatialHash {
public:
    SpatialHash() = default;
    ~SpatialHash() = default;

    // Grid Insertion & Clearing
    void insert(Entity* entity, const AABB& box);
    void clear();

    // Query elements overlapping a box
    std::vector<Entity*> query(const AABB& box) const;

private:
    static constexpr float CELL_SIZE = 64.0f;

    // Custom hash for 2D grid coordinates
    struct CellKey {
        int x, y;
        bool operator==(const CellKey& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct KeyHash {
        size_t operator()(const CellKey& key) const {
            return (std::hash<int>()(key.x) ^ (std::hash<int>()(key.y) << 1));
        }
    };

    std::unordered_map<CellKey, std::vector<Entity*>, KeyHash> m_grid;
};
