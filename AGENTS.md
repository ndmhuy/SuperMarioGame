# AGENTS.md — Super Mario Game

> This file contains critical guidelines, pre-instructions, and compact context for AI agents working on the Super Mario Game project. **Read this file FIRST before any task.**

---

## ⚡ Quick Context (Read This First — Saves Context Window)

> [!IMPORTANT]
> **PROJECT SUMMARY IN 30 SECONDS**
>
> **What**: 2D Mario platformer in C++17 using SFML 3.0.2 + ImGui-SFML.
> **Build**: CMake with FetchContent. `mkdir build && cd build && cmake .. && make`.
> **Architecture**: Layered — Core → World → Entity → Physics → Infrastructure.
> **Patterns**: Factory, Singleton, State, Observer, Strategy, Command, Decorator, Memento, Object Pool, Template Method (10+ total).
> **Scope**: 110 features across 20 categories (v2.0 expanded spec).
> **Key Files**:
> - `SPEC.md` — **Frozen specification v2.0** (constants, behaviors, schemas). Read before implementing.
> - `FEATURE_PROPOSAL.md` — TA justification for expanded scope.
> - `implementation_plan.md` — Architecture diagrams and user answers.
> - `TASKS.md` — **Sequential task checklist** with checkboxes for keeping track of progress.
> - `logs/agent_history.log` — Append your work summary after every task.

### Constants You'll Need Repeatedly

```
Window:        1280×720 px
Tile:          32×32 px
Gravity:       0.5 px/frame² (= 1800 px/s² at 60fps)  [ImGui-tunable]
Walk Speed:    150 px/s                                  [ImGui-tunable]
Run Speed:     300 px/s                                  [ImGui-tunable]
Jump Height:   ~4 tiles (128 px)                         [ImGui-tunable]
Timestep:      1/60 s fixed, with interpolated rendering
Level Size:    200 tiles wide × 22.5 tiles tall
Coyote Time:   6 frames (100ms)
Jump Buffer:   6 frames (100ms)
Wall Slide:    50 px/s (capped downward)
Ground Pound:  600 px/s (instant slam)
Combo Mult:    ×1, ×2, ×4, ×8
```

### Luigi Modifiers (vs Mario)

```
Jump Force:     ×1.2
Walk/Run Speed: ×0.85
Airborne Gravity: ×0.9 (floatier)
Special: Double jump
```

### Entity Hierarchy (Inheritance Tree)

```
Entity (abstract)
├── Character (abstract)
│   ├── Player (abstract) → Mario, Luigi, Toad*, Peach*
│   └── Enemy (abstract) → Goomba, KoopaTroopa, KoopaParatroopa, Boo, Bowser,
│                           PiranhaPlant, BulletBill, HammerBro, Thwomp,
│                           ChainChomp, Lakitu, Spiny, BoomBoom
├── Item (abstract) → Mushroom, FireFlower, Coin, Star, OneUpMushroom,
│                     CapeFeather, MegaMushroom, MiniMushroom, POWBlock,
│                     PSwitch, Trampoline, StarCoin
└── Block (abstract) → BrickBlock, QuestionBlock, Pipe, Flagpole, HiddenBlock,
                       MovingPlatform, FallingPlatform, IceBlock, ConveyorBelt
(* = unlockable)     (25+ concrete classes total)
```

### Design Pattern → Class Mapping

```
Factory    → EntityFactory::create(type, pos) — 25+ types; Lakitu spawns Spinies
Singleton  → Game, ResourceManager, SoundManager, AchievementManager
State      → GameStateManager (9 states), IPlayerState (5 concrete base states), FallingPlatform, Thwomp
Observer   → EventBus (15+ events) — HUD, Sound, Combo, Achievement, Stats subscribe
Strategy   → 7+ strategies: Patrol, Chase, Fly, TimerEmergence, Linear, HammerThrow, TetheredChase, ProximityTrigger
Command    → 8+ commands: Jump, Move, Fire, Crouch, GroundPound, WallJump + debug console
Decorator  → StarDecorator, MegaDecorator (temporary power-up wrappers around active IPlayerState)
Memento    → GameSnapshot (time rewind, replay system)
Pool       → ObjectPool<T> for fireballs, particles, projectiles
Template   → IMovementStrategy::execute() skeleton with overridable hooks (delegated from Enemy)
```

### Player States Chain

