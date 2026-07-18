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
    
    // Reactive Action selection (Neural network inference or decision tree)
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

## 📈 5. Next Steps for Implementation

1. **Verify State & Input Framework**: Ensure `InputManager` supports registering P2 controllers (Keyboard/Gamepad) for Split-Screen and Shared-Screen configurations.
2. **Prototype Shadow Mario**: Implement the ring-buffer recording loop inside `PlayingState` and hook up position interpolation to ensure smooth tracking.
3. **A* Helper Integration**: Implement a basic pathfinding helper on the `TileMap` to allow the AI to navigate platforms on custom levels.
