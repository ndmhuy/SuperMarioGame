#pragma once

#include <memory>
#include <vector>

class TileMap;
class Entity;

// A post-generation reachability check for MapGenerator.
//
// Where this comes from
// ----------------------
// An abandoned side branch (A/mapgen-gan-plan, never merged — see AGENTS.md /
// the project's standing rule against merging the GAN+RL work) built a real
// solvability oracle: simulate the player's actual jump physics frame-by-frame
// and run a bottleneck-cost search over the tile grid to certify a generated
// level is completable, rather than trust the placement heuristics that built
// it. That result was worth keeping even though the GAN/imitation-learning
// pipeline around it was not: MapGenerator's existing guardrails (capping pit
// width, dropping a platform across anything 3+ tiles wide) are placement-time
// heuristics, not a check that the finished level is actually walkable.
//
// This is a deliberately simplified, dependency-free reimplementation of that
// idea — a column-reachability BFS using this game's own jump/run constants as
// a bound, not a per-frame ballistic simulation — because the bottleneck-
// Dijkstra difficulty-grading half of the original oracle is a different
// problem (how hard is the hardest required move) from the one this project
// actually needs solved (is there a required move that is impossible at all).
namespace LevelSolvability {

// True if a path made only of walking and jumping (bound by this game's own
// WALK/RUN speed and JUMP_HEIGHT) connects every tile-column from
// `startTileX` to `endTileX` across `map`'s solid ground, allowing a
// MovingPlatform/FallingPlatform entity in `entities` to stand in for the
// ground at its own column — exactly the escape hatch MapGenerator's own pit
// guardrail already relies on, so a platform-bridged pit is not a false
// rejection.
bool isPathReachable(const TileMap& map,
                      const std::vector<std::unique_ptr<Entity>>& entities,
                      int startTileX, int endTileX);

} // namespace LevelSolvability
