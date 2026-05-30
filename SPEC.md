# Super Mario Game — Global Specification (FROZEN)

> **Version**: 1.0 — 2026-05-30
> **Status**: APPROVED — Do not modify without user confirmation.
> **Source**: Compiled from user answers in `implementation_plan.md` Section 6.

---

## 1. Project Overview

| Field | Value |
| :--- | :--- |
| **Project** | CS202 Final Project — Super Mario Bros. |
| **Language** | C++17 |
| **Engine** | SFML 3.0.2 + ImGui-SFML v3.0 |
| **Build** | CMake with FetchContent |
| **Rendering** | 2D only (no 3D) |
| **Platform** | macOS (primary), cross-platform via SFML |

---

## 2. Architecture

### 2.1 Layered Architecture

```
Application Layer    →  main.cpp (entry point)
Core Layer           →  Game, GameStateManager, InputManager, ResourceManager, SoundManager, EventBus
Game States          →  MenuState, CharSelectState, PlayingState, PauseState, GameOverState, VictoryState
World Layer          →  World, TileMap, Camera, HUD
Entity Layer         →  Entity → Character → Mario/Luigi/Enemy; Entity → Item; Entity → Block
Physics Layer        →  PhysicsEngine, AABB, CollisionDetector, CollisionResolver
Infrastructure       →  Renderer, AnimationManager, Serializer, EntityFactory, EventBus
```

### 2.2 Design Patterns (6 Total — 5 Required + 1 Bonus)

| # | Pattern | Class(es) | Purpose |
| :--- | :--- | :--- | :--- |
| 1 | **Factory** | `EntityFactory` | `createEntity(type, pos)` → `std::unique_ptr<Entity>`. Spawns all enemies, items, blocks. |
| 2 | **Singleton** | `Game`, `ResourceManager`, `SoundManager` | Single instances. Lazy-init via `getInstance()`. |
| 3 | **State** | `GameStateManager` / `IGameState`, `PlayerState` | Game flow (Menu→Play→Pause→GameOver). Player forms (Small/Super/Fire/Star). |
| 4 | **Observer** | `EventBus` | Publish/subscribe: `CoinCollected`, `EnemyDefeated`, `PlayerDied`, `PowerUpCollected`. HUD and SoundManager subscribe. |
| 5 | **Strategy** | `IMovementStrategy` | Enemy AI: `PatrolStrategy`, `ChaseStrategy`, `FlyStrategy`. Swappable at runtime. |
| 6 | **Command** *(bonus)* | `ICommand` + `InputManager` | Keyboard → game commands (`JumpCommand`, `MoveLeftCommand`, `FireCommand`). Enables rebindable keys. |

---

## 3. Window & Rendering

| Setting | Value |
| :--- | :--- |
| **Window Resolution** | 1280 × 720 pixels |
| **Tile Size** | 32 × 32 pixels |
| **Visible Width** | 40 tiles (1280 / 32) |
| **Visible Height** | 22.5 tiles (720 / 32) |
| **Art Style** | 16-bit SNES style |
| **Sprite Source** | Open-source (OpenGameArt, The Spriters Resource, etc.) |
| **Sprite Dimensions** | 32 × 32 base |
| **Scrolling** | Horizontal only |
| **Background** | Parallax scrolling (multiple layers: sky, clouds, mountains, ground) |
| **Particles** | Brick fragments, coin sparkles, death poof, stomp effects |
| **Transitions** | Fade-in/fade-out between levels and screens |
| **Dev Tools** | ImGui-SFML overlay (toggle with F12 or similar) |

---

## 4. Physics & Gameplay Constants

> All values are exposed to ImGui for real-time tuning.

| Constant | Initial Value | Notes |
| :--- | :--- | :--- |
| **Gravity** | 0.5 px/frame² (= 1800 px/s² at 60fps) | Tunable via ImGui |
| **Mario Walk Speed** | 150 px/s | Tunable via ImGui |
| **Mario Run Speed** | 300 px/s | Tunable via ImGui |
| **Mario Jump Height** | ~4 tiles (128 px at 32px/tile) | Tunable via ImGui |
| **Fixed Timestep** | 1/60 s (16.667 ms) | With interpolated rendering |
| **Physics** | Custom AABB | No Box2D dependency |

### 4.1 Luigi Modifiers (Relative to Mario)

