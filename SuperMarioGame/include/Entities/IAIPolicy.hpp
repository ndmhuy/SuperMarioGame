#pragma once

#include <SFML/System/Vector2.hpp>

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
    Hazard,    // lava, water, and enemy projectiles
    Coin,      // coins and coin tiles: worth a detour, never worth a death
    PowerUp,   // mushrooms, flowers, stars, question blocks: change what the
               // player IS, so they are worth more than a coin and sometimes
               // worth a risk
    // The enemy split is not cosmetic. Stomping a Goomba kills it; stomping a
    // Spiny calls player->takeDamage(1). Under the previous single Enemy state
    // those two cells were byte-identical in the observation, so no policy —
    // heuristic or learned, at any capacity — could tell "jump on this for
    // reward" from "jump on this and get hurt". That is an
    // information-theoretic limit, not a training problem.
    EnemyStompable,  // Goomba, Koopa, HammerBro, Lakitu, BulletBill, bosses
    EnemyDangerous,  // Spiny, PiranhaPlant, Thwomp, ChainChomp, Boo
    // A projectile that cannot hurt the player — its own fireball, or a
    // team-mate's. Previously every Projectile encoded as Hazard, so an agent
    // that shot a fireball then fled from its own shot.
    FriendlyProjectile,
    // v4 splits. Star and 1-Up leave the PowerUp bucket because they change
    // BEHAVIOUR, not just value: a star makes touching enemies safe for its
    // duration (every avoidance rule inverts), and an extra life changes what
    // a risk costs. A mushroom and a fire flower are both "collect if cheap",
    // so they stay lumped.
    ItemStar,
    ItemOneUp,
    // A trampoline: a launch DEVICE, not a pickup — the agent stands on it and
    // is thrown. Encoded as Solid it was an invisible catapult: bonus_1's
    // agent was launched over a pit by a surface it could not tell from
    // ground.
    Bouncer
};

// The vision grid is always this size — a full screen's worth of tiles, which is
// the Hard difficulty's radius. See property 1 above.
inline constexpr int kAIVisionWidth = 21;
inline constexpr int kAIVisionHeight = 15;
inline constexpr int kAIVisionCells = kAIVisionWidth * kAIVisionHeight;
// One-hot width per cell: the six AICellState values. Only the neural side needs
// this, but it is stated here because it is a property of the observation, not of
// whatever consumes it.
inline constexpr int kAICellStateCount = 12;

// v4: two motion features per cell — the occupying entity's velocity,
// normalized by run speed. Zero for tiles and still entities. This is what
// makes a MOVING platform distinguishable from a fixed one: without it the
// two are byte-identical in a single-frame observation and no memoryless
// policy can time a jump onto one (the same information-theoretic wall the
// Goomba/Spiny merge was). Enemy approach and projectile direction come free.
inline constexpr int kAICellMotionFeatures = 2;
inline constexpr int kAICellFeatures = kAICellStateCount + kAICellMotionFeatures;

// Scalar features appended after the grid, in this order: dxToGoal, dyToGoal,
// dxToOpponent, dyToOpponent, vx, vy, onGround, canJump, isPoweredUp,
// onWall, powerTier, invincibility.
inline constexpr int kAIScalarFeatures = 12;

// Bumped whenever the feature layout OR its semantics change. Weight files
// record the version they were trained against and are refused if it does not
// match: a silently mismatched input layer yields a policy that acts
// confidently and arbitrarily, which is the hardest possible thing to diagnose
// from the outside.
//
// v2: entity-form blocks (pipes, question blocks, platforms) are now painted
// into the grid as Solid/Reward across their full footprint.
// v3: the cell vocabulary grew from 6 states to 9 — Reward split into
// Coin/PowerUp, Enemy split into EnemyStompable/EnemyDangerous, and the
// agent's own fireballs separated from Hazard. This changes the FEATURE COUNT
// (315 cells x 9 = 2835 + 9 scalars = 2844), so a v2 weight file is not merely
// mis-scaled, it is the wrong shape.
// v4: three additions, each closing a measured perception gap.
//   - Motion planes: per-cell entity velocity (see kAICellMotionFeatures).
//   - Cell vocabulary 9 -> 12: ItemStar, ItemOneUp, Bouncer.
//   - Scalars 9 -> 12: onWall (wall-jump exists in the physics and was
//     invisible to every policy), powerTier (Small 0 / Super 0.5 / Fire+Cape
//     1 — isPoweredUp said "big", not "armed"), invincibility (a starred
//     agent could not know it was starred).
// Cell layout changes from [one-hot x9] to [one-hot x12, vx, vy] per cell,
// so featureCount moves 2844 -> 4422 and every v3 weight file is refused.
inline constexpr int kAIObservationVersion = 4;

struct AIObservation {
    // Row-major, centred on the agent's own tile. grid[y * W + x], with the
    // agent at (kAIVisionWidth / 2, kAIVisionHeight / 2).
    std::array<AICellState, kAIVisionCells> grid{};

    // Per-cell entity velocity, same indexing as `grid`, normalized by run
    // speed and clamped to [-1, 1]. Zero for tiles, still entities, and
    // Unknown/Empty cells.
    std::array<float, kAIVisionCells> velX{};
    std::array<float, kAIVisionCells> velY{};

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
    bool onWall = false;       // pressed against a wall — a wall-jump is available
    float powerTier = 0.0f;    // Small/Mini 0, Super 0.5, Fire/Cape 1
    float invincibility = 0.0f;  // star seconds remaining / 10, clamped to [0,1]

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

    // Velocity of whatever occupies a cell, agent-relative coordinates as at().
    // (0, 0) for out-of-grid reads and for anything that is not moving.
    sf::Vector2f velocityAt(int dx, int dy) const {
        const int x = dx + kAIVisionWidth / 2;
        const int y = dy + kAIVisionHeight / 2;
        if (x < 0 || x >= kAIVisionWidth || y < 0 || y >= kAIVisionHeight) {
            return {0.0f, 0.0f};
        }
        const std::size_t i = static_cast<std::size_t>(y) * kAIVisionWidth + x;
        return {velX[i], velY[i]};
    }

    // Flattened, normalized feature vector: the grid one-hot encoded cell by
    // cell in row-major order, then the nine scalars in the order they are
    // declared above. Length is always featureCount(), for every difficulty —
    // an Easy agent's unseen cells encode as the Unknown one-hot rather than
    // shortening the vector.
    //
    // This is the network's input layer, so the layout is a contract: changing
    // the cell order, the one-hot order or the scalar order invalidates every
    // trained weight file. If it must change, bump kAIObservationVersion.
    std::vector<float> toFeatureVector() const;

    static constexpr std::size_t featureCount() {
        return static_cast<std::size_t>(kAIVisionCells) * kAICellFeatures +
               kAIScalarFeatures;
    }
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
