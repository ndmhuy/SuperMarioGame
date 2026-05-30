# Super Mario Game

A modular, extensible, and high-performance 2D Mario platformer built in C++17 using **SFML 3.0.2** and **ImGui v1.91.8 + ImGui-SFML v3.0** for real-time engine tuning and level editing.

This project is developed as the CS202 Final Project, with an emphasis on Object-Oriented Programming (OOP) principles, clean architecture, and Software Design Patterns.

---

## 👥 Team Members
- **Nguyễn Đình Minh Huy** (Student ID: 25125083)

---

## 🎮 Game Controls

| Action | Player 1 (Solo / Co-op) | Player 2 (Versus Mode) |
| :--- | :--- | :--- |
| **Move Left** | `A` or `←` | `Left Arrow` |
| **Move Right** | `D` or `→` | `Right Arrow` |
| **Jump / Double Jump** | `W` or `↑` or `Space` | `Up Arrow` |
| **Run (Hold)** | `Left Shift` | `Right Shift` (or hold run) |
| **Fireball** | `F` or `J` | `M` |
| **Special Action** | `E` | `N` |
| **Switch Character** | `Tab` (Toggle between Mario & Luigi) | - |
| **Pause Game** | `Escape` | `Escape` |
| **Toggle Dev Tools** | `F12` (Open/Close ImGui UI Overlay) | - |

---

## 🛠️ Key Features & Architecture

* **Strict OOP Design**: High-quality class inheritance, polymorphism, encapsulation, and abstraction.
* **Layered Engine Architecture**:
  * **Core**: Game loop, game states, event system, input parsing, sound manager.
  * **Entities**: Abstract entities, characters (Mario, Luigi, enemies), items, blocks.
  * **Physics**: AABB (Axis-Aligned Bounding Box) collision detection and custom resolution.
  * **Graphics**: Animations, spritesheet handling, camera scrolling, particle system.
  * **Infrastructure**: Asset loading, save/load game state serialization.
* **Physics & Timestep**: Custom 2D physics with a fixed update timestep of 1/60s and frame-interpolation rendering.
* **Character Customization**:
  * **Mario**: Standard platforming physics.
  * **Luigi**: Higher jump force (×1.2), slower walking/running speed (×0.85), floatier air gravity (×0.9), and a special double-jump ability.
  * **Power-up Transitions**: Small $\rightarrow$ Super (Mushroom) $\rightarrow$ Fire (Fire Flower), with temporary Star invincibility overlay.
* **Sound Design**: Retro sound effects and looping background music tailored to each level's theme.
* **Checkpoint & Save System**: Checkpoint flag tracking; auto-saves on checkpoints and support for 3 manual save slots in JSON format.

---

## 📐 Software Design Patterns (6 Mapped)

The project implements **six core design patterns** to maintain code modularity:

| Pattern | Class / Component | Description |
| :--- | :--- | :--- |
| **Factory** | `EntityFactory` | Dynamically creates enemies, blocks, and items from coordinate and config data. |
| **Singleton** | `Game`, `ResourceManager`, `SoundManager` | Guarantees single, global instances of core engines with lazy initialization. |
| **State** | `GameStateManager` / `PlayerState` | Manages game screen transitions (Menu, Play, Pause, GameOver) and Player states (Small, Super, Fire, Star). |
| **Observer** | `EventBus` | Decouples gameplay events (e.g. coin collection, enemy stomped) from HUD updates and SFX cues. |
| **Strategy** | `IMovementStrategy` | Pluggable enemy AI patterns: `PatrolStrategy` (Goomba/Koopa), `ChaseStrategy` (Boo), and `FlyStrategy` (Paratroopa). |
| **Command** | `InputManager` / `ICommand` | Maps keyboard inputs to rebindable game commands. Enables Undo/Redo operations in the Level Editor. |

---

## 🚀 Post-MVP Bonus Features

1. **Mario Maker (Level Editor)**: Press `F1` in-game to pause and paint terrain, drag-and-drop enemies, place coins, and test maps in real time. (Nails the **Serialization** requirement).
2. **Time Manipulation (Rewind)**: Hold `Shift` to rewind the game state up to 5 seconds (~300 frames) using a circular state buffer.
3. **Shadow Mario (Rival AI)**: Mimics the player's recorded inputs with a 3-second delay.
4. **Dynamic Lighting (Shaders)**: Radial light shader around the player, lighting up pitch-black cave environments.
5. **Procedural Level Generation**: Endless runner mode with chunk-based terrain generation and automated scaling difficulty.

---

## 📁 Project Structure

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

## ⚙️ Build and Run Instructions

This project uses **CMake** to automatically pull SFML and ImGui source packages and compile them. No pre-installation of libraries is required.

### Requirements
- **Compiler**: C++17-capable compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- **CMake**: Version 3.15 or higher

### Build Steps

Run the following commands in your terminal from the project's **Git root** directory:

```bash
# 1. Navigate to the app directory
cd SuperMarioGame

# 2. Create and enter a build directory
mkdir -p build && cd build

# 3. Configure the project with CMake (this fetches SFML and ImGui packages)
cmake ..

# 4. Compile the project
cmake --build .
```

### Running the Game

After a successful compilation, execute the binary from the `SuperMarioGame/build` directory:

* **macOS / Linux**:
  ```bash
  ./SuperMarioGame
  ```
* **Windows**:
  ```cmd
  SuperMarioGame.exe
  ```
