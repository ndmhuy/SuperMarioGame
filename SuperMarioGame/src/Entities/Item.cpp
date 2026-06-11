#include "Entities/Item.hpp"
#include "Entities/Player.hpp"

void Item::activate(Player& player) {
    // TODO: Implement by hand
}

void Item::collect() {
    // TODO: Implement by hand
    collected = true;
    destroy();
}