```
Small ──(Mushroom)──▶ Super ──(FireFlower)──▶ Fire
  ▲                     ▲                       │
  └──────(Hit)──────────┘───────(Hit)───────────┘
Super ──(CapeFeather)──▶ Cape
Small ──(MiniMushroom)──▶ Mini (half-tile)

Power-down: Fire/Cape → Super → Small (step-down)
5 concrete base states: Small, Super, Fire, Cape, Mini
2 Decorators (wrap current state): Star (10s invincible), Mega (8s giant)
```

---

## Critical Compliance Directives for AI Agents

> [!IMPORTANT]
> **MANDATORY RULES FOR AGENT EXECUTION**
>
> 1. **Git Branching Policy**:
>    - All development process must take place on the `dev` branch.
>    - The `main` branch is reserved only for delivery/releases after major milestone changes.
>    - For each main task, create and work on a subbranch created from the `dev` branch (e.g., `feature/core-engine`, `feature/physics`).
>    - Switch branches cleanly and never commit directly to `main`.
>
> 2. **Commit Policy**:
>    - Commit after completing each subtask/task.
>    - Use clear, traceable commit messages that make it easy to follow the history (e.g., `feat: implement AABB collision detection`, `fix: resolve character jump gravity bug`).
>
> 3. **Automatic Task & Prompt Logging**:
>    - Agents must automatically append a summary of each user prompt and the corresponding output/results to the local log file: [agent_history.log](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/logs/agent_history.log).
>    - Do not skip this step; ensure the log is updated at the end of every interaction.
>
> 4. **Plan Deviations**:
>    - If you plan to deviate from the approved implementation plan or do what is not in the plan, you **MUST** inform the user and discuss/obtain confirmation first.
>
> 5. **Strict C++ & OOP Design**:
>    - The codebase must be written in **C++17**.
>    - Adhere strictly to Object-Oriented Programming (OOP) principles: encapsulation, inheritance, polymorphism, and abstraction.
>    - Emphasize clean software architecture and system design to ensure long-term maintainability.
>    - Avoid global state (unless using the Singleton pattern carefully) and spaghetti code.
>
> 6. **Read SPEC.md Before Implementing**:
>    - Always check `SPEC.md` for exact values, schemas, and behavioral rules before writing code.
>    - Do NOT guess physics constants, JSON schemas, or enemy behaviors — they are all defined in the spec.

---

## Technical Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C++17 |
| **Graphics/Window/Audio** | SFML 3.0.2 |
| **Dev Tools / Level Editor** | ImGui v1.91.8 + ImGui-SFML v3.0 |
| **Build System** | CMake with FetchContent |
| **JSON Parsing** | nlohmann/json (header-only, via FetchContent) |
| **Serialization** | JSON format for saves and level files |

---

## Project Structure & Architecture

```text
SuperMarioGame/              # Git root
├── AGENTS.md                # Agent instructions (this file)
├── SPEC.md                  # Frozen specification (source of truth)
├── implementation_plan.md   # Architecture diagrams + user answers
├── TASKS.md                 # Global sequential tasks file
├── README.md                # Project documentation
├── .gitignore
├── logs/
│   └── agent_history.log    # Agent interaction log
│
├── Report/                  # ── REPORT FOLDER ──
│   └── SuperMarioGame/      # LaTeX report files
│       ├── main.tex
│       └── README.md
│
└── SuperMarioGame/          # ── APP CODE FOLDER (same name as parent) ──
    ├── CMakeLists.txt       # CMake build configuration
    ├── include/             # ── HEADER FILES (.hpp) ──
    │   ├── Core/            # Game, StateManager, Input, Resource, Sound, EventBus
    │   ├── Entities/        # Entity hierarchy, Factory, AI Strategies
    │   ├── Graphics/        # Animation, Camera, HUD, SpriteSheet, Particles
    │   ├── Physics/         # AABB, PhysicsEngine, Collision Detector/Resolver
    │   └── Utils/           # Constants, TileMap, LevelLoader, Serializer, Math
    │
    ├── src/                 # ── SOURCE FILES (.cpp) ──
    │   ├── main.cpp         # Entry point: Game::getInstance().run()
    │   ├── Core/            # Mirrors include/Core/
    │   ├── Entities/        # Mirrors include/Entities/
    │   ├── Graphics/        # Mirrors include/Graphics/
    │   ├── Physics/         # Mirrors include/Physics/
    │   └── Utils/           # Mirrors include/Utils/
    │
    ├── assets/              # ── GAME ASSETS ──
    │   ├── textures/        # Sprite sheets, tilesets, backgrounds
    │   ├── sounds/sfx/      # WAV sound effects
    │   ├── sounds/music/    # OGG background music
    │   └── fonts/           # PressStart2P.ttf or similar
    │
    ├── saves/               # Save files (slot_1.json, etc.)
    └── build/               # CMake build directory
```