| Attribute | Multiplier |
| :--- | :--- |
| Jump Force | ×1.2 (jumps higher) |
| Walk/Run Speed | ×0.85 (slower) |
| Airborne Gravity | ×0.9 (floatier) |
| **Special** | Double jump ability |

### 4.2 Fireball Mechanics

| Property | Value |
| :--- | :--- |
| Max Active | 2 at once |
| Behavior | Bounces on ground |
| Destruction | On wall impact |
| Fire Rate | ~0.3s cooldown between shots |

---

## 5. Characters

### 5.1 Player Characters

| Character | Select | Special |
| :--- | :--- | :--- |
| **Mario** | Default | Standard physics |
| **Luigi** | Alt | Higher jump, slower, floatier, double jump |

- Switching: Both at character select screen AND during gameplay via hotkey.
- Two-player: Same keyboard, versus mode.

### 5.2 Player States (State Pattern)

```
Small Mario/Luigi  ──(Mushroom)──▶  Super  ──(Fire Flower)──▶  Fire
     ▲                                ▲                          │
     │                                └──────(Hit)───────────────┘
     └───────────────────(Hit)────────────────┘

Star = temporary overlay state on any form (10 seconds, invincible)
```

| State | Size | Abilities | On Hit |
| :--- | :--- | :--- | :--- |
| **Small** | 1 tile (32px) | Walk, run, jump | Die (lose life) |
| **Super** | 2 tiles (64px) | Walk, run, jump, break bricks | → Small (invincibility frames) |
| **Fire** | 2 tiles (64px) | Walk, run, jump, break bricks, shoot fireballs | → Super (invincibility frames) |
| **Star** | Same as current | All current + invincible, kills enemies on contact | Timer expires → return to base state |

---

## 6. Enemies

### 6.1 Enemy Roster

| Enemy | AI Strategy | Stomp Result | Fireball Result |
| :--- | :--- | :--- | :--- |
| **Goomba** | `PatrolStrategy` — walks one direction, reverses on wall, falls off ledges | Dies (squish animation) | Dies |
| **Koopa Troopa** | `PatrolStrategy` — walks, reverses on wall | Retreats into shell (kickable) | Dies |
| **Koopa Paratroopa** | `FlyStrategy` — flies in a pattern | Loses wings → becomes Koopa Troopa | Dies |
| **Boo (Ghost)** | `ChaseStrategy` — idle until player within 250px, then chases. Stops when player faces it. | Cannot be stomped | Cannot be killed by fireball |
| **Bowser (Boss)** | Custom boss AI — Level 3 end | Multi-hit (health bar) | Takes damage |

### 6.2 Proximity AI Detail (Boo)

- **Idle Range**: > 250 pixels from player
- **Chase Range**: ≤ 250 pixels from player
- **Behavior**: Moves toward Mario when Mario's back is turned. Freezes when Mario faces it (classic Boo behavior).
- **Invulnerable**: Cannot be stomped or killed by fireballs. Player must avoid.

---

## 7. Items & Power-ups

| Item | Source | Effect |
| :--- | :--- | :--- |
| **Super Mushroom** | QuestionBlock | Small → Super |
| **Fire Flower** | QuestionBlock | Super → Fire (if Small, acts as Mushroom first) |
| **Coin** | QuestionBlock, floating in level | +1 coin, +200 score. 100 coins = 1 extra life |
| **Star** | QuestionBlock | 10 seconds invincibility overlay |
| **1-UP Mushroom** | QuestionBlock (rare) | +1 life |

- Items spawn **only from QuestionBlocks** (no hidden blocks in MVP).
- Power-down chain: Fire → Super → Small (step-down on each hit).

---

## 8. Levels & World

### 8.1 Level Specifications

| Level | Theme | Width | Difficulty | Unique Features |
| :--- | :--- | :--- | :--- | :--- |
| **Level 1** | Overworld / Grassland | 200 tiles (6400px) | Easy | Few enemies, flat terrain, tutorial-like |
| **Level 2** | Underground / Cave | 200 tiles (6400px) | Medium | Gaps/pits, more enemies, darker palette |
| **Level 3** | Castle / Lava | 200 tiles (6400px) | Hard | Lava pits, high enemy density, Bowser boss |

### 8.2 Level Structure

