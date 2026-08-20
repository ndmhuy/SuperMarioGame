# 🤖 2-Player Mode: AI & Screen Layout Specification (Expanded & Diverse Spec)

This document presents the detailed design for the **2-Player Mode** featuring both **Human vs. Human** and **Human vs. AI** configurations. It covers diverse camera layouts (Shared-Screen vs. Split-Screen), distinct AI archetypes/difficulties, and the integration of the **Shadow Mario (Dark Mario)** replay mechanic.

---

## 🎮 1. The Four Gameplay Modes

To provide maximum variety, players can select from four distinct multiplayer modes:

```
┌───────────────────────────────────────────────────────────────────────────┐
│                           MULTIPLAYER GAME MODES                          │
├─────────────────────────────────────┬─────────────────────────────────────┤
│ 1. Shared-Screen Versus (Combat)     │ 2. Split-Screen Speedrun (Race)     │
│ - Players share same camera frame.  │ - Vertical split screen.            │
│ - Direct collision & stomping.      │ - Independent camera tracking.      │
│ - High interaction and combat.      │ - Pure speed race (Isolated/Shared).│
├─────────────────────────────────────┼─────────────────────────────────────┤
│ 3. Shared-Screen Co-op (Buddy)      │ 4. Shadow Mario Chase (Puzzle)      │
│ - Cooperate to reach the end.       │ - P2 is a dark clone of P1.         │
│ - Double-jump boosts off P1's head.  │ - Mimics P1's inputs with 3s lag.   │
│ - Shared lives/score pool.          │ - Touching the clone damages P1.    │
└─────────────────────────────────────┴─────────────────────────────────────┘
```

### 1.1 Shared-Screen Versus (Classic Combat Mode)
* **Screen Layout**: Single shared view. Camera tracks the midpoint: $x_{cam} = \frac{x_{P1} + x_{P2}}{2}$.
* **Camera Bounds Constraints**:
  - *Hard Bounding (Splat)*: The camera follows the leading player. If the trailing player falls off the left screen edge, they take 1 point of damage, lose their active power-up state, and are automatically teleported to drop back in from the top-center of the screen.
* **Combat**: Enabled. Players can stomp on each other's heads (causing a 0.5s stun to the stomped player, and a vertical bounce boost to the stomping player), push each other, and shoot fireballs that stun or damage the opponent.
* **Resource Sharing**: Items (Mushrooms, Flowers) are limited. First to touch them claims them.

