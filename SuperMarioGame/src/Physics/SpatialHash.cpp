#include "Physics/SpatialHash.hpp"
#include "Entities/Entity.hpp"
#include <unordered_set>
#include <cmath>

void SpatialHash::insert(Entity* entity, const AABB& box) {
    int startX = static_cast<int>(std::floor(box.x / CELL_SIZE));
    int endX = static_cast<int>(std::floor((box.x + box.width) / CELL_SIZE));
    int startY = static_cast<int>(std::floor(box.y / CELL_SIZE));
    int endY = static_cast<int>(std::floor((box.y + box.height) / CELL_SIZE));

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            m_grid[{x, y}].push_back(entity);
        }
    }
}

void SpatialHash::clear() {
    m_grid.clear();
}

std::vector<Entity*> SpatialHash::query(const AABB& box) const {
    std::unordered_set<Entity*> candidates;

    int startX = static_cast<int>(std::floor(box.x / CELL_SIZE));
    int endX = static_cast<int>(std::floor((box.x + box.width) / CELL_SIZE));
    int startY = static_cast<int>(std::floor(box.y / CELL_SIZE));
    int endY = static_cast<int>(std::floor((box.y + box.height) / CELL_SIZE));

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            auto it = m_grid.find({x, y});
            if (it != m_grid.end()) {
                for (Entity* entity : it->second) {
                    candidates.insert(entity);
                }
            }
        }
    }

    return std::vector<Entity*>(candidates.begin(), candidates.end());
}
