#pragma once

#include <algorithm>

#include "Core/Game.hpp"
#include "Physics/AABB.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"

// "Is that solid?", asked by everything that has to look before it moves.
//
// The only terrain query outside the physics engine used to be inlined in
// PatrolStrategy::calculateTarget, so nothing else could ask: the Hammer Bro
// shuffle walked straight off its platform and MovingPlatform drove into walls
// with no tilemap query at all (R21 defects 5 and 10). There is now exactly one
// definition of solidity to keep correct, and callers reach it through
// Enemy::hasFloorAhead() or directly.
//
// Coordinates are world pixels, not tile indices. A world with no tilemap
// installed — the menu states, and every headless harness — answers "no
// terrain" rather than crashing, which is deliberately the permissive answer:
// absence of a tilemap is absence of evidence, not evidence of a wall.
namespace TerrainProbe {

inline bool isSolidAt(float x, float y) {
    const TileMap* map = Game::getInstance().getTileMap();
    if (!map) return false;
    // Only solid ground counts. Water, lava and a coin tile are all "not Empty"
    // but none of them holds anything up, so a != Empty test walked patrols
    // straight into lava.
    return TileMap::getInfo(map->getTileAt(x, y)).isSolid;
}

// Does `box` overlap any solid tile?
//
// The edges are inset by a pixel on purpose: a body that has come to rest flush
// against a wall shares that wall's boundary coordinate, and without the inset
// it would report itself as already inside the wall and refuse to move again.
inline bool overlapsSolid(const AABB& box) {
    if (box.width <= 0.0f || box.height <= 0.0f) return false;
    if (!Game::getInstance().getTileMap()) return false;

    constexpr float INSET = 1.0f;
    const float left   = box.x + INSET;
    const float top    = box.y + INSET;
    const float right  = std::max(left, box.x + box.width  - INSET);
    const float bottom = std::max(top,  box.y + box.height - INSET);

    // Step by a tile, but always finish on the far edge, so a box narrower or
    // taller-than-a-multiple-of-a-tile cannot straddle a tile it never samples.
    for (float y = top;;) {
        for (float x = left;;) {
            if (isSolidAt(x, y)) return true;
            if (x >= right) break;
            x = std::min(x + Constants::TILE_SIZE, right);
        }
        if (y >= bottom) break;
        y = std::min(y + Constants::TILE_SIZE, bottom);
    }
    return false;
}

}  // namespace TerrainProbe
