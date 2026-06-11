#include "Entities/Entity.hpp"

AABB Entity::getBoundingBox() const {
    // TODO: Implement by hand
    return boundingBox;
}

bool Entity::isActive() const {
    // TODO: Implement by hand
    return active;
}

void Entity::destroy() {
    // TODO: Implement by hand
    active = false;
}
