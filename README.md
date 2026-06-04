# Super Mario Game

A modular, extensible, and high-performance 2D Mario platformer built in C++17 using **SFML 3.0.2** and **ImGui v1.91.8 + ImGui-SFML v3.0** for real-time engine tuning and level editing.

This project is developed as the CS202 Final Project, with an emphasis on Object-Oriented Programming (OOP) principles, clean architecture, and Software Design Patterns. The project implements **110 distinct features** across 20 categories, powered by **10+ design patterns**.

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
| **Crouch / Ground Pound** | `S` or `↓` | `Down Arrow` |
| **Run (Hold)** | `Left Shift` | `Right Shift` |
| **Fireball** | `F` or `J` | `M` |
| **Special Action** | `E` | `N` |
| **Switch Character** | `Tab` (Toggle characters) | - |
| **Minimap Toggle** | `M` | - |
| **Pause Game** | `Escape` | `Escape` |
| **Toggle Dev Tools** | `F12` (Open/Close ImGui UI Overlay) | - |
| **Debug Console** | `~` (Tilde) | - |

---

## 🛠️ Key Features & Architecture

### Core Engine
* **Layered Engine Architecture**: Core → Entities → Physics → Graphics → Infrastructure
* **Fixed-Timestep Game Loop**: 1/60s updates with frame-interpolated rendering
* **9 Game States**: Menu, World Map, Character Select, Playing, Pause, Game Over, Victory, Options, Statistics

### Characters (4 Playable)
* **Mario**: Standard platforming physics
* **Luigi**: Higher jump (×1.2), slower speed (×0.85), floatier gravity (×0.9), double jump
* **Toad** (Unlockable): Faster speed (×1.3), lower jump (×0.8), instant acceleration
* **Peach** (Unlockable): Float hover (1.5s), slightly slower (×0.9)
* **5 Power-Up States**: Small → Super → Fire/Cape, Mini (half-size)
* **2 Decorators**: Star (10s invincible), Mega (8s giant)

### Gameplay Mechanics (10 Movement Types)
* Walk, Run, Jump, Wall Jump/Slide, Ground Pound, Crouch/Slide, Swimming, Climbing, Combo System, Momentum/Skidding
* **Coyote Time** (6 frames) & **Jump Buffering** (6 frames) for polished game feel
* **Damage Knockback** with stun frames

### Enemies (13 Types + 3 Color Variants)
* Goomba, Koopa Troopa, Paratroopa, Boo, Piranha Plant, Bullet Bill, Hammer Bro, Thwomp, Chain Chomp, Lakitu & Spiny
* **Boom Boom** (mid-boss) and **Bowser** (final boss) with multi-phase AI
* Color variants (Red Goomba, Red Koopa, Red Paratroopa) with unique AI behaviors

### Items & Power-ups (12 Types)
* Mushroom, Fire Flower, Coin, Star, 1-UP, Cape Feather, Mega Mushroom, Mini Mushroom, POW Block, P-Switch, Trampoline, Star Coin

### Blocks & World Objects (9 Types)
* Brick, Question, Hidden, Pipe, Flagpole, Moving Platform, Falling Platform, Ice Block, Conveyor Belt

### Levels & World
* **3 Themed Levels**: Overworld, Underground, Castle (200 tiles each)
* **World Map**: SMB3-style overhead navigation with level nodes and star coin tracking
* **Special Sections**: Swimming, vertical scrolling, autoscroll
* **Bonus Rooms**: Hidden warp pipe destinations with timed coin collection
* **3 Star Coins per level** for completionism and unlockables

### Visual Effects
* Parallax scrolling backgrounds, particle system (8 types), screen shake
* Entity death animations, invincibility FX (rainbow cycling, sprite flashing)
* Water/lava animated surfaces, floating score text, combo counter display

### Audio
* **17+ Sound Effects** with surface-dependent footsteps and combo SFX escalation
* **Dynamic Music Layers** that respond to gameplay (proximity to boss, low timer, combos)
* Separate SFX/Music volume sliders

### Save System & Persistence
* 3 save slots with auto-save at checkpoints
* Statistics tracking (enemies defeated, coins, deaths, time, combos)
* **Achievement System** with 10 unlockable achievements and toast notifications
* Settings persistence (volume, difficulty, key bindings, colorblind mode)