---

## Key Modules — When to Modify What

| If you need to... | Modify / Create |
| :--- | :--- |
| **Change game constants** (gravity, speed, tile size) | `include/Utils/Constants.hpp` |
| **Implement/Tweak physics or gravity** | `src/Physics/PhysicsEngine.cpp` |
| **Add a new entity type** | Subclass in `include/Entities/` + register in `EntityFactory` |
| **Change enemy AI behavior** | Implement new `IMovementStrategy` subclass |
| **Add a new game screen** | Implement `IGameState` subclass in `src/Core/` |
| **Add level loading or save/load** | `src/Utils/LevelLoader.cpp` or `src/Utils/Serializer.cpp` |
| **Adjust graphics, UI, or camera** | `src/Graphics/` (HUD.cpp, Camera.cpp, etc.) |
| **Add sound effects or audio** | `src/Core/SoundManager.cpp` + subscribe to `EventBus` |
| **Expose value to ImGui** | Add slider in `PlayingState` ImGui panel |

---

## Coding Conventions

### File Naming
- Headers: `include/Module/ClassName.hpp`
- Sources: `src/Module/ClassName.cpp`
- One class per file (header + source pair)

### Code Style
```cpp
// Use #pragma once for header guards
#pragma once

// Include order: std → SFML → project headers
#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>
#include "Entities/Entity.hpp"

// Namespace: none (flat) — use full class names
// Naming: PascalCase for classes, camelCase for methods/vars, UPPER_SNAKE for constants
// Smart pointers: std::unique_ptr for ownership, std::shared_ptr only when needed
// Virtual destructors on ALL base classes
```

### Example Entity Pattern
```cpp
// include/Entities/NewEnemy.hpp
#pragma once
#include "Entities/Enemy.hpp"

class NewEnemy : public Enemy {
public:
    explicit NewEnemy(sf::Vector2f position);
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void onStomped() override;
    void onHitByFireball() override;
};
```

---

## Common Pitfalls & Anti-Patterns

> [!WARNING]
> **NO DIRECT BRANCH COMMITS TO MAIN**
> Never make commits directly to `main`. Always branch off `dev`, merge back into `dev`, and let the user handle final merges to `main` for deliveries.

> [!WARNING]
> **ENFORCE SOLID PRINCIPLES**
> - **Single Responsibility Principle (SRP)**: Keep entities separate from physics collision logic. Do not build huge monolith classes.
> - **Dependency Inversion**: High-level systems should not depend directly on low-level rendering code. Wrap renderer interfaces.
> - **Open/Closed Principle**: Add new enemies or items by subclassing `Enemy` or `Item`, not by modifying switch blocks in character collision methods.

> [!IMPORTANT]
> **MEMORY MANAGEMENT**
> In C++, ensure no memory leaks occur. Prefer smart pointers (`std::unique_ptr` and `std::shared_ptr`) over raw pointers, especially in vectors of entities. Ensure destructors of base classes are marked `virtual`.

> [!CAUTION]
> **SFML 3.0.2 API DIFFERENCES**
> SFML 3.0 has breaking changes from SFML 2.x:
> - `sf::Texture::loadFromFile()` → use `sf::Texture("path")` or check 3.0 API
> - `sf::Keyboard::isKeyPressed()` uses scoped enums: `sf::Keyboard::Key::W`
> - Event types changed: `sf::Event::KeyPressed` → `sf::Event::is<sf::Event::KeyPressed>()`
> - Always check SFML 3.0 documentation, NOT 2.x tutorials.

> [!NOTE]
> **AUTOMATIC LOG ENTRY FORMAT**
> Every time you finish a task, append to `logs/agent_history.log` in this format:
> ```text
> [YYYY-MM-DD HH:MM:SS] Branch: <branch_name>
> Prompt: <brief prompt summary>
> Files Modified: <list of files>
> Summary of Changes: <brief bulleted list>
> ---
> ```
