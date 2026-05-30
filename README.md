# Super Mario Game

A modular, extensible, and high-performance 2D/3D Mario-style game built in C++17/20 using **SFML 3.0.2** and **ImGui** for development tools and level editors.

This project is developed for the CS202 Final Project, with an emphasis on Object-Oriented Programming (OOP) principles and Software Design Patterns.

---

## Team Members
- **Nguyễn Đình Minh Huy** (25125083)

---

## Features
- **Strict OOP Design**: Inheritance, polymorphism, encapsulation, and abstraction.
- **Design Patterns (At least 5)**:
  - **Factory Pattern**: Dynamic spawning of blocks, enemies, items, and players.
  - **Singleton Pattern**: Game engine managers, assets caches, and sound controllers.
  - **State Pattern**: Character states (Small Mario, Super Mario, Fire Mario).
  - **Observer Pattern**: Event processing and scoring HUD updates.
  - **Strategy Pattern**: Different enemy AI movement patterns.
- **Multi-Level Flow**: 3 levels of increasing difficulty.
- **Physics & Collisions**: Character movements, jumping physics, gravity tuning, and AABB collision resolution.
- **Save/Load Game**: Serialization of game states.
- **Level Editor**: Interactive tool to build and save custom map templates.

---

## Project Structure
```text
SuperMarioGame/              # Git root
├── AGENTS.md                # Agent instructions and pre-instructions
├── SPEC.md                  # Frozen specification (source of truth)
├── implementation_plan.md   # Architecture diagrams + user answers
├── TASKS.md                 # Global sequential tasks file
├── README.md                # Project documentation (this file)
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
    ├── include/             # Header files (.h / .hpp)
    │   ├── Core/            # Game loop, state managers, input handlers
    │   ├── Entities/        # Mario, Luigi, Goomba, Koopa, blocks, items
    │   ├── Graphics/        # Renderer wrappers, spritesheets, animations
    │   ├── Physics/         # Collision detection, bounding boxes
    │   └── Utils/           # File I/O, helpers, config constants
    ├── src/                 # Source files (.cpp)
    │   ├── main.cpp         # Main entry point
    │   ├── Core/
    │   ├── Entities/
    │   ├── Graphics/
    │   ├── Physics/
    │   └── Utils/
    ├── assets/              # Textures, fonts, audio files, tilemaps
    ├── saves/               # Save files
    └── build/               # CMake build directory
```

---

## Build and Run Instructions

This project uses **CMake** to automatically pull SFML and ImGui source packages and compile them. No pre-installation of these libraries is required.

### Requirements
- **Compiler**: Supporting C++17 or higher (GCC, Clang, or MSVC)
- **CMake**: Version 3.15 or higher

### Build Steps
From a command prompt or terminal in the project root:

```bash
# 1. Navigate to the app directory
cd SuperMarioGame

# 2. Create a build directory
mkdir -p build && cd build

# 3. Configure the project with CMake (this will download SFML/ImGui dependencies)
cmake ..

# 4. Build the executable
cmake --build .
```

### Running the Game
After a successful build, run the generated executable from the build directory:
- **macOS / Linux**:
  ```bash
  ./SuperMarioGame
  ```
- **Windows**:
  ```cmd
  SuperMarioGame.exe
  ```
