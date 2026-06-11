#include "Physics/AABB.hpp"

bool AABB::intersects(const AABB& other) const {
    // TODO: Implement by hand
    return false;
}

AABB AABB::getOverlap(const AABB& other) const {
    // TODO: Implement by hand
    return AABB{};
}

bool AABB::contains(float px, float py) const {
    // TODO: Implement by hand
    return false;
}

sf::Vector2f AABB::getCenter() const {
    // TODO: Implement by hand
    return sf::Vector2f{0.0f, 0.0f};
}
