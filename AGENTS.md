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

---

<!-- AGENTHUB:L3:BEGIN — generated, do not edit by hand -->
<!-- Synced by AgentHub 3-Layer Memory Engine at 2026-08-21T14:12:57.798Z -->

## 🥾 BOOT DIGEST — read this first (Tier 0)

> Flash-tier / small-context agents: hold these one-liners and the pointer table; open a pointed file only when the task needs it; do NOT read past the Tier 2 marker.

- **[g-rule-1]** Strict Git Branch Isolation: All feature and bugfix work happens on a branch created off the project's designated development branch (dev...
- **[g-rule-2]** Defensive Implementation & Zero Superficial Fixes: NEVER mask errors by returning dummy fallbacks, commenting out broken assertions, or s...
- **[g-rule-3]** Context-Efficient Subagent Execution: Delegate long-running research or deep search tasks to subagents with isolated context windows.
- **[g-rule-4]** Empirical Log Verification & Audit Trails: Never declare a bug fixed or feature complete without running test/build verification commands...
- **[g-rule-5]** AI Usage Logging & Declaration: Before concluding any task, append an entry to logs/agent_history.log in the format: [YYYY-MM-DD HH:MM:SS...
- **[g-rule-6]** Plan Adherence & Deviation Approval: The project's designated plan file (implementation_plan.md, SPEC.md, or TASKS.md) is the source of t...
- **[g-rule-7]** Human-Owned Integration (No Auto-Merge): Agents implement on their task branch, verify compilation, push the branch to origin, and stop.
- **[g-rule-8]** Destructive VCS Operation Guard: Before any reset, rebase, checkout that discards changes, clean, stash drop, or merge: run git status an...
- **[g-rule-9]** Targeted Edits, Never Whole-File Overwrites: Apply changes as targeted replacements that preserve surrounding human-authored content.
- **[g-rule-10]** Fetch Before You Read the Repository: Before ANY task whose output describes repository state - audits, weekly reports, code review, prog...
- **[g-rule-11]** "Complete" Means Reachable and Observed: Never mark a task complete because a file exists and compiles.
- **[g-rule-12]** Commit Per Completed Unit: Commit after completing each subtask, not in one batch at the end.
- **[g-rule-13]** Automated Verification Gate (CI/CD): Every project containing buildable or testable code carries a CI workflow that builds it and runs it...
- **[g-rule-14]** Human-Facing Docs Are Readable HTML - Rendered Views vs Authored Artifacts: Anything a human reads gets a self-contained local HTML form ...
- **[g-rule-15]** One Current Doc - Legacy Goes to the Archive: Each topic has exactly ONE current document.
- **[g-rule-16]** Tiered Boot Protocol: Load project context in tiers, sized to your model, and stop at the tier you can hold.
- **[g-rule-17]** Cross-File Contracts Get a Parity Test: When one fact must exist in two or more places - an i18n key emitted from Python, an action id, a...
- **[g-rule-18]** Suggest Rules Upstream, Do Not Fork Them: When you notice something rule-worthy while working in a project - a repeated manual step, a fa...
- **[g-rule-19]** Build Outputs Are Built, Never Committed: Any artifact a build step can produce - compiled binaries and sidecars, generated PDFs, __pycac...
- **[g-rule-20]** How to Write a Document: Every document states, in its first screen, what it is and who it is for; a reader must never scroll to find out.
- **[g-rule-21]** Every Substantial Piece of Work Gets a Learning Record: Whenever you build a feature, module, algorithm, or subsystem, export a self-cont...
- **[g-rule-22]** Reference Docs Are Derived, Never Hand-Synced: Any document that restates facts the code already contains - endpoint lists, module invent...
- **[g-rule-23]** Sensitive Content Never Enters Git: g-rule-19 keeps build OUTPUTS out of git; this covers sensitive INPUTS - identity documents, keys, cr...
- **[g-rule-24]** Comments Carry Why, Not What: A comment that restates what the code does is hand-synced duplication of the code - g-rule-22's failure at ...
- **[g-rule-25]** Join Keys Need Immutability or an Alias Chain: An identifier used as a JOIN KEY between systems - a project slug, a bundle id, a foreign ...
- **[sm-rule-1]** Member Identity Resolution (Project-Specific Mapping): In THIS project: branches prefixed A/ are Member A (Nguyen Dinh Minh Huy - Engine ...
- **[sm-rule-2]** Strict OOP Design Patterns Enforcing: Enforce 10+ Design Patterns: Factory (EntityFactory), Singleton, State (GameStateManager, IPlayerSt...
- **[sm-rule-3]** Physics & Coyote Jump Timestep Rules: Timestep 1/60s fixed with interpolated rendering.
- **[sm-rule-4]** C++17 Standard (Project Constraint): The codebase is written in C++17, not C++20.
- **[sm-rule-5]** SPEC.md Is the Behavioral Source of Truth: Check SPEC.md for exact values, schemas, and behavioral rules before writing gameplay or physi...
- **[sm-rule-6]** Weekly Report Policy: Weekly reports go to docs/Group52_XX/52.md every Saturday before 23:59, following the prior week's template (full s...
- **[sm-rule-7]** imgui.ini Is The Only Discardable File: When a git operation is blocked by local changes, imgui.ini is machine-generated and is the only ...

### Pointers — open on demand, never inline

- Plan / scope source of truth: `TASKS.md`
- `SPEC.md` — Behavioral source of truth for values and schemas - see sm-rule-5
- `SuperMarioGame/src/` — Game source (nested one level below the repo root)
- `SuperMarioGame/CMakeLists.txt` — enable_testing() + add_test() register the ctest regression suite
- `logs/agent_history.log` — Append-only session log - union on merge conflicts, see g-rule-5
- `.github/workflows/ci.yml` — Build + regression suite on push to main/dev/A-**/B-** - see g-rule-13
- `.member_profile.json` — Per-member private instructions - still git-tracked, see open threads
- `SuperMarioGame/third_party/nn/` — Vendored NeuralNetwork library - the ONLY C++20 target here, see sm-rule-4
- `docs/REPORT_RULES.md` — Weekly report template and quality bar - source of g-rule-20

### Reading these docs as HTML

Markdown working docs (README, SPEC, TASKS, plan files) render to self-contained HTML with:

```bash
python3 ~/Documents/AgentHub/scripts/render_docs.py super-mario-game
```

Output lands in `docs/rendered/` and is GITIGNORED. The markdown stays the only place content is edited - never hand-edit a rendered view, and never commit one (g-rule-14 class A, g-rule-19). Authored artifacts are the opposite: learning records in `docs/learning/` and reports in `reports/` ARE committed (class B).

### Suggesting a rule back to AgentHub

Noticed something rule-worthy? Do NOT edit this generated block — append to `.agenthub/suggestions.json` in this project instead:

```json
{"suggestions":[{
  "title":"...", "rationale":"the rule, and the failure it prevents",
  "proposedLayer":"L2" | "L3",        // set it explicitly; omitting it defaults to L2
  "evidence":"files, commits, incidents - survives in Tier 2",
  "suggestedBy":"who/when",
  "requestedDisposition":"defer",     // optional. "defer" is honoured on import;
  "deferRationale":"why not now"      // promote/amend stay human decisions.
}]}
```

The hub collects it (`node scripts/collect.js`) and a human promotes it. Directives OUTSIDE these markers are hand-authored and yours to edit freely.

---

## 📖 Full rule text (Tier 2 — large-context models)

## 🌐 Layer 3 — Global Universal Rules

### [Git & Branching] Strict Git Branch Isolation
All feature and bugfix work happens on a branch created off the project's designated development branch (dev where present, otherwise main). Never work directly on the integration branch. Verify the build compiles before pushing. Integration itself is governed by g-rule-7.

### [Architecture & Quality] Defensive Implementation & Zero Superficial Fixes
NEVER mask errors by returning dummy fallbacks, commenting out broken assertions, or swallowing exceptions. Root cause analysis must precede all code changes.

### [AI Subagent Delegation] Context-Efficient Subagent Execution
Delegate long-running research or deep search tasks to subagents with isolated context windows. Do not poll in a loop; wait for asynchronous task completion.

### [Documentation & Auditing] Empirical Log Verification & Audit Trails
Never declare a bug fixed or feature complete without running test/build verification commands and reading un-truncated log outputs.

### [AI Governance] AI Usage Logging & Declaration
Before concluding any task, append an entry to logs/agent_history.log in the format: [YYYY-MM-DD HH:MM:SS] Branch: <branch> / Prompt: <summary> / Files Modified: <list> / Summary of Changes: <bullets> / Git Fingerprint: <commit before> -> <after>. EVERY field the project's template defines is mandatory: a field you cannot answer is written 'n/a' WITH a reason, never omitted. Report honestly - if code compiles but nothing calls it, say so; if you did not run the program, write 'build only'. An optimistic entry is worse than no entry because it stops anyone looking again. The log is append-only shared history: when it CONFLICTS during a merge, resolve by UNION - keep every entry from both sides sorted by timestamp, never choose a side. Maintain prompts.md where the course requires an AI Usage Declaration.

### [Planning & Scope] Plan Adherence & Deviation Approval
The project's designated plan file (implementation_plan.md, SPEC.md, or TASKS.md) is the source of truth for scope. Read it before writing code. If you intend to deviate from the approved plan, or to do work not in it, inform the user and obtain confirmation first. Update task checkboxes as work completes.

### [Git & Branching] Human-Owned Integration (No Auto-Merge)
Agents implement on their task branch, verify compilation, push the branch to origin, and stop. Never merge into dev, never commit directly to main, never open and self-merge a pull request. Code review and integration are the user's decisions.

### [Git & Branching] Destructive VCS Operation Guard
Before any reset, rebase, checkout that discards changes, clean, stash drop, or merge: run git status and git log to capture the current state, state plainly what will be lost, and obtain explicit confirmation - do NOT proceed without it. Record HEAD before and after the operation in the log. NEVER discard uncommitted work to unblock a git operation: if a merge, pull or checkout is blocked by local changes, the default is to COMMIT them first - committing is reversible, discarding is not. Only 'git checkout -- <file>' a file that is machine-generated, and back it up outside the repo first. Never combine these with --force or --hard while uncommitted work exists.

### [Architecture & Quality] Targeted Edits, Never Whole-File Overwrites
Apply changes as targeted replacements that preserve surrounding human-authored content. Never regenerate an entire existing file to make a small change, and never write over a file you have not read. This applies to generated artifacts and rule files as much as to source code.

### [Git & Branching] Fetch Before You Read the Repository
Before ANY task whose output describes repository state - audits, weekly reports, code review, progress summaries, branch analysis, 'what is the status of X' - run 'git fetch --all' first and record it in the log. A local branch is NOT evidence of project state: check 'git rev-list --left-right --count <branch>...origin/<branch>' before drawing conclusions from what is on disk, and check whether work exists on an unmerged branch before declaring it missing. This applies to a repository's VISIBILITY and settings as much as to its branches: 'the repo is public' is repository state, it changes without touching a single commit, and a local clone cannot tell you. Check it (gh api repos/<owner>/<name>) before describing exposure - and check forkCount too, because GitHub keeps a formerly-public repo's commits reachable by SHA through the fork network, so making a repo private only fully closes one that was never forked. Evidence: this hub asserted twice, as an urgent security finding, that secrets remained retrievable from a public repo that had been private for a day.

### [Documentation & Auditing] "Complete" Means Reachable and Observed
Never mark a task complete because a file exists and compiles. A task is complete only when (a) the code is reachable from the program's real entry point, not solely from a verify_*/test harness, and (b) you have observed it working - a passing test case or an actual run of the program. Building is not verifying. If you implemented something but did not wire it, say so explicitly and leave the checkbox unticked.

### [Git & Branching] Commit Per Completed Unit
Commit after completing each subtask, not in one batch at the end. Use clear, traceable, conventional messages (e.g. 'feat: implement AABB collision detection', 'fix: resolve jump gravity bug') so history can be followed without reading diffs.

### [Verification & CI] Automated Verification Gate (CI/CD)
Every project containing buildable or testable code carries a CI workflow that builds it and runs its tests automatically on push and pull request to the integration branches. A rule only a human remembers to apply is not enforced - CI is the mechanism that makes g-rule-11 ('complete means reachable and observed') checkable by something other than good intentions. CI must be HERMETIC: pin dependencies, and never let a test assert against a developer's local paths, mounted drives, or machine state - point the base directory at an empty scratch dir instead. Where the project ships artifacts, a release workflow builds them too. Do not mark CI optional because a project is coursework: the August 2026 SuperMarioGame audit found three critical defects and six inert subsystems that survived precisely because nothing ran automatically.

### [Documentation & Auditing] Human-Facing Docs Are Readable HTML - Rendered Views vs Authored Artifacts
Anything a human reads gets a self-contained local HTML form (inline CSS, opens from disk, no network, printable). There are TWO classes and they are handled OPPOSITELY - the test is: does a markdown file already hold the truth?

(A) RENDERED VIEWS - yes, a .md is the source of truth (SPEC.md, TASKS.md, the plan file, README, TASK_DIVISION). The markdown stays the ONLY place the content is edited; the HTML is a generated VIEW produced by 'python3 scripts/render_docs.py <project-id>' into docs/rendered/, is GITIGNORED, and carries a do-not-edit banner naming its source. Never commit these and never hand-edit them: a committed render is a second copy that is stale within hours, which is exactly g-rule-22's failure, and per g-rule-19 a derived artifact is built, not tracked. Regenerate instead of updating.

(B) AUTHORED ARTIFACTS - no markdown source holds the truth; the HTML IS the document (learning records per g-rule-21, audits, reports, guides). These are authored once, live in the project (docs/learning/, reports/), and ARE committed. Where the content derives from data, generate it with a script so it cannot drift, and never hand-edit the generated file.

Both classes must state what they are and who they are for in the first screen (g-rule-20).

### [Documentation & Auditing] One Current Doc - Legacy Goes to the Archive
Each topic has exactly ONE current document. When a doc, plan, or artifact is superseded, MOVE it to the project's designated archive directory in the same change that supersedes it, prefixed with its supersession date. The archive path is recorded per project as archiveDir (default docs/archive/); Lecturing_Source uses archive/legacy_planning/ - honour the project's choice rather than inventing a second archive. Add a README there saying when and why things were moved. Never delete it, and never leave it in place looking authoritative: a stale doc that still looks current is how contradictions survive for months. Scratch and test artifacts either move to the archive or get gitignored - the repo root is not a scratchpad.

### [Context & Boot] Tiered Boot Protocol
Load project context in tiers, sized to your model, and stop at the tier you can hold. TIER 0 (everyone, always): the BOOT DIGEST at the top of the AgentHub block in AGENTS.md - one line per rule - plus the pointer table. TIER 1 (on demand): open ONLY the pointed file that matches the current task (constants for physics work, report rules for reports, spec for gameplay values). TIER 2 (large-context models only): the full rule text and reference docs. Small or flash-tier agents MUST NOT inline the whole rulebook - holding 10 rules reliably beats holding 40 badly. Authoring side of the same rule: constants, tables, and external links live in ONE pointed file each, never duplicated inline in AGENTS.md; a value that exists in two places is already wrong in one of them.

### [Verification & CI] Cross-File Contracts Get a Parity Test
When one fact must exist in two or more places - an i18n key emitted from Python, an action id, an enum member, an endpoint string, a JSON field a JS module reads, a default model name, a documented screen - the SAME commit adds or extends the parity test that fails when the copies disagree. Follow the project's existing patterns (AST extraction on the Python side, node execution or source scan on the JS side). 'The agent should remember the other place' is not a mechanism; a failing test that names the other place is. g-rule-16 says a value in two places is already wrong in one of them - this is what catches it. Evidence: in Lecturing_Source, every contract WITH a parity test (i18n key parity, help-guide coverage, prompt/proxy sync) has never drifted; every contract without one has - the _Answers.txt grammar reimplemented 7 times, the event vocabulary across 8 surfaces, default model names in 5 places holding 3 different values, and a chatbot tool registry in 5 places with one decorated tool already silently unreachable. A contract also breaks when one side SILENTLY DISCARDS what the other sends: GenerateRequest had no critics_model field, so Pydantic dropped the value the UI and the chatbot both sent and the critics agent always ran the hard-coded default - the feature looked wired and never was (g-rule-11). Assert the value arrives, not just that it was sent. AND THE TEST MUST FAIL WHEN THE VALUE IS MISSING, not only when copies differ. A parity test that locates its target by a fragile path and returns None when it cannot find it PASSES VACUOUSLY - it reports success precisely when the thing it guards has vanished, which is worse than no test because it buys false confidence. Search for the key and fail if absent; assert non-empty before asserting equal. Evidence: an identity parity test addressed tauri.conf.json by a v1-schema path and returned None, so it would have gone green after a v2 upgrade removed that path - the same family as the critics_model field Pydantic dropped silently.

### [Communication] Suggest Rules Upstream, Do Not Fork Them
When you notice something rule-worthy while working in a project - a repeated manual step, a failure that a rule would have prevented, a convention the team keeps re-deciding - APPEND it to .agenthub/suggestions.json in that project (schema: id, title, rationale, proposedLayer L2|L3, evidence, status pending) rather than silently editing the AGENTS.md managed block, which is generated and will be overwritten. Project-specific directives outside the markers are still yours to edit freely. AgentHub collects suggestions with 'node scripts/collect.js'; a human promotes them. This is the upstream half of the loop: rules flow hub -> project by distribution, and project -> hub by suggestion.

### [Architecture & Quality] Build Outputs Are Built, Never Committed
Any artifact a build step can produce - compiled binaries and sidecars, generated PDFs, __pycache__, bundled assets, dumped git logs - is gitignored and produced by CI, never committed. A tracked build output is a merge conflict waiting to happen and makes the diff lie about what changed. Evidence: Lecturing_Source stopped tracking its compiled sidecar in 8db4c31; DataStructureVisualizer tracks README.pdf, git_history.txt and commits_summary.txt as of 2026-08-21 (verified against both the working tree and the remote root) - all three are reproducible from source or from git itself, which is what makes them build outputs rather than content.

### [Documentation & Auditing] How to Write a Document
Every document states, in its first screen, what it is and who it is for; a reader must never scroll to find out. Then: (1) SUMMARY BEFORE DETAIL - the conclusion, current state, or quick-context block comes first, the reasoning after (same tiering as g-rule-16). (2) PRECISION OVER VAGUENESS - never 'fixed character movement'; write 'moved horizontal friction and run-speed clamping into PhysicsEngine::update()'. A sentence a reader cannot act on is not documentation. (3) LINK, DO NOT COPY - reference the file, commit, or other doc with a clickable path instead of restating its content; duplicated prose is g-rule-16's two-places problem in text form. (4) VERIFY BEFORE WRITING - check commit SHAs, filenames, and command output against the real tree; never invent a detail, and say 'not verified' rather than implying you checked. (5) RATIONALE FOR EVERY RULE OR DECISION - record the failure it prevents, so a later reader can tell whether it still applies. (6) A DOC THAT DESCRIBES A PROCEDURE ends with the exact commands to run it. Project-specific templates (report structures, runbooks) live in the project and override these defaults where they conflict.

### [Documentation & Auditing] Every Substantial Piece of Work Gets a Learning Record
Whenever you build a feature, module, algorithm, or subsystem, export a self-contained local HTML LEARNING RECORD to docs/learning/<topic>.html (g-rule-14: inline CSS, opens from disk, no network). It is written for the student who must revise this project months later - or explain it in a viva - not for the person who just wrote it. It opens with PREREQUISITES - what a reader must already know to follow it, and where to go learn that - so someone can tell in one screen whether they are ready. Required sections: (1) WHAT & WHY - the problem, the approach chosen, and a COMPARISON TABLE of the alternatives rejected with the reason each lost; a table is revisable in a way a paragraph is not, and the rejected options are the part you will have forgotten first; (2) TECH STACK - languages, libraries, versions, and why each was chosen; (3) CS & MATH FOUNDATIONS - the actual theory the work rests on, with real formulas and complexity analysis, not hand-waving (e.g. the gradient derivation, the O(V log V) argument, the fixed-timestep integration maths); (4) DIAGRAMS - at least one, and TEXT-AUTHORED (Mermaid source, or ASCII in a pre block), never a pasted raster image: a PNG cannot be diffed, drifts silently, and hides which revision it depicts. Include the diagram SOURCE in the record so it survives offline and can be regenerated. Draw what the prose cannot hold - the architecture and who calls whom, the data flow end to end, the state machine and its illegal transitions, or the shape of the data structure. Label the edges, not just the boxes: an unlabelled arrow is a guess. (5) HOW IT WORKS - this section MUST CONTAIN THE ACTUAL CODE, not a description of it. Embed the load-bearing excerpts verbatim - the function that carries the idea, the loop that does the work, the invariant check - each captioned with its real location (path, line range, and the commit the record is stamped against) so a reader can open it and diff it. Then ANNOTATE: walk the reader through what each significant part does AND why it is written that way, naming the real identifiers, real constants and real complexity. Quote the smallest complete unit that carries the idea; if an excerpt is too long to quote, that is a signal the code wants decomposing, not that the excerpt should be summarised. Prose like 'the system processes the input and stores the result' is a failure of this section - it would be true of almost any program and teaches nothing. On the copy-drift tension: a quoted excerpt IS a copy of code, which g-rule-22 would normally forbid, and it is permitted here for one reason only - the record is a TEACHING SNAPSHOT pinned to a named commit, not a live reference. That is what makes it honest, so the location caption and the commit stamp are mandatory, not decoration, and the excerpt must be re-verified whenever the code it teaches changes. EXTRACT EXCERPTS BY SCRIPT, and have the script ASSERT ITS ANCHORS: check that the first and last line of the range still match the content you expect, so a shifted line number fails loudly instead of silently quoting the wrong code. Hand-pasted excerpts with hand-typed line numbers are a copy with no integrity check at all. Evidence: a retrofit of five records extracted 15 excerpts by script, and the anchor assertions caught two wrong line ranges BEFORE anything was written. AN EXCERPT'S COMMIT AND THE RECORD'S STAMP ARE TWO DIFFERENT ANCHORS and may legitimately differ: caption an excerpt against the commit whose code it actually shows, which is the record's existing stamp where that code is unchanged and a newer commit where it moved. Forcing them equal makes the record either falsely claim freshness for code it has not re-read, or falsely claim staleness for code that never changed; (6) WORKED TRACE - take ONE concrete input and follow it all the way through with REAL VALUES, not placeholders: the actual graph, the actual file, the actual request, and what each step produces. State the answer at the end and why it is right. This is the section that most reliably converts 'I read it' into 'I can explain it', and the one a reader will return to before a viva. (7) BUILD & RUN - the exact commands; (8) OUTCOME & EVIDENCE - what was measured, benchmark numbers, test results; (9) PITFALLS, MISCONCEPTIONS & DEBUGGING - three distinct things: what went WRONG while building it and the root cause, so it is not rediscovered; what a learner is likely to BELIEVE WRONGLY about this topic (the plausible-but-false idea, stated and then corrected); and how it FAILS AT RUNTIME - the symptom you would actually observe, and the first three things you would check; (10) REVISION AIDS - key takeaways ('if you remember only five things'), self-check questions whose answers prove understanding rather than recall, a glossary of project-specific terms, EXTENSION EXERCISES ('if you wanted to add X, where would you start, and what would you have to change') since being able to extend a thing is the real test of understanding it, and WHERE THIS SITS IN THE COURSE - the module and topic it maps to, so revision can be aimed; (11) REFERENCES - clickable links to files, commits, and external reading; (12) CHANGELOG plus a 'verified against commit <sha>' stamp. UPDATE IT IN THE SAME CHANGE that alters the thing it describes - a stale learning record teaches the wrong thing, which is worse than none (g-rule-15). Generate the skeleton with 'node scripts/new_learning_doc.js' rather than starting from a blank file. NOTE: weekly reports are a DIFFERENT artifact governed by each project's own report rules (e.g. SuperMarioGame docs/REPORT_RULES.md); a learning record is per-topic and permanent, a weekly report is per-week and historical. Do not merge the two. A SECTION YOU CANNOT FILL HONESTLY should say 'not applicable, because ...' and stop. Padding a section to look complete is worse than omitting it, because it teaches the reader that the headings are decoration - and a record whose sections are all half-filled will not be trusted for the ones that matter.

### [Documentation & Auditing] Reference Docs Are Derived, Never Hand-Synced
Any document that restates facts the code already contains - endpoint lists, module inventories, dependency graphs, state machines, counts - must be GENERATED by a script from the code (and drift-checked in CI) or must not exist. Hand-maintained prose is reserved for what code cannot state: intent, invariants, and the history of why. A hand-synced restatement of code is already wrong or soon will be, and a stale one is worse than none because it looks authoritative. If a generated artifact contradicts prose, the generated artifact wins and the prose gets fixed. THIS APPLIES TO THE RULESET ITSELF. A rule's evidence clause states facts about the world - a repository's visibility, a commit hash, a count, whether a secret was rotated - and those rot exactly like any other hand-maintained restatement, except worse: a stale rule is distributed to every project and read as authority. So re-verify a rule's factual claims before citing them, and PREFER HISTORICAL CLAIMS OVER CURRENT-STATE CLAIMS when writing evidence: 'LICENSE.key was committed and removed in 542a770' is permanently true and needs no maintenance, whereas 'the repository is public' is current state that can flip overnight. Where a current-state claim is unavoidable, date it and name the check that confirms it (e.g. 'private with 0 forks, verified 2026-08-21 via gh api'). Evidence: this hub distributed an evidence clause asserting secrets remained retrievable from a public repository that had been private for a day; the historical half of that same clause was correct all along.

### [Security & Data Handling] Sensitive Content Never Enters Git
g-rule-19 keeps build OUTPUTS out of git; this covers sensitive INPUTS - identity documents, keys, credentials, client names, personal data. The two differ in KIND, not degree: a committed build output is noise, while a passport scan or licence key committed once stays retrievable from history forever, including after deletion. Therefore: (1) deny sensitive types repo-wide BEFORE the first commit - history is immutable, so there is no second chance; (2) enforce with a CI job that runs INDEPENDENTLY of the build, so a broken build cannot mask a leak; (3) prefer a LOCATION INVARIANT in code over a pattern list - store sensitive data outside every working tree and raise if the resolved path sits inside one, because a pattern list must stay correct as the project grows while an invariant cannot silently rot; (4) default to a PRIVATE repository whenever the blast radius of one mistake is irreversible disclosure - private is reversible, a leak is not. If something sensitive was already committed, treat it as disclosed: rotate the secret and say so. Deleting the file does not undo publication. Evidence: Lecturing_Source committed LICENSE.key and client_name.txt and removed them in 542a770 - deletion did not unpublish them, they stayed reachable from ancestor commits, which is why the remedy was to make the repository private (private with 0 forks, verified 2026-08-21 via gh api - privating only fully closes a repo that was never forked, since GitHub keeps formerly-public commits reachable by SHA through the fork network) AND to rotate the dev-key salt in 1bdb394 rather than merely delete it, guarded now by tests/test_data_safety.py::TestNoHardcodedLicenceSecrets. Rotate-and-close beats scrub: a history rewrite would invalidate every clone to remove data already behind auth and cryptographically inert. Document Vault answered the same risk structurally - ~/.docvault outside any working tree plus VaultLocationError - rather than trusting a pattern list.

### [Architecture & Quality] Comments Carry Why, Not What
A comment that restates what the code does is hand-synced duplication of the code - g-rule-22's failure at line level. It rots the moment the code changes and then actively misleads, so it is worse than no comment. Do not write them, and delete them when found. If a comment is needed to explain WHAT the code does, rename or restructure first. Comment these instead: (1) WHY this approach rather than the obvious alternative; (2) NON-OBVIOUS INVARIANTS and the failure they prevent - cite the incident where there was one; (3) CONSTRAINTS FROM OUTSIDE THE FILE - an API quirk, a hardware limit, a spec clause, another module's expectation; (4) DELIBERATE CHOICES a future reader would otherwise 'fix' and break; (5) UNITS, RANGES, COORDINATE SYSTEMS, OWNERSHIP AND LIFETIME wherever they are ambiguous. Name the rule or incident when one applies. Every public function, class, or module gets a docstring stating its CONTRACT: what it guarantees, what it raises, and what it deliberately does NOT do. A comment describing a workaround must say what would allow its removal. Keep the comment adjacent to what it describes and update it in the same change (g-rule-15 at line level): a stale comment is a stale doc with a shorter fuse.

ENFORCE THE MECHANICAL CLAUSE. Most of this rule is judgement - why-not-what cannot be linted - but the docstring-on-public-API clause CAN be, and g-rule-13's argument applies: a rule only a human remembers is not enforced. Turn on your linter's docstring rules (ruff's D / pydocstyle) scoped to SOURCE ONLY, and let existing CI carry it - no new job required. Two calibrations keep the enforcement honest to this rule's intent rather than fighting it: exempt tests (test names are the documentation there, and forcing docstrings on them produces exactly the what-restating noise this rule bans), and disable imperative-mood checks (D401) where noun-phrase contract lines read better - 'The stored original for a hash, or None' states a contract more clearly than a forced imperative. Record the reason for each calibration in the config beside it, per this rule. Evidence: Document Vault enabled it in the same session the rule arrived; the first run found 13 real gaps (9 public functions, 2 classes, 2 methods), independently matching a manual audit's finding and extending it, and those gaps are now impossible to reintroduce.

### [Architecture & Quality] Join Keys Need Immutability or an Alias Chain
An identifier used as a JOIN KEY between systems - a project slug, a bundle id, a foreign key in stored data, a directory named after an entity - is load-bearing in a way its name does not advertise. This is the third member of g-rule-17's family and the quietest: the copies do not disagree and nothing is discarded; the identity underneath both simply changes, records stop matching, and resolved work silently resurrects or vanishes. Pick one of two remedies deliberately, never by accident: (1) IMMUTABILITY - freeze the key, comment why (g-rule-24), pin it with a test; or (2) AN ALIAS CHAIN - permit the rename but record prior names on the record and match against them everywhere the key is used for lookup. Choose by which side is cheaper to change: FREEZE when copies are already in the field and beyond your reach; ALIAS when the rename is already in flight. If the key can be renamed at all, a RENAME TEST belongs beside the parity test - repeat the lookup under the new name and assert that nothing re-imports and nothing disappears. Evidence, one lesson from opposite directions: AgentHub's collect.js fingerprinted suggestions on the project id, so centralising a slug re-imported two already-promoted suggestions as new pending duplicates (remedy: aliases, proven by two consecutive collects returning zero new); Lecturing Grading Tools' com.lecturing.grading joins the launcher to the licence file, so renaming it would have split the app-data directory and dropped every installed beta to community tier (remedy: freeze plus test).

## 📁 Layer 2 — Project Domain Rules (SuperMarioGame (CS202-Cpp))

**Category**: C++ Game Engine & OOP
**Tech Stack**: C++17, SFML 3.0.2, ImGui-SFML, CMake, CTest, 10+ Design Patterns
**Repository**: https://github.com/ndmhuy/SuperMarioGame

### [CRITICAL] Member Identity Resolution (Project-Specific Mapping)
In THIS project: branches prefixed A/ are Member A (Nguyen Dinh Minh Huy - Engine & Infrastructure); branches prefixed B/ are Member B (Partner - Entities, Gameplay, Level Design, Save/Load). Resolve identity in this precedence: .member_profile.json, then git config user.name, then branch prefix. Do not carry this A/B mapping into any other repository - DesignPatternsGroup52 uses the opposite assignment.

### [MANDATORY] Strict OOP Design Patterns Enforcing
Enforce 10+ Design Patterns: Factory (EntityFactory), Singleton, State (GameStateManager, IPlayerState), Observer (EventBus), Strategy, Command, Decorator, Memento, Pool, Template Method.

### [STANDARD] Physics & Coyote Jump Timestep Rules
Timestep 1/60s fixed with interpolated rendering. Gravity 0.5 px/frame², Coyote Time 6 frames (100ms), Jump Buffer 6 frames.

### [CRITICAL] C++17 Standard (Project Constraint)
The codebase is written in C++17, not C++20. Do not introduce C++20-only features (concepts, ranges, coroutines, consteval) into this project.

### [MANDATORY] SPEC.md Is the Behavioral Source of Truth
Check SPEC.md for exact values, schemas, and behavioral rules before writing gameplay or physics code. Consult SFML 3.0 documentation only - 2.x tutorials are API-incompatible.

### [STANDARD] Weekly Report Policy
Weekly reports go to docs/Group52_XX/52.md every Saturday before 23:59, following the prior week's template (full spec: docs/REPORT_RULES.md). Generate them from agent_history.log entries and merged branches for the week - never from memory - and render 52.pdf plus a local HTML copy per g-rule-14. Use the supermario-weekly-report-writer skill. Structure and quality bar: docs/REPORT_RULES.md (5 mandatory sections: General Information, Tasks Completed per member, AI Usage Declaration, Tasks Planned, Issues & Resolutions); its quality guidelines are now universal as g-rule-20.

### [CRITICAL] imgui.ini Is The Only Discardable File
When a git operation is blocked by local changes, imgui.ini is machine-generated and is the only file safe to 'git checkout --' (back it up outside the repo first). Everything else gets committed, per g-rule-8. Rationale: a previous session ran 'git reset --hard && git clean -fd' and permanently destroyed the Week 8 progress report in docs/Group52_08/; a later session found ~3,100 lines of uncommitted Member A work one careless clean away from the same fate.

### 🛠 Assigned Project Skills
- `sfml-game-debugging-skill`
- `supermario-weekly-report-writer`
- `learning-record-writer`
- `doc-view-renderer`

## ⚡ Layer 1 — Working Context

> Note: `session_l1.json` referenced below lives in the **AgentHub repo**, not in this project. Nothing in this section asks you to create a local file.

No active session for this project. Live L1 state is held in the AgentHub repo (`AgentHub/data/session_l1.json` - NOT a file in this project).

### [CRITICAL] Session Identity Resolution
At session start, resolve the active member using .member_profile.json, then git config user.name, then branch prefix - in that order - and record the result and its source in session_l1.json. Read the A/B mapping from the active project's L2 rules; never assume it from another project.

### [MANDATORY] Single-Project Context Budget
Load L3 universal rules plus exactly one project's L2 block. Do not load other projects' L2 rules into the same session - cross-project rules conflict (see the inverted A/B mapping) and waste the context window.

### [STANDARD] Session Handoff
Before concluding, write taskFocus and any unresolved openThreads back to session_l1.json so the next session resumes without re-deriving state. This is L1 only - it is transient and must never be promoted into L2 or L3.

<!-- AGENTHUB:L3:END -->
