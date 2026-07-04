#include "Entities/Item.hpp"
#include "Entities/Player.hpp"

Item::Item(sf::Vector2f pos) {
    position = pos;
    boundingBox = AABB{pos.x, pos.y, 32.0f, 32.0f};
    active = true;
    collected = false;
}

void Item::activate(Player& player) {
    // Overridden by subclasses
}

void Item::collect() {
    collected = true;
    destroy();
}
