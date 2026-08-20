#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>

// Loads the certified-route sidecar written by tools/solvability.py.
//
// The sidecar (`<level>.waypoints.json`, next to the level file) holds the
// footholds of the kindest winnable route through the level, computed by the
// solvability oracle from the game's own physics constants — no agent, no
// search heuristic. AIController feeds these into the observation's goal
// channel, so dxToGoal/dyToGoal point at the next node of a path that is
// PROVEN to exist, instead of at the map's right edge.
//
// Missing sidecar, unreadable JSON, wrong version, or a level the oracle calls
// broken all return an empty vector — and an empty vector means AIController
// falls back to the right-edge goal, which is exactly the shipped behaviour.
// Guidance is an upgrade, never a dependency.
std::vector<sf::Vector2f> loadAIWaypoints(const std::string& levelPath);
