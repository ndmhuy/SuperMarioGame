#pragma once

#include <array>
#include <cstddef>
#include <vector>

// The seam between an AI opponent's senses and its decisions.
//
// AIController does the sensing and the actuating: it reads the tilemap and the
// entity list into an AIObservation, and it turns an AIAction back into calls on
// a Player. What it does *not* do is decide. That lives behind this interface,
// so the shipped heuristic and a future trained network are interchangeable
// without the game loop knowing which one is installed.
//
// Four properties are load-bearing for the reinforcement-learning side project
// (docs/two_player_ai_plan.md §6), and all four are cheap now and expensive to
// retrofit:
//
//  1. AIObservation is FIXED-SIZE and normalized. Difficulty changes how much of
//     the grid is filled in, never how big it is — an easy opponent sees
//     Unknown outside its radius rather than a smaller grid. A network trained
//     against one difficulty therefore accepts every other.
//  2. AIAction mirrors PlayerFramePacket's buttons, so a recorded human session
//     is directly usable as imitation-learning data.
//  3. The exploration noise lives in AIController, not in here, so it applies
//     identically to both implementations — and is exactly epsilon-greedy.
//  4. decide() is synchronous and allocation-free on the hot path.

// What the agent believes occupies one cell of its vision grid.
enum class AICellState {
    Unknown,   // outside this difficulty's vision radius — not the same as empty
    Empty,
    Solid,     // ground, brick, pipe: stands on, blocks, can be jumped onto
    Hazard,    // lava, and anything that damages on contact
    Reward,    // coins, items, question blocks
    Enemy
};

// The vision grid is always this size — a full screen's worth of tiles, which is
// the Hard difficulty's radius. See property 1 above.
inline constexpr int kAIVisionWidth = 21;
inline constexpr int kAIVisionHeight = 15;
inline constexpr int kAIVisionCells = kAIVisionWidth * kAIVisionHeight;
// One-hot width per cell: the six AICellState values. Only the neural side needs
// this, but it is stated here because it is a property of the observation, not of
// whatever consumes it.
inline constexpr int kAICellStateCount = 6;

struct AIObservation {
    // Row-major, centred on the agent's own tile. grid[y * W + x], with the
    // agent at (kAIVisionWidth / 2, kAIVisionHeight / 2).
    std::array<AICellState, kAIVisionCells> grid{};

    // Offset to the current objective, in tiles, clamped to [-1, 1] by dividing
    // through by the vision half-extent.
    float dxToGoal = 0.0f;
    float dyToGoal = 0.0f;
    // Offset to the human player. Zero when there is no opponent to speak of.
    float dxToOpponent = 0.0f;
    float dyToOpponent = 0.0f;
    // Own velocity, normalized against the run speed.
    float vx = 0.0f;
    float vy = 0.0f;

    bool onGround = false;
    bool canJump = false;      // grounded, or inside the coyote-time window
    bool isPoweredUp = false;  // Super or better: can take a hit, may be able to shoot

    // Read one cell in agent-relative tile coordinates: (0, 0) is the agent's
    // own tile, +x is right, +y is down. Out-of-grid reads answer Unknown, so a
    // caller may probe freely without bounds-checking first.
    AICellState at(int dx, int dy) const {
        const int x = dx + kAIVisionWidth / 2;
        const int y = dy + kAIVisionHeight / 2;
        if (x < 0 || x >= kAIVisionWidth || y < 0 || y >= kAIVisionHeight) {
            return AICellState::Unknown;
        }
        return grid[static_cast<std::size_t>(y) * kAIVisionWidth + x];
    }

    // NOTE: the flattening to a [-1, 1] float vector that a neural policy needs
    // is deliberately NOT here. It has exactly one caller, and that caller lives
    // on the reinforcement-learning branch, so defining it on this branch would
    // ship a function nothing calls — the failure mode audit item B-9 records
    // five times over. The branch adds `toFeatureVector()` alongside the policy
    // that consumes it. Everything it needs to be well-defined — a fixed cell
    // count, a fixed state count, normalized scalars — is guaranteed above.
};

// The agent's discrete action space. Deliberately the same buttons a human has,
// and the same set PlayerFramePacket records (property 2 above).
struct AIAction {
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool run = false;
    bool crouch = false;
    bool shoot = false;
    bool groundPound = false;
};

class IAIPolicy {
public:
    virtual ~IAIPolicy() = default;

    // One decision. Called at the difficulty's reaction cadence rather than
    // every frame, so it must be a pure function of the observation plus
    // whatever state the policy chooses to keep — never of the frame count.
    virtual AIAction decide(const AIObservation& observation) = 0;

    // Shown in the HUD and the dev overlay.
    virtual const char* name() const = 0;

    // Forget any per-episode state. Called when a level loads or the agent
    // respawns, so a policy that keeps momentum does not carry it across a life.
    virtual void reset() {}
};
