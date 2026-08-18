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

## Member Recognition & Private Knowledge System

To support seamless collaboration between Member A and Member B, agents must dynamically identify which developer is currently working and adapt to their specific custom preferences/orders using a dual system:

### 1. Dynamic Member Recognition
At the start of every session, the agent MUST run checks to identify the active developer:
- **Git Branch Check**: Retrieve the active git branch name (e.g. running `git branch --show-current`).
  - Branches starting with `A/` indicate **Member A (Huy - Engine & Infrastructure)** is working.
  - Branches starting with `B/` indicate **Member B (Partner - Entities & Gameplay)** is working.
- **Git User Check**: Retrieve the local git user configuration (`git config user.name`).
- **Profile Check**: Check if `.member_profile.json` exists at the workspace root.

### 2. Private Knowledge & Custom User Orders
Since all git-tracked files are shared, custom personal notes, reminders, or private user orders for the agent should be kept in a local, gitignored configuration file called `.member_profile.json` at the project root.

Each developer should create their own `.member_profile.json` locally. The agent must search for and read this file (if present) to load custom, member-specific orders.

#### Example `.member_profile.json` Schema:
```json
{
  "memberName": "A",
  "developerName": "Huy Nguyen",
  "privateNotes": "Any specific reminders or notes from the user that should not be shared via Git.",
  "agentCustomInstructions": [
    "Prioritize performance optimizations for SFML rendering.",
    "Ensure CMake config remains clean and builds in debug mode."
  ],
  "customBuildDir": "build_debug",
  "privateTasks": [
    "Refactor PhysicsEngine collision resolver before Wednesday class"
  ]
}
```
If `.member_profile.json` is missing, the agent must gracefully fall back to git branch prefix detection or invite the developer to create this file.

---

## Critical Compliance Directives for AI Agents

