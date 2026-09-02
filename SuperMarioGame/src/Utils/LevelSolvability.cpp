#include "Utils/LevelSolvability.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Entities/Entity.hpp"

#include <cmath>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace {

// Derived from this game's own physics constants (Constants.hpp), not
// authored separately — so a future tuning of jump height or run speed keeps
// this check honest instead of silently drifting from what the player can
// actually do. See AGENTS.md's physics table for where these numbers come
// from: JUMP_HEIGHT ~4 tiles, gravity 0.5 px/frame^2 at 60 fps.
int maxJumpTilesHorizontal() {
    // v0 = sqrt(2 * g * h); airtime = 2 * v0 / g; distance = RUN_SPEED * airtime.
    const float g = Constants::GRAVITY;                         // px/frame^2
    const float h = Constants::JUMP_HEIGHT;                     // px
    const float v0 = std::sqrt(2.0f * g * h);                   // px/frame
    const float airtimeFrames = 2.0f * v0 / g;
    const float airtimeSeconds = airtimeFrames / 60.0f;
    const float distancePx = Constants::RUN_SPEED * airtimeSeconds;
    return static_cast<int>(distancePx / Constants::TILE_SIZE);
}

int maxJumpTilesVertical() {
    return static_cast<int>(Constants::JUMP_HEIGHT / Constants::TILE_SIZE);
}

// The first tile the player could stand ON at column `x`, scanning down, or -1
// if the column is a bottomless pit (no floor at all within the map).
//
// The run of solid tiles hanging from row 0 is skipped, because it is a
// CEILING and nobody stands on the underside of one. This check used to return
// the first solid tile full stop, and MapGenerator gives every Castle and
// Underground map a two-row solid ceiling across its whole width — so every
// column answered "row 0", every rise was zero, and isPathReachable() returned
// true unconditionally. The solvability guarantee was vacuous for exactly the
// two themes with the least forgiving geometry, the boss castle among them.
int groundRowAt(const TileMap& map, int x) {
    int y = 0;
    while (y < map.getHeight() && TileMap::getInfo(map.getTileType(x, y)).isSolid) ++y;
    for (; y < map.getHeight(); ++y) {
        if (TileMap::getInfo(map.getTileType(x, y)).isSolid) return y;
    }
    return -1;
}

} // namespace

namespace LevelSolvability {

bool isPathReachable(const TileMap& map,
                      const std::vector<std::unique_ptr<Entity>>& entities,
                      int startTileX, int endTileX) {
    if (startTileX >= endTileX) return true;

    const int maxDx = std::max(1, maxJumpTilesHorizontal());
    const int maxRise = std::max(1, maxJumpTilesVertical());

    // Columns a MovingPlatform/FallingPlatform stands ready to bridge — the
    // same escape hatch MapGenerator's own pit guardrail already relies on
    // (see MapGenerator.cpp's platform placement for wide pits), so a pit a
    // platform actually covers is not treated as a break in the path.
    std::unordered_map<int, int> platformRowAt;   // tileX -> tileY
    for (const auto& entity : entities) {
        if (!entity) continue;
        const std::string type = entity->getTypeName();
        if (type != "moving_platform" && type != "falling_platform") continue;
        const int tx = static_cast<int>(entity->getPosition().x / Constants::TILE_SIZE);
        const int ty = static_cast<int>(entity->getPosition().y / Constants::TILE_SIZE);
        platformRowAt[tx] = ty;
    }

    auto rowAt = [&](int x) -> int {
        if (x < 0 || x >= map.getWidth()) return -1;
        const int ground = groundRowAt(map, x);
        if (ground >= 0) return ground;
        // No solid floor here — but a platform a couple of tiles either side
        // of its recorded x still counts, since it is a few tiles wide.
        for (const auto& [px, py] : platformRowAt) {
            if (std::abs(px - x) <= 2) return py;
        }
        return -1;
    };

    // BFS over columns: from a standable column, every column within
    // maxDx tiles is reachable if climbing UP to it costs at most maxRise
    // tiles (any descent is free — falling is not a movement constraint).
    std::deque<int> queue;
    std::unordered_set<int> visited;
    queue.push_back(startTileX);
    visited.insert(startTileX);

    while (!queue.empty()) {
        const int x = queue.front();
        queue.pop_front();
        if (x >= endTileX) return true;

        const int fromRow = rowAt(x);
        if (fromRow < 0) continue;   // standing nowhere, cannot jump from here

        for (int dx = 1; dx <= maxDx; ++dx) {
            const int nx = x + dx;
            if (nx > endTileX + maxDx || visited.count(nx)) continue;
            const int toRow = rowAt(nx);
            if (toRow < 0) continue;
            const int rise = fromRow - toRow;   // positive = climbing up
            if (rise > maxRise) continue;
            visited.insert(nx);
            queue.push_back(nx);
        }
    }
    return false;
}

} // namespace LevelSolvability
