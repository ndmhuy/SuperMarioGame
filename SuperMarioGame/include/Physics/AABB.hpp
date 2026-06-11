#pragma once

#include <SFML/System/Vector2.hpp>

struct AABB {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    // Checks if this AABB intersects another
    bool intersects(const AABB& other) const;

    // Returns the overlap AABB between this and another
    AABB getOverlap(const AABB& other) const;

    // Checks if a point lies inside this AABB
    bool contains(float px, float py) const;

    // Returns the center of the bounding box
    sf::Vector2f getCenter() const;
};
