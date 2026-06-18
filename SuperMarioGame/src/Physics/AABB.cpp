#include "Physics/AABB.hpp"
#include <algorithm>
#include <cmath>

bool AABB::intersects(const AABB& other) const {
    return this->x < other.x + other.width && this->x + this->width > other.x &&
           this->y < other.y + other.height && this->y + this->height > other.y;
}

AABB AABB::getOverlap(const AABB& other) const {
    float overlapX = std::min(this->x + this->width, other.x + other.width) -
                     std::max(this->x, other.x);
    float overlapY = std::min(this->y + this->height, other.y + other.height) -
                     std::max(this->y, other.y);

    if (overlapX > 0.0f && overlapY > 0.0f) {
        return AABB{std::max(this->x, other.x), std::max(this->y, other.y),
                    overlapX, overlapY};
    }

    return AABB{0.0f, 0.0f, 0.0f, 0.0f};
}

bool AABB::contains(float px, float py) const {
    return px >= this->x && px <= this->x + this->width &&
           py >= this->y && py <= this->y + this->height;
}

sf::Vector2f AABB::getCenter() const {
    return sf::Vector2f{this->x + this->width / 2.0f, this->y + this->height / 2.0f};
}