### Accessibility
* **3 Difficulty Modes**: Easy (5 lives, slower enemies) / Normal / Hard (1 life, no checkpoints)
* **Colorblind Mode**: Deuteranopia, Protanopia, Tritanopia palette swaps
* Audio navigation cues for menus

### Meta-Game
* **New Game+**: Mirrored levels, faster enemies, fewer power-ups
* **Daily Challenge**: Date-seeded procedural level with local leaderboard
* **Unlockable Characters**: Toad (complete all levels), Peach (no-death run)

---

## 📐 Software Design Patterns (10+ Implemented)

| Pattern | Class / Component | Description |
| :--- | :--- | :--- |
| **Factory** | `EntityFactory` | Creates 25+ entity types. Lakitu uses Factory to spawn Spinies at runtime. |
| **Singleton** | `Game`, `ResourceManager`, `SoundManager`, `AchievementManager` | Global instances with lazy initialization. |
| **State** | `GameStateManager`, `IPlayerState`, `FallingPlatform`, `Thwomp` | 9 game states, 5 base player states, entity lifecycles. |
| **Observer** | `EventBus` | 15+ event types. HUD, Sound, Combo, Achievement, Statistics trackers all subscribe. |
| **Strategy** | `IMovementStrategy` | 7+ enemy AI strategies (Patrol, Chase, Fly, Swim, Tethered, HammerThrow, Proximity). Difficulty modes. |
| **Command** | `InputManager` / `ICommand` | 8+ game commands. Key rebinding. Debug console text→command. Replay serialization. |
| **Decorator** | `StarDecorator`, `MegaDecorator` | Temporary power-up overlays wrapping active `IPlayerState`. |
| **Memento** | `GameSnapshot` | Time rewind and replay system state capture. |
| **Object Pool** | `ObjectPool<T>` | Pre-allocated pools for fireballs, particles, projectiles. |
| **Template Method** | `IMovementStrategy::execute()` | Base skeleton: `calculateTarget() → applyMovement() → checkConstraints()`. Concrete strategies override hooks. |

---

## 🚀 Post-MVP Bonus Features

1. **Mario Maker (Level Editor)**: Press `F1` in-game to paint terrain, drag-and-drop enemies, undo/redo, and test maps in real time.
2. **Time Manipulation (Rewind)**: Hold `Shift` to rewind game state up to 5 seconds (~300 frames) using a circular state buffer.
3. **Shadow Mario (Rival AI)**: Mimics the player's recorded inputs with a 3-second delay.
4. **Dynamic Lighting (Shaders)**: Radial light shader around the player, lighting up pitch-black cave environments.
5. **Procedural Level Generation**: Endless runner mode with chunk-based terrain generation and automated scaling difficulty.

---

## 📁 Project Structure

```text
SuperMarioGame/              # Git root
├── AGENTS.md                # Agent instructions and pre-instructions
├── SPEC.md                  # Specification v2.0 (110 features)
├── FEATURE_PROPOSAL.md      # TA feature expansion justification
├── implementation_plan.md   # Architecture diagrams + user answers
├── TASKS.md                 # Global sequential tasks file (11 phases)
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
└── SuperMarioGame/          # ── APP CODE FOLDER ──
    ├── CMakeLists.txt       # CMake build configuration
    ├── include/             # Header files (.hpp)
    │   ├── Core/            # Game loop, state managers, input, events, achievements
    │   ├── Entities/        # Mario, Luigi, Toad, Peach, 13 enemies, 12 items, 9 blocks
    │   ├── Graphics/        # Renderer, animations, HUD, minimap, particles, camera
    │   ├── Physics/         # AABB, spatial hash, collision detection/resolution
    │   └── Utils/           # Constants, TileMap, LevelLoader, Serializer, ObjectPool, DebugConsole
    ├── src/                 # Source files (.cpp) — mirrors include/
    │   └── main.cpp         # Entry point: Game::getInstance().run()
    ├── assets/
    │   ├── textures/        # Sprite sheets, tilesets, backgrounds
    │   ├── sounds/sfx/      # WAV sound effects (17+)
    │   ├── sounds/music/    # OGG background music (10+ tracks)
    │   ├── fonts/           # PressStart2P.ttf
    │   ├── levels/          # Level JSON files (3 + bonus)
    │   └── config/          # entities.json, config.json
    ├── saves/               # Save files (slot_1..3, highscores, daily, stats)
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