| Property | Value |
| :--- | :--- |
| **Format** | 3 continuous stages, each with mid-level checkpoints |
| **File Format** | JSON |
| **Height** | 22.5 tiles visible (720px / 32px) |
| **End Marker** | Flagpole (classic Mario) |
| **Warp Pipes** | Yes — teleport to bonus coin areas or between level sections |
| **Checkpoints** | Mid-level checkpoint flags. On death: respawn at last checkpoint. |

### 8.3 Level JSON Schema (Draft)

```json
{
  "name": "Level 1 - Grassland",
  "theme": "overworld",
  "width": 200,
  "height": 23,
  "tileSize": 32,
  "backgroundLayers": ["sky.png", "clouds.png", "mountains.png"],
  "spawnPoint": { "x": 2, "y": 20 },
  "checkpoints": [{ "x": 100, "y": 20 }],
  "flagpole": { "x": 198, "y": 8 },
  "tiles": [
    { "type": "ground", "x": 0, "y": 22, "w": 50 },
    { "type": "brick", "x": 20, "y": 16 },
    { "type": "question_block", "x": 22, "y": 16, "item": "coin" }
  ],
  "entities": [
    { "type": "goomba", "x": 30, "y": 21 },
    { "type": "koopa", "x": 50, "y": 21 },
    { "type": "coin", "x": 25, "y": 14 }
  ],
  "pipes": [
    { "entrance": { "x": 40, "y": 20 }, "exit": { "x": 40, "y": 10 }, "target": "bonus_area_1" }
  ]
}
```

---

## 9. Game Flow & UI

### 9.1 Screen Flow

```
Main Menu ──▶ Character Select ──▶ Level 1 ──▶ Level Complete ──▶ Level 2 ──▶ ... ──▶ Victory
   │                                  │  ▲
   │                                  ▼  │
   │                               Pause Menu
   │                                  │
   │                          ┌───────┴───────┐
   │                          │               │
   │                       Resume      Restart Level
   │                                          │
   └──────────◀── Game Over ◀─── (lives = 0) ┘
```

### 9.2 Main Menu Options

- New Game
- Load Game
- Options (Volume: SFX slider + Music slider)
- High Scores (persisted across sessions)
- Quit

### 9.3 Pause Menu Options

- Resume
- Restart Level
- Return to Main Menu
- Quit

### 9.4 HUD Elements (Always Visible During Gameplay)

| Element | Position | Format |
| :--- | :--- | :--- |
| Score | Top-left | `SCORE 000000` |
| Coins | Top-center-left | `×00` with coin icon |
| World/Level | Top-center | `WORLD 1-1` |
| Time | Top-center-right | `TIME 300` (countdown) |
| Lives | Top-right | `×3` with character icon |

### 9.5 Lives & Game Over

- Start with **3 lives**.
- On death: respawn at **last checkpoint** (or level start if no checkpoint reached).
- On Game Over (lives = 0): return to **Main Menu**.
- Time limit: **300 game-seconds** per level.

---

## 10. Audio

### 10.1 Sound Effects (SFX)

| Event | SFX File |
| :--- | :--- |
| Jump | `sfx/jump.wav` |
| Coin Collect | `sfx/coin.wav` |
| Stomp Enemy | `sfx/stomp.wav` |
| Power-up | `sfx/powerup.wav` |
| Power-down | `sfx/powerdown.wav` |
| Fireball | `sfx/fireball.wav` |
| 1-UP | `sfx/oneup.wav` |
| Player Death | `sfx/death.wav` |
| Flagpole | `sfx/flagpole.wav` |
| Pipe Warp | `sfx/pipe.wav` |

### 10.2 Background Music (BGM)

| Context | BGM File |
| :--- | :--- |
| Main Menu | `music/menu.ogg` |
| Overworld (Level 1) | `music/overworld.ogg` |
| Underground (Level 2) | `music/underground.ogg` |
| Castle (Level 3) | `music/castle.ogg` |
| Star Power | `music/star.ogg` (overrides level BGM while active) |
| Game Over | `music/gameover.ogg` (jingle) |
| Victory | `music/victory.ogg` (jingle) |

### 10.3 Audio Controls

- Separate volume sliders: SFX and Music (in Options menu).
- Source: Open-source / royalty-free retro-style audio.

---

## 11. Save/Load (Serialization)

### 11.1 Format

JSON — human-readable, easy to debug.

### 11.2 Persisted Data