### 1.2 Split-Screen Speedrun (Independent Race Mode)
* **Screen Layout**: Vertical split screen. Player 1 view on the left ($640 \times 720$), Player 2/AI view on the right ($640 \times 720$).
* **Configurations**:
  - *Isolated Dimension*: Players run on separate instances of the level. No contact, no items stolen, pure time trial.
  - *Interconnected Dimension*: Players exist in the same level world but see it through separate cameras. If P1 breaks a block, it is broken for P2. If P1 kills an enemy, it vanishes for P2. Direct collision is active only if they are at the same level coordinates (indicated by an icon showing the other player's distance/direction).

### 1.3 Shared-Screen Co-op (Cooperative Buddy Mode)
* **Screen Layout**: Single shared view. Camera bounds are *Soft*: the leading player cannot scroll the screen forward if the trailing player is at the left edge.
* **Co-op Mechanics**:
  - Friendly fire is disabled (fireballs do not hurt the partner).
  - *Boost Jump*: Jumping on the partner's head gives a high-bounce boost (up to 8 tiles high) without hurting or stunning them.
  - Shared pool of Lives and Score.

### 1.4 Shadow Mario Chase (Time-Lag Puzzle Mode)
* **Screen Layout**: Single shared view focusing on Player 1.
* **Concept**: Player 2 is replaced by **Shadow Mario** (an invulnerable dark clone). Shadow Mario has no independent intelligence; instead, it replicates the player's exact movement commands delayed by exactly 3 seconds.
* **Objective**: The player must complete the level while avoiding their own past trail. Touching Shadow Mario instantly damages the player.

---

## 🧠 2. AI Archetypes & Difficulty Tiers

To make the AI feel alive and diverse, we define three **difficulties** and three **behavioral archetypes** that can be mixed and matched.

### A. Difficulty Adjustments

| Parameter | 🟢 Easy | 🟡 Normal | 🔴 Hard |
| :--- | :--- | :--- | :--- |
| **Observation Grid** | $5 \times 5$ tiles | $11 \times 11$ tiles | $21 \times 15$ tiles (Full Screen) |
| **Reaction Latency** | 400ms delay | 120ms delay | 0ms delay (Frame-Perfect) |
| **Action Noise ($\epsilon$)** | $\epsilon = 0.35$ (Clumsy) | $\epsilon = 0.05$ (Suboptimal) | $\epsilon = 0.0$ (Flawless Execution) |
| **Allowed Controls** | Walk, Normal Jump | Walk, Run, Jump, Shoot | Walk, Run, Wall Jump, Slide, Ground Pound |

### B. Behavioral Archetypes (AI Personalities)
These adjust the **reward weightings** inside the AI policy:

1. **The Speedrunner (Time-Optimized)**:
   - *Utility*: Focuses 100% on rightward progress and finding optimal path shortcuts. Ignores coins, items, and combat unless they block the path.
2. **The Hunter (Aggressive Combatant)**:
   - *Utility*: Heavily rewards harming Player 1. If P1 is nearby, the Hunter will abandon pathfinding temporarily to position itself for a stomp or fire fireballs/shells.
3. **The Collector (Resource Hoarder)**:
   - *Utility*: Prioritizes hitting Question Blocks, collecting Coins, and grabbing Mushrooms/Flowers. It runs slower but is usually fully powered up.

---

## 👥 3. Shadow Mario (Dark Mario) Design

Shadow Mario acts as a physical history trace of the player. It is implemented using a **Ring Buffer Command Replay** system.

```mermaid
flowchart LR
    Player[Human Player] -- Collects Inputs -- > Frame[Active Frame]
    Frame -- Record State -- > Queue[(Delay Queue / Ring Buffer)]
    Queue -- 3.0 Seconds Delay -- > Replay[Replay Engine]
    Replay -- Executes Commands -- > Shadow[Shadow Mario Entity]
```

### 3.1 Input Recording Mechanism
Every frame, the game records the active player's input commands and physics state into a structural frame packet:

```cpp
struct PlayerFramePacket {
    float timestamp;
    sf::Vector2f position;
    bool moveLeft;
    bool moveRight;
    bool jump;
    bool run;
    bool crouch;
    bool shoot;
    bool groundPound;
};
```
These packets are pushed to a `std::deque<PlayerFramePacket>` inside the level controller.

### 3.2 Replay Execution
At each update cycle:
1. The replay engine checks the front of the queue:
   $$\text{Current Time} - \text{Packet Timestamp} \ge 3.0\text{ seconds}$$
2. If true, the packet is popped, and the inputs are applied directly to the **Shadow Mario** entity.
3. **Position Correction**: To prevent desyncs caused by floating-point rounding errors or frame rate variance, the replay engine gently interpolates (lerps) the Shadow Mario's position to the packet's recorded position if the difference exceeds a small threshold ($> 4\text{ pixels}$):
   $$\text{Position} = \text{Position} + 0.1 \times (\text{Recorded Position} - \text{Position})$$

### 3.3 Interactions
* **Damage**: Shadow Mario carries a persistent hazard collider. Touching it deals 1 point of damage to the human player.
* **Immunity**: Shadow Mario is immune to Goombas, Koopas, fireballs, and hazards. It cannot fall into pits unless the player did (since it mirrors the exact path).
* **Color Cycling**: Rendered with a translucent dark-purple sprite, trailing ghost particles to visually distinguish it.

---

## 🧱 4. C++ Class Integration

We integrate these features into the engine using the **Strategy** and **Decorator** patterns.

```
                  Entity (Abstract)
                     │
                 Character
                     │
            ┌────────┴────────┐
            │                 │
          Player         Enemy (Abstract)
            │
      ┌─────┴────────────────────────────────┐
      │                                      │
    Mario / Luigi / etc.               ShadowMario
  (Uses InputManager for P1,            (Uses DelayQueueReplay
   AIController for P2 AI)                for movement inputs)
```

### Proposed C++ Interfaces

```cpp
// include/Entities/AIController.hpp
#pragma once
#include <memory>
#include "Entities/Player.hpp"
#include "Utils/TileMap.hpp"

enum class AIScreenMode { SharedScreen, SplitScreen };
enum class AIAroundState { Empty, Solid, Threat, Reward, Enemy };

class AIController {
public:
    AIController(Player& controlledPlayer, AIScreenMode screenMode);
    
    void update(float dt, const Player& humanPlayer, const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities);
    void setDifficulty(int difficultyLevel); // Easy, Normal, Hard
    void setArchetype(int archetypeId);      // Speedrunner, Hunter, Collector

private:
    Player& m_player;
    AIScreenMode m_screenMode;
    int m_difficulty = 1;
    int m_archetype = 0;
    float m_decisionTimer = 0.0f;
    float m_reactionDelay = 0.12f; // 120ms for Normal
    
    // Abstract grid vision mapping
    std::vector<std::vector<AIAroundState>> scanEnvironment(const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities);
    
    // Heuristic Path Planning (A*)
    sf::Vector2f planPathToGoal(const TileMap& tileMap);
    
    // Decision-making is delegated to an IAIPolicy (see §6): heuristic decision
    // tree first, neural network later — the controller only senses and actuates.
    std::unique_ptr<IAIPolicy> m_policy;
    void executeTacticalMove(const sf::Vector2f& targetWaypoint, const std::vector<std::vector<AIAroundState>>& visionGrid);
};
```

```cpp
// include/Entities/ShadowMario.hpp
#pragma once
#include "Entities/Player.hpp"
#include <deque>

struct PlayerFramePacket {
    float timestamp;
    sf::Vector2f position;
    bool moveLeft;
    bool moveRight;
    bool jump;
    bool run;
    bool crouch;
    bool shoot;
};

class ShadowMario : public Player {
public:
    ShadowMario(sf::Vector2f startPos);
    void update(float dt) override;
    void recordFrame(float gameTime, const Player& targetPlayer);
    
private:
    std::deque<PlayerFramePacket> m_historyBuffer;
    float m_delayTime = 3.0f; // 3 seconds lag
};
```

---

## 🖥️ 5. GUI & UX Adaptation Per Mode

Every mode needs the UI to acknowledge it. The engine's existing screens (`MenuState`, `CharacterSelectState`, `OptionsState`, `PauseState`, `VictoryState`, `GameOverState`) and the ImGui `DevPanel` are the touch points.

### 5.1 Mode Selection Flow
* `MenuState`'s `ROW_VERSUS` becomes the entry to a **mode picker** (Versus / Co-op / Shadow Chase; Split-Screen greyed out until implemented) instead of hardcoding shared-screen versus.
* For Versus and Co-op, an **opponent picker**: Human (P2 keys) or CPU. Choosing CPU exposes difficulty (Easy/Normal/Hard) and archetype (Speedrunner/Hunter/Collector) selectors.
* `CharacterSelectState` must forward the chosen mode (today the versus path bypasses it — P1 is always Mario on 1-1). In 2P-human it runs twice (P1 pick, then P2 pick); with a CPU opponent, P2's portrait shows a "CPU" badge with the archetype icon.

### 5.2 In-Game HUD Per Mode
* **Versus**: generalize `renderVersusHud` — replace the hardcoded "P1/P2" labels with slot names ("P2" vs "CPU · HUNTER"), keep the lead indicator.
* **Co-op**: single shared pool display (lives, score) plus both players' power-up forms side by side; no lead indicator.
* **Shadow Chase**: standard P1 HUD plus a **shadow gauge** — a 3-second ring/bar showing how far behind the shadow is, flashing a proximity warning when the shadow is within ~3 tiles of the player.
* **Minimap**: add a second marker for P2 / the shadow (today it renders P1 only).

### 5.3 End & Pause Screens
* `VictoryState` / `GameOverState` need a mode-aware headline: "P1 WINS" / "CPU WINS" (Versus), shared result (Co-op), and for Shadow Chase, "CAUGHT BY YOUR SHADOW" on shadow-touch death.
* `PauseState` in a CPU match should freeze the `AIController` cleanly (no decision-timer drift across pause).

### 5.4 Settings & Dev Tooling
* `OptionsState`: expose the P2 keybinding set that already exists in `InputManager` (arrows/M/N defaults) for rebinding, and resolve the `M`-key collision with the minimap toggle.
* **ImGui `DevPanel`** (per AGENTS.md tunability rule): sliders for shadow delay (default 3.0s), lerp threshold/factor, AI reaction latency, action noise ε, and live archetype/difficulty switching; plus an AI debug overlay drawing the observation grid and current target waypoint (extend the existing strategy `getDebugState()` overlay).

---

## 🧬 6. RL-Ready Policy Seam (Side Project: Neural Network AI)

The heuristic AI ships first, but the `AIController` is deliberately split so a trained **reinforcement-learning policy can be plugged in later without touching the game loop**. The controller owns *sensing* and *actuation*; the *decision* is behind an interface:

```cpp
// include/Entities/IAIPolicy.hpp
#pragma once
#include <vector>

struct AIObservation {
    // Fixed-size, normalized — this IS the RL observation space.
    std::vector<float> grid;      // flattened vision grid (AIAroundState one-hot), size = W*H*5
    float dxToGoal, dyToGoal;     // normalized offset to current objective
    float dxToOpponent, dyToOpponent;
    float vx, vy;                 // own velocity, normalized
    bool onGround, canJump, isPoweredUp;
};

struct AIAction {
    // Discrete action space, mirrors PlayerFramePacket's buttons.
    bool moveLeft, moveRight, jump, run, crouch, shoot, groundPound;
};

class IAIPolicy {
public:
    virtual ~IAIPolicy() = default;
    virtual AIAction decide(const AIObservation& obs) = 0;
};
```

* `HeuristicPolicy : IAIPolicy` — the utility-scored decision tree shipping in the first pass (archetypes = reward weightings, per §3B).
* `NeuralPolicy : IAIPolicy` — later wraps the neural network for inference; same observation in, same action out.
* **Design consequences to honor now** (cheap now, painful to retrofit):
  1. `AIObservation` is fixed-size and normalized to [-1, 1] from day one — heuristic and network consume the identical struct.
  2. `AIAction` mirrors `PlayerFramePacket`'s button fields, so a recorded human session doubles as **imitation-learning data** and the shadow-replay machinery doubles as an action executor.
  3. Difficulty's ε-noise is applied *outside* the policy (in `AIController`), so it works identically for both — and maps directly onto ε-greedy exploration during training.
  4. **Reward hooks via `EventBus`**: the reward function is an EventBus subscriber (coin/damage/kill/flag events already exist as events), not code inside the policy. Training reads rewards without touching gameplay code.
  5. Keep `decide()` synchronous and allocation-free on the hot path; the controller calls it at the difficulty's reaction cadence, not every frame.
* **Out of scope for now, noted for training later**: a headless/accelerated stepping mode for the sim, and (de)serialization of network weights. These get their own proposal when the side project starts.

---

## 📈 7. Next Steps for Implementation

1. **Verify State & Input Framework**: Ensure `InputManager` supports registering P2 controllers (Keyboard/Gamepad) for Split-Screen and Shared-Screen configurations.
2. **Prototype Shadow Mario**: Implement the ring-buffer recording loop inside `PlayingState` and hook up position interpolation to ensure smooth tracking.
3. **A* Helper Integration**: Implement a basic pathfinding helper on the `TileMap` to allow the AI to navigate platforms on custom levels.
4. **GUI Pass Per Mode**: Land the §5 items alongside each mode's mechanics — a mode is not complete until its menu route, HUD, and end screens reflect it.
5. **Policy Seam First**: Build `AIController` against `IAIPolicy` from the first commit (§6), shipping `HeuristicPolicy`; the neural network arrives later as a drop-in.
