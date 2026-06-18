#include "Entities/Entity.hpp"

AABB Entity::getBoundingBox() const {
    return AABB{position.x, position.y, boundingBox.width, boundingBox.height};
}

bool Entity::isActive() const {
    return active;
}

void Entity::destroy() {
    active = false;
}