```json
{
  "version": "1.0",
  "timestamp": "2026-05-30T14:30:00Z",
  "player": {
    "character": "mario",
    "state": "fire",
    "position": { "x": 1024.5, "y": 640.0 },
    "lives": 3,
    "score": 12500,
    "coins": 47
  },
  "level": {
    "id": 2,
    "name": "Underground",
    "timeRemaining": 185.5,
    "checkpoint": { "x": 100, "y": 20 }
  },
  "highScores": [50000, 42000, 35000, 28000, 12500]
}
```

### 11.3 Save Slots

- 3 save slots available.
- Auto-save at checkpoint.
- Manual save from pause menu.

---

## 12. Two-Player Mode (Versus)

| Setting | Value |
| :--- | :--- |
| **Mode** | Same keyboard, versus |
| **Player 1** | WASD + Q (fire) + E (special) |
| **Player 2** | Arrow keys + M (fire) + N (special) |
| **Objective** | Both players in same level. Race to flagpole. Can stomp each other (or configurable). |

---

## 13. Controls

### 13.1 Single-Player Keyboard Mapping

| Action | Key |
| :--- | :--- |
| Move Left | A or ← |
| Move Right | D or → |
| Jump | W or ↑ or Space |
| Run | Left Shift (hold) |
| Fire | F or J |
| Pause | Escape |
| Switch Character | Tab |
| Dev Tools (ImGui) | F12 |

### 13.2 Input Architecture

- **Command Pattern**: Keyboard → `ICommand` objects → Character actions.
- Abstraction layer for future gamepad/controller support.

---

## 14. Bonus Features (Post-MVP — May Introduce Later)

These features are **not in MVP scope** but are planned for post-MVP development if time allows.

### 14.1 Mario Maker (In-Game Level Editor)

- **Trigger**: Press F1 to pause and open editor overlay.
- **UI**: ImGui drag-and-drop: place tiles, enemies, items, coins.
- **Pattern**: Command Pattern for Undo/Redo.
- **Serialization**: Export/import levels as JSON.
- **Academic Value**: Nails the "Save/Load / Serialization" bonus requirement.

### 14.2 Time Manipulation ("Braid" Mechanic)

- **Trigger**: Hold Shift to rewind time.
- **Implementation**: Store game state snapshots in a circular buffer (~5 seconds = 300 frames).
- **Pattern**: Memento Pattern.
- **Academic Value**: Demonstrates advanced data structures (circular buffer).

### 14.3 Shadow Mario (Rival AI)

- **Feature**: Dark Mario that mimics player inputs with a 3-second delay.
- **Implementation**: Record player input history, replay through AI controller.
- **Pattern**: State Pattern + Strategy Pattern for pathing.
- **Academic Value**: Goes beyond basic proximity AI.

### 14.4 Dynamic Lighting & Weather (SFML Shaders)

- **Feature**: Day/night cycle, underground darkness with player-centered light radius, rain effects.
- **Implementation**: `sf::Shader` (GLSL fragment shaders).
- **Academic Value**: Graphics programming, unique visual presentation.

### 14.5 Procedural Level Generation (Infinite Mario)

- **Feature**: "Endless Mode" with dynamically generated terrain.
- **Implementation**: Chunk-based generation with difficulty scaling (Perlin noise or rule-based).
- **Academic Value**: Algorithmic thinking, procedural generation.

---

## 15. Rubric Alignment

| Requirement | Points | Our Implementation |
| :--- | :--- | :--- |
| Player Inputs, Movement, Collision | 20 | AABB physics, Command pattern input, 2-player support |
| Enemy Behavior | 10 | 5 enemy types with Strategy pattern AI |
| Power-Ups and Items | 10 | 5 item types, 4 player states |
| 3 Level Completion | 15 | Overworld, Underground, Castle with checkpoints |
| Sounds | 10 | 10+ SFX, per-level BGM, volume controls |
| OOP Design | 10 | Deep class hierarchy, SOLID principles, smart pointers |
| 5 Design Patterns | 25 | Factory, Singleton, State, Observer, Strategy (+Command) |
| AI | 5 | Patrol, Chase, Fly strategies; Boo proximity; Boss AI |
| Multiple Players | 5 | Mario + Luigi, character select + hotkey switch, 2P versus |
| **Total** | **110/115** | (no 3D = -5) |