> [!IMPORTANT]
> **MANDATORY RULES FOR AGENT EXECUTION**
>
> 1. **Git Branching & Sync Policy**:
>    - All development process must take place on branches created off `dev`.
>    - The `main` branch is reserved only for delivery/releases after major milestone changes.
>    - **Branch Naming Convention**: Task branches must use naming prefixes based on the assigned member:
>      - Member A: `A/branch-name` (e.g., `A/core-engine`, `A/physics`).
>      - Member B: `B/branch-name` (e.g., `B/input-sound`, `B/enemies`).
>    - **Syncing with Other Members**: At the start of every working session, pull the upstream `dev` branch (`git pull origin dev`) to your local `dev` branch to fetch the other member's merged work. Then, rebase your task branch against the updated local `dev` branch to minimize conflicts.
>    - **No Auto-Merges**: Agents must **NOT** merge task branches into `dev` **on their own initiative**. The agent's default job is to implement the task on its branch, verify compilation, and push the branch to origin. The user handles code reviews and merges.
>      - **Exception — user-directed merges**: if the user explicitly asks for a merge, perform it, but: record the `Git Fingerprint` before and after; resolve every conflict by **combining** both sides where both carry real logic, never by blanket `--ours`/`--theirs`; state in the log which files conflicted and how each was resolved; and rebuild plus run the game afterwards.
>    - **Never push without being asked.** Committing locally is the default; pushing publishes to the other member and is the user's call.
>    - Keep `main` current at milestones. As of August 2026 `main` sat 153 commits behind `dev` with "Initial commit" at its tip — meaning the default branch represented nothing. Fast-forward and tag at each milestone.
>
> 2. **Commit Policy**:
>    - Commit after completing each subtask/task.
>    - Use clear, traceable commit messages that make it easy to follow the history (e.g., `feat: implement AABB collision detection`, `fix: resolve character jump gravity bug`).
>
> 3. **Automatic Task & Prompt Logging**:
>    - Agents must automatically append a summary of each user prompt and the corresponding output/results to the local log file: [agent_history.log](logs/agent_history.log).
>    - Do not skip this step; ensure the log is updated at the end of every interaction.
>    - Every log entry **MUST** contain **all** fields in the log format template below — `Git Fingerprint`, `Fetched Remotes`, `Reachable From Main` and `Verified By` included. A field you cannot answer is written `n/a` **with a reason**, never omitted.
>    - **Report honestly.** If a feature compiles but nothing calls it, say so. If you did not run the game, write `build only`. The log is the project's audit trail; an optimistic entry is worse than no entry because it stops anyone from looking again.
>    - **Log conflicts are resolved by union, never by choosing a side.** `agent_history.log` is append-only history shared across branches. When it conflicts during a merge, keep **every** entry from both sides sorted by timestamp. Discarding one side destroys sessions permanently — an August 2026 three-way fork orphaned 8 entries this way.
>
> 4. **Plan Deviations**:
>    - If you plan to deviate from the approved implementation plan or do what is not in the plan, you **MUST** inform the user and discuss/obtain confirmation first.
>
> 5. **Strict C++ & OOP Design**:
>    - The codebase must be written in **C++17**.
>    - Adhere strictly to Object-Oriented Programming (OOP) principles: encapsulation, inheritance, polymorphism, and abstraction.
>    - Emphasize clean software architecture and system design to ensure long-term maintainability.
>    - Avoid global state (unless using the Singleton pattern carefully) and spaghetti code.
>    - **Encapsulation & Getters/Setters**: Be extremely careful with getters/setters. Avoid unnecessary, trivial getters/setters that expose internal states or raw pointers (e.g., exposing window pointers or manager vectors directly, which violates encapsulation). Keep variables private/protected and prefer high-level action-oriented methods.
>
>
> 6. **Read SPEC.md Before Implementing**:
>    - Always check `SPEC.md` for exact values, schemas, and behavioral rules before writing code.
>    - Do NOT guess physics constants, JSON schemas, or enemy behaviors — they are all defined in the spec.
>
> 7. **Version Control Safety — Git Fingerprint Cross-Reference**:
>    - When dealing with **any** version control operation (`reset`, `rebase`, `checkout`, `clean`, `merge`), agents **MUST**:
>      1. Record the current `HEAD` commit hash **before** the operation in the log.
>      2. Record the resulting `HEAD` commit hash **after** the operation in the log.
>      3. Before running destructive git commands (`git reset --hard`, `git clean -fd`, `git checkout -- .`), **check for uncommitted/untracked work** via `git status` and **explicitly warn the user** if any files would be lost. **Do NOT proceed without user confirmation.**
>      4. Cross-reference the `Git Fingerprint` fields in `agent_history.log` to verify that the codebase is at the expected state before making changes.
>      5. **Never discard uncommitted work to unblock a git operation.** If a merge, pull or checkout is blocked by local changes, the default is to **commit them first** — committing is reversible, discarding is not. Only `git checkout -- <file>` a file that is machine-generated (e.g. `imgui.ini`), and back it up outside the repo first.
>    - **Rationale**: A previous agent session ran `git reset --hard && git clean -fd` and permanently destroyed untracked files including the Week 8 progress report (`docs/Group52_08/`). A later session found ~3,100 lines of uncommitted Member A work (three sub-level maps, `tools/`, the Week 10 report) sitting in the working tree with no commit — one careless `git clean` from being lost. This rule prevents both.
>
> 8. **Fetch Before You Read the Repository**:
>    - Before **any** task whose output describes repository state — weekly reports, audits, code review, progress summaries, branch analysis, "what is the status of X" — you **MUST** run `git fetch --all` first, and record `Fetched Remotes: yes` in the log entry.
>    - A local branch is **not** evidence of project state. Check `git rev-list --left-right --count dev...origin/dev` before drawing any conclusion from what you read on disk.
>    - When reporting on a feature, check whether the relevant work exists on an **unmerged branch** before declaring it missing.
>    - **Rationale**: An August 2026 audit was written against a local `dev` that was 9 commits stale. It declared the entire audio system missing and Phase 6 falsely marked complete. The audio system was fully implemented and merged on `origin/dev`. The finding had to be publicly retracted on issue #11.
>
> 9. **"Complete" Means Reachable and Observed**:
>    - Do **NOT** tick a checkbox in [TASKS.md](TASKS.md) or [TASK_DIVISION.md](TASK_DIVISION.md) on the basis that a file exists and compiles.
>    - A task is complete only when: (a) the code is called from a path reachable from `main()`, **not** solely from a `verify_*` harness; and (b) you have observed it working — a `ctest` case or an actual run of the game.
>    - If you implement something but do not wire it, say so explicitly in the log (`Reachable From Main: no — harness only`) and leave the checkbox unticked.
>    - **Rationale**: `verify_*` harnesses prove a class works in isolation. They do not prove the game ever constructs it. Six subsystems shipped "complete" and inert on this basis.
>
> 10. **Run the Game**:
>    - Building is not verifying. Before reporting gameplay work complete, launch the executable and confirm the change, then record `Verified By: ran the game`.
>    - Across 208 logged sessions this project wrote 86 test harnesses and recorded 4 playtests. A six-second run after the August 2026 integration immediately surfaced a broken menu-music path that had been silently failing at startup.

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
> Every time you finish a task, append to `logs/agent_history.log` in this format.
> **Every field is required.** Write `n/a` with a reason rather than omitting a line.
> ```text
> [YYYY-MM-DD HH:MM:SS] Branch: <branch_name>
> Git Fingerprint: <short_hash> (before) → <short_hash> (after)
> Fetched Remotes: <yes, HH:MM | no — reason>
> Prompt: <brief prompt summary>
> Files Modified: <list of files>
> Reachable From Main: <yes | no — harness only | n/a — not runtime code>
> Verified By: <ctest <case> | ran the game | build only | not verified>
> Summary of Changes: <brief bulleted list>
> ---
> ```
>
> **Field semantics — read these, they are not decorative:**
>
> | Field | Meaning |
> | :--- | :--- |
> | `Git Fingerprint` | HEAD at session start and end. Lets any later agent detect unexpected rewinding or data loss. If you were told not to commit, write `<hash> (after, no new commits per user directive)`. |
> | `Fetched Remotes` | Whether you ran `git fetch --all` **before** reading or reporting on repository state. See Directive 8. |
> | `Reachable From Main` | Whether the code you touched is actually called from a path starting at `main()`. A `verify_*` harness constructing a class does **NOT** count — answer `no — harness only`. |
> | `Verified By` | How you know it works. `build only` is an honest and acceptable answer; claiming more than you did is not. |
>
> **Why `Reachable From Main` exists**: an August 2026 audit found six completed
> subsystems (Minimap, ParticleSystem, AnimationManager, SpriteColorFilter,
> SpriteTransformAnim, EntityDeathEffect) that compiled, passed their harnesses,
> and were marked complete in `TASK_DIVISION.md` — while never being constructed
> by the running game. This field makes that failure visible at the time it happens
> instead of two months later.

---

## 📝 Weekly Report Writing Rules & Policy

Agents must follow these rules when writing weekly reports:
1. **Timing**: Generate the report every Saturday before 23:59.
2. **Location**: Write reports to `docs/Group52_XX/52.md`, where `XX` is the zero-padded week number.
3. **Format**: Follow the template of prior weeks. It must contain:
   - General Information (dates, repo URL).
   - Detailed tasks completed this week, separated by member and branch, with clickable file links.
   - AI Usage Declaration.
   - Tasks Planned for Next Week.
   - Issues & Resolutions (technically detailed problem and resolution).
4. **Rules Reference**: The full report writing rules are stored in [docs/REPORT_RULES.md](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/docs/REPORT_RULES.md).

