#include "Entities/Entity.hpp"

const AABB& Entity::getBoundingBox() const {
    return boundingBox;
}

bool Entity::isActive() const {
    return active;
}

void Entity::destroy() {
    active = false;
}

sf::Vector2f Entity::getPosition() const {
    return position;
}

sf::Vector2f Entity::getVelocity() const {
    return velocity;
}
