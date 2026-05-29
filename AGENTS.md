# AGENTS.md — Super Mario Game

> This file contains critical guidelines, workflows, and technical details for AI agents working on the Super Mario Game project.

## Critical Compliance Directives for AI Agents

> [!IMPORTANT]
> **MANDATORY RULES FOR AGENT EXECUTION**
> 
> 1. **Git Branching Policy**:
>    - All development process must take place on the `dev` branch.
>    - The `main` branch is reserved only for delivery/releases after major milestone changes.
>    - For each main task, create and work on a subbranch created from the `dev` branch (e.g., `feature/movement`, `feature/collision-detection`).
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
>    - The codebase must be written in **C++** (targeting C++17 or C++20).
>    - Adhere strictly to Object-Oriented Programming (OOP) principles: encapsulation, inheritance, polymorphism, and abstraction.
>    - Emphasize clean software architecture and system design to ensure long-term maintainability.
>    - Avoid global state (unless using the Singleton pattern carefully) and spaghetti code.

---

## Project Purpose

Develop a modular, extensible, and high-performance 2D Mario-style game in C++ featuring:
- Character system (Mario, Luigi, enemies) with distinct abilities and states.
- Power-ups/items system (Mushroom, Coin, Fire Flower).
- Multi-level game flow (at least 3 levels with increasing difficulty).
- Collision detection and tilemap management.
- Sound effects and music.
- Save/load game progress serialization.
- Level Editor (bonus feature).

---

## Technical Stack

- **Language**: C++17 / C++20
- **Libraries/Frameworks**: (Pending selection: e.g., SFML, SDL2, or Raylib)
- **Build System**: CMake (cross-platform configuration)
- **Design Patterns Required (Minimum 5)**:
  1. **Factory Pattern**: Spawning entities, items, and enemies.
  2. **Singleton Pattern**: Game engine instance, sound manager, or resource manager.
  3. *State Pattern* (Recommended): Character states (Small Mario, Super Mario, Fire Mario).
  4. *Observer Pattern* (Recommended): Event and UI scoring updates.
  5. *Strategy Pattern* (Recommended): Enemy AI movement behaviors.

---

## Project Structure & Architecture

```text
SuperMarioGame/
├── include/                # Header files (.h / .hpp)
│   ├── Core/               # Game loop, state managers, input handlers
│   ├── Entities/           # Mario, Luigi, Goomba, Koopa, blocks, items
│   ├── Graphics/           # Renderer wrappers, spritesheets, animations
│   ├── Physics/            # Collision detection, bounding boxes
│   └── Utils/              # File I/O, helpers, config constants
├── src/                    # Source files (.cpp)
│   ├── Core/
│   ├── Entities/
│   ├── Graphics/
│   ├── Physics/
│   └── Utils/
├── assets/                 # Textures, fonts, audio files, tilemaps
├── logs/                   # Agent and system run logs
│   └── agent_history.log   # Automated agent prompt & output log
├── CMakeLists.txt          # Build configuration
└── README.md               # Student and user documentation
```

---

## Key Modules — When to Modify What

| If you need to... | Modify / Create |
| :--- | :--- |
| **Change/Add game-wide configurations or paths** | `include/Core/Config.hpp` or similar settings file |
| **Implement/Tweak movement physics or gravity** | Physics engine or player entity controllers in `src/Entities/` |
| **Add a new design pattern or change entity spawning** | Factory class or managers in `src/Entities/` |
| **Add level loading or serialization rules** | File parser / serialization utils in `src/Utils/` |
| **Adjust graphics, UI overlays, or camera views** | Renderer, HUD, or view code in `src/Graphics/` |
| **Add sound effects or audio playbacks** | Sound manager / Singleton in `src/Core/` |

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
