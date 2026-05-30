# Super Mario Game — Sequential Task Checklist

> **Reference**: [SPEC.md](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SPEC.md) for all constants and specifications.
> **Rule**: Each phase = one feature branch off `dev`. Merge to `dev` when phase is complete.

---

## Phase 0 — Environment Setup ✅ COMPLETE

- [x] Initialize git repo, `dev` branch
- [x] Create directory structure (`include/`, `src/`, `assets/`, `logs/` under nested app folder)
- [x] Configure [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt) with SFML 3.0.2 + ImGui-SFML
- [x] Write baseline [main.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/main.cpp) with window + ImGui
- [x] Verify compilation
- [x] Create [AGENTS.md](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/AGENTS.md), [README.md](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/README.md), [.gitignore](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/.gitignore)
- [x] Commit: `feat: initialize project environment with SFML and ImGui`

---

## Phase 1 — Core Engine (`feature/core-engine`)

> **Goal**: Game loop, state management, resource loading, input, sound, events.
> **Branch**: `git checkout -b feature/core-engine dev`

### 1.1 Constants & Utilities
- [ ] Create [Constants.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Utils/Constants.hpp) — all game constants from SPEC §4
  - `WINDOW_WIDTH=1280`, `WINDOW_HEIGHT=720`, `TILE_SIZE=32`
  - `GRAVITY=0.5f`, `MARIO_WALK_SPEED=150.f`, `MARIO_RUN_SPEED=300.f`
  - `MARIO_JUMP_HEIGHT=128.f`, `FIXED_TIMESTEP=1.0f/60.0f`
  - `LUIGI_JUMP_MULT=1.2f`, `LUIGI_SPEED_MULT=0.85f`, `LUIGI_GRAVITY_MULT=0.9f`
  - `STAR_DURATION=10.0f`, `LEVEL_TIME=300.0f`, `INITIAL_LIVES=3`
- [ ] Create [MathUtils.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Utils/MathUtils.hpp) + [MathUtils.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Utils/MathUtils.cpp)
  - `clamp()`, `lerp()`, `sign()`, vector helpers
- [ ] Commit: `feat: add game constants and math utilities`

### 1.2 Resource Manager (Singleton)
- [ ] Create [ResourceManager.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/ResourceManager.hpp) + [ResourceManager.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/ResourceManager.cpp)
  - Singleton with `static ResourceManager& getInstance()`
  - `loadTexture(id, path)`, `getTexture(id)` → `sf::Texture&`
  - `loadFont(id, path)`, `getFont(id)` → `sf::Font&`
  - `loadSoundBuffer(id, path)`, `getSoundBuffer(id)` → `sf::SoundBuffer&`
  - Internal `std::unordered_map<std::string, sf::Texture>` etc.
  - Delete copy/move constructors
- [ ] Commit: `feat: implement ResourceManager singleton`

### 1.3 Sound Manager (Singleton)
- [ ] Create [SoundManager.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/SoundManager.hpp) + [SoundManager.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/SoundManager.cpp)
  - Singleton with `static SoundManager& getInstance()`
  - `playSound(id)` — plays a sound effect (pool of `sf::Sound` objects)
  - `playMusic(path)`, `stopMusic()`, `pauseMusic()`
  - `setSFXVolume(float)`, `setMusicVolume(float)`
  - Uses `ResourceManager` for sound buffers
  - `sf::Music` for streaming BGM
- [ ] Commit: `feat: implement SoundManager singleton`

### 1.4 Event Bus (Observer Pattern)
- [ ] Create [EventBus.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/EventBus.hpp) + [EventBus.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/EventBus.cpp)
  - `enum class EventType { CoinCollected, EnemyDefeated, PlayerDied, PowerUpCollected, LevelComplete, ... }`
  - `struct GameEvent { EventType type; std::any data; }`
  - `subscribe(EventType, std::function<void(const GameEvent&)>)` → returns subscription ID
  - `unsubscribe(subscriptionId)`
  - `publish(GameEvent)`
  - Thread-safe not required (single-threaded game loop)
- [ ] Commit: `feat: implement EventBus observer pattern`

### 1.5 Input Manager (Command Pattern)
- [ ] Create [InputManager.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/InputManager.hpp) + [InputManager.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/InputManager.cpp)
  - `ICommand` interface: `virtual void execute(Character&) = 0`
  - Concrete commands: `JumpCommand`, `MoveLeftCommand`, `MoveRightCommand`, `FireCommand`, `RunCommand`
  - `InputManager` maps `sf::Keyboard::Key` → `std::unique_ptr<ICommand>`
  - `handleInput(sf::Event, Character&)` — processes key events
  - `update(Character&)` — processes held keys (continuous movement)
  - Support Player 1 (WASD) and Player 2 (Arrow keys) mappings
  - Abstraction layer for future gamepad support
- [ ] Commit: `feat: implement InputManager with Command pattern`

### 1.6 Game State Manager (State Pattern)
- [ ] Create [IGameState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/IGameState.hpp) — abstract interface
  - `virtual void enter() = 0`
  - `virtual void exit() = 0`
  - `virtual void handleInput(sf::Event) = 0`
  - `virtual void update(float dt) = 0`
  - `virtual void render(sf::RenderTarget&) = 0`
  - `virtual ~IGameState() = default`
- [ ] Create [GameStateManager.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/GameStateManager.hpp) + [GameStateManager.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/GameStateManager.cpp)
  - Stack-based: `pushState()`, `popState()`, `changeState()`
  - `std::stack<std::unique_ptr<IGameState>>`
  - `getCurrentState()` → `IGameState*`
  - Calls enter/exit on transitions
- [ ] Commit: `feat: implement GameStateManager with state stack`

### 1.7 Game Class (Singleton, Main Loop)
- [ ] Create [Game.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/Game.hpp) + [Game.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/Game.cpp)
  - Singleton: `static Game& getInstance()`
  - Owns `sf::RenderWindow` (1280×720)
  - Owns `GameStateManager`
  - Fixed timestep game loop:
    ```cpp
    while (running) {
      processEvents();
      accumulator += frameTime;
      while (accumulator >= FIXED_TIMESTEP) {
        update(FIXED_TIMESTEP);
        accumulator -= FIXED_TIMESTEP;
      }
      render(accumulator / FIXED_TIMESTEP); // interpolation alpha
    }
    ```
  - `run()`, `quit()`
  - ImGui integration: `ImGui::SFML::Update()` in loop
  - Initial state: push `MenuState` (placeholder for now)
- [ ] Refactor [main.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/main.cpp) — calls `Game::getInstance().run()`
- [ ] Commit: `feat: implement Game singleton with fixed-timestep loop`

### 1.8 Placeholder States
- [ ] Create [MenuState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/MenuState.hpp) + [MenuState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/MenuState.cpp) — placeholder "Press Enter to Start"
- [ ] Create [PlayingState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/PlayingState.hpp) + [PlayingState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/PlayingState.cpp) — placeholder colored screen
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt) — add all new source files
- [ ] Verify build compiles and runs with state transitions
- [ ] Commit: `feat: add placeholder MenuState and PlayingState`
- [ ] **Merge**: `git checkout dev && git merge feature/core-engine`

---

## Phase 2 — Physics Engine (`feature/physics`)

> **Goal**: AABB collision, gravity, velocity, tile-based collision resolution.
> **Branch**: `git checkout -b feature/physics dev`

### 2.1 AABB
- [ ] Create [AABB.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Physics/AABB.hpp) + [AABB.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Physics/AABB.cpp)
  - `struct AABB { float x, y, width, height; }`
  - `bool intersects(const AABB& other) const`
  - `AABB getOverlap(const AABB& other) const`
  - `bool contains(float px, float py) const`
  - `sf::Vector2f getCenter() const`
- [ ] Commit: `feat: implement AABB collision primitive`

### 2.2 Collision Detector
- [ ] Create [CollisionDetector.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Physics/CollisionDetector.hpp) + [CollisionDetector.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Physics/CollisionDetector.cpp)
  - `struct CollisionInfo { bool collided; sf::Vector2f overlap; sf::Vector2f normal; Entity* other; }`
  - `checkEntityVsEntity(Entity&, Entity&)` → `CollisionInfo`
  - `checkEntityVsTileMap(Entity&, TileMap&)` → `std::vector<CollisionInfo>`
  - Direction detection: top, bottom, left, right collision normals
- [ ] Commit: `feat: implement CollisionDetector with direction sensing`

### 2.3 Collision Resolver
- [ ] Create [CollisionResolver.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Physics/CollisionResolver.hpp) + [CollisionResolver.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Physics/CollisionResolver.cpp)
  - `resolveEntityVsTile(Entity&, CollisionInfo&)` — push entity out of tile
  - `resolveEntityVsEntity(Entity&, Entity&, CollisionInfo&)` — stomp/damage/bounce
  - `resolvePlayerVsEnemy(Character&, Enemy&, CollisionInfo&)` — stomp vs side hit
  - `resolvePlayerVsItem(Character&, Item&, CollisionInfo&)` — collect/activate
  - Ground detection: set `onGround = true` when bottom collision with tile
- [ ] Commit: `feat: implement CollisionResolver with response logic`

### 2.4 Physics Engine
- [ ] Create [PhysicsEngine.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Physics/PhysicsEngine.hpp) + [PhysicsEngine.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Physics/PhysicsEngine.cpp)
  - `applyGravity(Entity&, float dt)` — add gravity to velocity.y (use Constants::GRAVITY)
  - `integrateVelocity(Entity&, float dt)` — position += velocity * dt
  - `update(std::vector<Entity*>&, TileMap&, float dt)`:
    1. Apply gravity to all entities
    2. Integrate velocities
    3. Detect collisions (entity-vs-tile, entity-vs-entity)
    4. Resolve collisions
    5. Update ground states
  - ImGui debug: toggle collision box rendering, show velocity vectors
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt)
- [ ] Write a test: place a rectangle on screen, verify it falls and stops on a floor tile
- [ ] Commit: `feat: implement PhysicsEngine with gravity and collision pipeline`
- [ ] **Merge**: `git checkout dev && git merge feature/physics`

---

## Phase 3 — Entity Hierarchy (`feature/entities`)

> **Goal**: All game entities — player characters, enemies, items, blocks, and EntityFactory.
> **Branch**: `git checkout -b feature/entities dev`

### 3.1 Entity Base Class
- [ ] Create [Entity.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Entity.hpp) + [Entity.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Entity.cpp)
  - Abstract base. Members: `position`, `velocity`, `boundingBox`, `active`, `sprite`
  - Pure virtual: `update(float dt)`, `render(sf::RenderTarget&)`
  - Virtual: `getBoundingBox()`, `isActive()`, `destroy()`
  - Virtual destructor
- [ ] Commit: `feat: implement abstract Entity base class`

### 3.2 Character Base Class
- [ ] Create [Character.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Character.hpp) + [Character.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Character.cpp)
  - Inherits `Entity`. Members: `health`, `speed`, `jumpForce`, `onGround`, `facingRight`
  - Methods: `moveLeft()`, `moveRight()`, `jump()`, `run()`, `takeDamage()`
  - `PlayerState` enum: `Small, Super, Fire, Star`
  - State transitions: `powerUp(ItemType)`, `powerDown()`
  - Invincibility frames timer after hit
- [ ] Commit: `feat: implement Character base class with state management`

### 3.3 Mario
- [ ] Create [Mario.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Mario.hpp) + [Mario.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Mario.cpp)
  - Inherits `Character`
  - Physics values from `Constants.hpp`: walk=150, run=300, jump=4 tiles
  - `shootFireball()` — if Fire state, max 2 active
  - Animation state tracking (idle, walk, run, jump, crouch, die)
- [ ] Commit: `feat: implement Mario entity`

### 3.4 Luigi
- [ ] Create [Luigi.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Luigi.hpp) + [Luigi.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Luigi.cpp)
  - Inherits `Character`
  - Modified physics: speed×0.85, jump×1.2, airGravity×0.9
  - `doubleJump()` — can jump once more while airborne
  - Separate animation set
- [ ] Commit: `feat: implement Luigi entity with double jump`

### 3.5 Enemy Base + AI Strategies
- [ ] Create [IMovementStrategy.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/IMovementStrategy.hpp) — interface
  - `virtual void move(Enemy& enemy, float dt) = 0`
  - `virtual ~IMovementStrategy() = default`
- [ ] Create [PatrolStrategy.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/PatrolStrategy.hpp) + [PatrolStrategy.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/PatrolStrategy.cpp)
  - Walk in one direction. Reverse on wall collision. Fall off ledges.
- [ ] Create [ChaseStrategy.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/ChaseStrategy.hpp) + [ChaseStrategy.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/ChaseStrategy.cpp)
  - Idle until target within 250px. Move toward target. Stop if target faces enemy (for Boo).
- [ ] Create [FlyStrategy.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/FlyStrategy.hpp) + [FlyStrategy.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/FlyStrategy.cpp)
  - Sinusoidal vertical movement while patrolling horizontally.
- [ ] Create [Enemy.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Enemy.hpp) + [Enemy.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Enemy.cpp)
  - Inherits `Character`. Holds `std::unique_ptr<IMovementStrategy>`
  - `setStrategy(std::unique_ptr<IMovementStrategy>)`
  - `onStomped()` — virtual, default: die
  - `onHitByFireball()` — virtual, default: die
  - Scoring: each enemy kill grants points
- [ ] Commit: `feat: implement Enemy base class and AI movement strategies`

### 3.6 Concrete Enemies
- [ ] Create [Goomba.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Goomba.hpp) + [Goomba.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Goomba.cpp)
  - Uses `PatrolStrategy`. Stomped → squish animation → destroy.
- [ ] Create [KoopaTroopa.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/KoopaTroopa.hpp) + [KoopaTroopa.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/KoopaTroopa.cpp)
  - Uses `PatrolStrategy`. Stomped → shell state. `kickShell()` sends shell sliding.
  - Shell can kill other enemies on contact.
- [ ] Create [KoopaParatroopa.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/KoopaParatroopa.hpp) + [KoopaParatroopa.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/KoopaParatroopa.cpp)
  - Uses `FlyStrategy`. Stomped → loses wings → becomes regular KoopaTroopa.
- [ ] Create [Boo.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Boo.hpp) + [Boo.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Boo.cpp)
  - Uses `ChaseStrategy`. Cannot be stomped/fireballed. Freezes when player faces it.
- [ ] Commit: `feat: implement Goomba, KoopaTroopa, Paratroopa, and Boo enemies`

### 3.7 Items
- [ ] Create [Item.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Item.hpp) + [Item.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Item.cpp)
  - Inherits `Entity`. `collected` flag. `virtual activate(Character&)`, `collect()`
  - Pop-out animation (rises from block)
- [ ] Create concrete items:
  - [ ] [Mushroom.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Mushroom.hpp) + [Mushroom.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Mushroom.cpp) — Small→Super. Moves horizontally.
  - [ ] [FireFlower.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/FireFlower.hpp) + [FireFlower.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/FireFlower.cpp) — Super→Fire. Stationary.
  - [ ] [Coin.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Coin.hpp) + [Coin.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Coin.cpp) — +1 coin, +200 score. EventBus `CoinCollected`.
  - [ ] [Star.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Star.hpp) + [Star.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Star.cpp) — 10s invincibility. Bounces.
  - [ ] [OneUpMushroom.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/OneUpMushroom.hpp) + [OneUpMushroom.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/OneUpMushroom.cpp) — +1 life. Moves horizontally.
- [ ] Commit: `feat: implement all items (Mushroom, FireFlower, Coin, Star, 1-UP)`

### 3.8 Blocks
- [ ] Create [Block.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Block.hpp) + [Block.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Block.cpp)
  - Inherits `Entity`. `breakable` flag. `virtual onHitFromBelow(Character&)`
  - Bump animation when hit
- [ ] Create concrete blocks:
  - [ ] [BrickBlock.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/BrickBlock.hpp) + [BrickBlock.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/BrickBlock.cpp) — breakable if Super/Fire. Particles.
  - [ ] [QuestionBlock.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/QuestionBlock.hpp) + [QuestionBlock.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/QuestionBlock.cpp) — contains item. Hit → spawn item + empty block.
  - [ ] [Pipe.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Pipe.hpp) + [Pipe.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Pipe.cpp) — warp pipe. `warpTarget`. Enter animation.
  - [ ] [Flagpole.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Flagpole.hpp) + [Flagpole.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Flagpole.cpp) — level end marker.
- [ ] Commit: `feat: implement BrickBlock, QuestionBlock, Pipe, and Flagpole`

### 3.9 Entity Factory (Factory Pattern)
- [ ] Create [EntityFactory.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/EntityFactory.hpp) + [EntityFactory.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/EntityFactory.cpp)
  - `enum class EntityType { Mario, Luigi, Goomba, KoopaTroopa, Paratroopa, Boo, Mushroom, FireFlower, Coin, Star, OneUp, BrickBlock, QuestionBlock, Pipe, Flagpole }`
  - `static std::unique_ptr<Entity> create(EntityType type, sf::Vector2f position)`
  - `static std::unique_ptr<Entity> create(EntityType type, sf::Vector2f position, const json& config)` — for level loader
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt) with all entity source files
- [ ] Verify build compiles
- [ ] Commit: `feat: implement EntityFactory for all entity types`
- [ ] **Merge**: `git checkout dev && git merge feature/entities`

---

## Phase 4 — Tilemap & Levels (`feature/tilemap-levels`)

> **Goal**: Level loading from JSON, tilemap rendering, camera scrolling.
> **Branch**: `git checkout -b feature/tilemap-levels dev`

### 4.1 TileMap
- [ ] Create [TileMap.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Utils/TileMap.hpp) + [TileMap.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Utils/TileMap.cpp)
  - 2D grid: `std::vector<std::vector<int>> tileGrid` (tile IDs)
  - Tile types: `Empty=0, Ground=1, Brick=2, Question=3, Pipe=4, ...`
  - `loadFromGrid(grid data)`
  - `render(sf::RenderTarget&, Camera&)` — only draw visible tiles (culling)
  - `getTileAt(int gridX, int gridY)` → tile type
  - `worldToGrid(sf::Vector2f)` → grid coords
  - `gridToWorld(int x, int y)` → world position
  - Tileset texture: single spritesheet, index to sub-rect
- [ ] Commit: `feat: implement TileMap with tile-based rendering and culling`

### 4.2 Level Loader
- [ ] Create [LevelLoader.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Utils/LevelLoader.hpp) + [LevelLoader.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Utils/LevelLoader.cpp)
  - `loadLevel(const std::string& jsonPath)` → `Level` struct
  - `struct Level { TileMap tilemap; std::vector<std::unique_ptr<Entity>> entities; sf::Vector2f spawnPoint; std::vector<sf::Vector2f> checkpoints; sf::Vector2f flagpolePos; std::string theme; }`
  - Parse JSON format from SPEC §8.3
  - Uses `EntityFactory::create()` for all entities
  - Validate level dimensions (200×23 tiles)
- [ ] Add a JSON parsing library (nlohmann/json via FetchContent or header-only)
- [ ] Commit: `feat: implement LevelLoader with JSON parsing`

### 4.3 Camera
- [ ] Create [Camera.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/Camera.hpp) + [Camera.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/Camera.cpp)
  - Wraps `sf::View`
  - `follow(const Entity& target)` — smooth follow with lookahead
  - `setBounds(float levelWidth, float levelHeight)` — clamp to level edges
  - Horizontal scrolling only (vertical locked)
  - `getVisibleBounds()` → `AABB` for tile/entity culling
- [ ] Commit: `feat: implement Camera with horizontal scrolling and bounds clamping`

### 4.4 Design Level Files
- [ ] Create [level_1.json](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/levels/level_1.json) — Overworld/Grassland
  - Ground tiles, platforms, gaps, QuestionBlocks, BrickBlocks, Goombas, Koopas, Coins, 1 checkpoint, 1 flagpole, warp pipe
- [ ] Create [level_2.json](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/levels/level_2.json) — Underground/Cave
  - Darker theme, pits, tighter platforms, more enemies, Paratroopas
- [ ] Create [level_3.json](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/levels/level_3.json) — Castle/Lava
  - Lava pits (instant death), Boo ghosts, Bowser boss area
- [ ] Commit: `feat: create 3 level JSON files (Overworld, Underground, Castle)`

### 4.5 Integration Test
- [ ] Wire `PlayingState` to load Level 1, render TileMap, spawn Mario
- [ ] Verify camera follows Mario, tiles render correctly
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt)
- [ ] Commit: `feat: integrate tilemap and level loading into PlayingState`
- [ ] **Merge**: `git checkout dev && git merge feature/tilemap-levels`

---

## Phase 5 — Graphics & Animation (`feature/graphics`)

> **Goal**: Sprite animations, HUD, parallax backgrounds, particles.
> **Branch**: `git checkout -b feature/graphics dev`

### 5.1 SpriteSheet Handler
- [ ] Create [SpriteSheet.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/SpriteSheet.hpp) + [SpriteSheet.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/SpriteSheet.cpp)
  - Load a texture atlas, support variable frame sizes
- [ ] Commit: `feat: implement SpriteSheet texture atlas handler`

### 5.2 Animation System
- [ ] Create [Animation.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/Animation.hpp) + [Animation.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/Animation.cpp)
  - `Animation(SpriteSheet&, std::vector<int> frameIndices, float frameDuration, bool loop)`
- [ ] Create [AnimationManager.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/AnimationManager.hpp) + [AnimationManager.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/AnimationManager.cpp)
  - Load animation definitions from config (`mario_idle`, `mario_walk`, etc.)
- [ ] Commit: `feat: implement Animation and AnimationManager`

### 5.3 HUD
- [ ] Create [HUD.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/HUD.hpp) + [HUD.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/HUD.cpp)
  - Render overlay: Score, Coins, World, Time, Lives (matching SPEC §9.4)
  - Retro font: `PressStart2P.ttf`
- [ ] Source a pixel font and add to `assets/fonts/`
- [ ] Commit: `feat: implement HUD with score, coins, time, and lives display`

### 5.4 Parallax Background
- [ ] Implement multi-layer background rendering in `PlayingState` or `Renderer`
  - Sky gradient, slow clouds (~0.2× speed), mountains (~0.5× speed)
- [ ] Source/create background tile assets
- [ ] Commit: `feat: implement parallax scrolling backgrounds`

### 5.5 Particle System
- [ ] Create [ParticleSystem.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/ParticleSystem.hpp) + [ParticleSystem.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Graphics/ParticleSystem.cpp)
  - Pre-allocated particle pool. Types: `BrickBreak`, `CoinSparkle`, `DeathPoof`, `Stomp`.
- [ ] Commit: `feat: implement ParticleSystem with brick and coin effects`

### 5.6 Screen Transitions
- [ ] Implement fade-in/fade-out black overlay transition in `GameStateManager`
- [ ] Commit: `feat: implement fade screen transitions`

### 5.7 Source & Integrate Assets
- [ ] Download 16-bit Mario-style spritesheets, place in [textures/](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/textures/)
- [ ] Wire all entities to use spritesheets instead of colored rectangles
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt)
- [ ] Commit: `feat: integrate sprite assets and wire to all entities`
- [ ] **Merge**: `git checkout dev && git merge feature/graphics`

---

## Phase 6 — Audio (`feature/audio`)

> **Goal**: Wire SoundManager, load SFX and BGM, volume controls.
> **Branch**: `git checkout -b feature/audio dev`

### 6.1 Source Audio Assets
- [ ] Find retro sound effects and BGM, place in [sounds/sfx/](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/sounds/sfx/) and [sounds/music/](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/assets/sounds/music/)
- [ ] Commit: `feat: add audio assets (SFX and BGM)`

### 6.2 Wire Sound Events
- [ ] In `SoundManager`, load all SFX on startup via `ResourceManager`
- [ ] Subscribe `SoundManager` to `EventBus` events (`CoinCollected`, `EnemyDefeated`, etc.)
- [ ] Add direct play calls for jumps, fireballs, pipe warping
- [ ] BGM handling for levels, victory, game over, and temporary star power override
- [ ] Commit: `feat: wire all sound events and background music`

### 6.3 Volume Controls
- [ ] Add volume sliders in Options menu and persist volume settings
- [ ] Commit: `feat: add SFX and music volume controls`
- [ ] **Merge**: `git checkout dev && git merge feature/audio`

---

## Phase 7 — Game States & UI (`feature/game-states`)

> **Goal**: All menu screens, pause, game over, victory.
> **Branch**: `git checkout -b feature/game-states dev`

### 7.1 Menu State
- [ ] Rewrite `MenuState` with keyboard-navigable options: New Game, Load Game, Options, High Scores, Quit
- [ ] Commit: `feat: implement full MenuState with navigation`

### 7.2 Character Select State
- [ ] Create [CharSelectState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/CharSelectState.hpp) + [CharSelectState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/CharSelectState.cpp)
  - Display Mario and Luigi side-by-side with stats and descriptions
- [ ] Commit: `feat: implement CharSelectState`

### 7.3 Playing State (Full Implementation)
- [ ] Rewrite `PlayingState` to integrate: level load, player spawn, loop (Input → Physics → Entity Update → Camera → Render)
- [ ] Add collision callbacks: stomp, item collect, pipe warp, flagpole
- [ ] Add checkpoints, time limit, Pause (Esc), character switch (Tab)
- [ ] Add ImGui development panel: physics tuning, entity inspector
- [ ] Commit: `feat: implement full PlayingState with all game systems`

### 7.4 Pause State
- [ ] Create [PauseState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/PauseState.hpp) + [PauseState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/PauseState.cpp)
  - Transparent overlay with Resume, Restart, Main Menu, Quit
- [ ] Commit: `feat: implement PauseState overlay`

### 7.5 Game Over State
- [ ] Create [GameOverState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/GameOverState.hpp) + [GameOverState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/GameOverState.cpp)
  - Animated screen with final score, options to return to menu
- [ ] Commit: `feat: implement GameOverState`

### 7.6 Victory State
- [ ] Create [VictoryState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/VictoryState.hpp) + [VictoryState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/VictoryState.cpp)
  - Score calculations (level complete, time/coin bonus), level transition
- [ ] Commit: `feat: implement VictoryState and level transitions`

### 7.7 Options State & High Scores
- [ ] Create [OptionsState.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Core/OptionsState.hpp) + [OptionsState.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Core/OptionsState.cpp)
  - Volume sliders, keyboard mapping reference
- [ ] Save/load high scores to [highscores.json](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/saves/highscores.json)
- [ ] Commit: `feat: implement OptionsState`
- [ ] Commit: `feat: implement high score display and persistence`
- [ ] Update [CMakeLists.txt](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/CMakeLists.txt)
- [ ] **Merge**: `git checkout dev && git merge feature/game-states`

---

## Phase 8 — Save/Load (`feature/save-load`)

> **Goal**: JSON serialization, save slots, auto-save at checkpoints.
> **Branch**: `git checkout -b feature/save-load dev`

### 8.1 Serializer
- [ ] Create [Serializer.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Utils/Serializer.hpp) + [Serializer.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Utils/Serializer.cpp)
  - Save/load slots 1-3 to [saves/slot_X.json](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/saves/) using SPEC §11.2 schema
- [ ] Commit: `feat: implement Serializer for save/load with JSON`

### 8.2 Save/Load Integration
- [ ] Trigger auto-save on reaching checkpoints
- [ ] Add manual save option in Pause Menu, load slot previews in Main Menu
- [ ] Commit: `feat: integrate save/load into game flow`
- [ ] **Merge**: `git checkout dev && git merge feature/save-load`

---

## Phase 9 — Enemy AI & Boss (`feature/enemy-ai`)

> **Goal**: Tune all enemy behaviors, implement Bowser boss fight.
> **Branch**: `git checkout -b feature/enemy-ai dev`

### 9.1 AI Behavior Tuning
- [ ] Tune Goombas, Koopa Troopa shell physics, Paratroopa flight paths, Boo proximity
- [ ] Add ImGui AI debug overlay (state labels, target vectors)
- [ ] Commit: `feat: tune enemy AI behaviors and add debug visualization`

### 9.2 Bowser Boss
- [ ] Create [Bowser.hpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Entities/Bowser.hpp) + [Bowser.cpp](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/src/Entities/Bowser.cpp)
  - Custom Boss AI: Phase 1 (walk + breathe fire), Phase 2 (jump + faster fire)
  - Add Bowser health bar to HUD, boss fireball entities, Level 3 boss arena
- [ ] Commit: `feat: implement Bowser boss fight for Level 3`

### 9.3 Difficulty Scaling
- [ ] Balance entity distributions and hazard counts across Levels 1, 2, and 3
- [ ] Commit: `feat: finalize difficulty progression across all levels`
- [ ] **Merge**: `git checkout dev && git merge feature/enemy-ai`

---

## Phase 10 — Two-Player & Polish (`feature/polish`)

> **Goal**: 2P versus mode, bug fixes, edge cases, visual polish.
> **Branch**: `git checkout -b feature/polish dev`

### 10.1 Two-Player Versus Mode
- [ ] Configure Player 2 keyboard mappings, spawn Mario + Luigi in PlayingState
- [ ] Implement shared camera following the leading player, versus rules (flagpole race, stomp option)
- [ ] Commit: `feat: implement two-player versus mode`

### 10.2 Edge Cases & Bug Fixes
- [ ] Respawn at checkpoint with death animation and delay
- [ ] Flashing invincibility frames after getting hit
- [ ] Clamp player movement inside camera boundaries
- [ ] Pipe warp sliding transitions, Shell collision chain reactions, Time-out death, 100 coin 1-UP jingle, floating score text
- [ ] Commit: `feat: fix edge cases and add death/respawn logic`

### 10.3 Visual Polish & Testing
- [ ] Screen shake on block breaks/Bowser stomps, coin sparkle trails, flagpole slide, World intro screen
- [ ] Commit: `feat: add visual polish effects`
- [ ] Complete full walkthrough tests of all 3 levels, power-up states, save/load slots, and 2P mode
- [ ] Verify steady 60fps performance
- [ ] Commit: `feat: final playtest and performance verification`
- [ ] **Merge**: `git checkout dev && git merge feature/polish`

---

## Bonus Phases (Post-MVP)

### Bonus A — Level Editor (`feature/level-editor`)
- [ ] ImGui editor overlay (F1 toggle)
- [ ] Tile palette: select and paint tiles
- [ ] Entity palette: drag-and-drop enemies, items
- [ ] Undo/Redo with Command Pattern
- [ ] Export/import level JSON
- [ ] Play-test button: instant switch to PlayingState with current level

### Bonus B — Time Rewind (`feature/time-rewind`)
- [ ] Circular buffer storing 300 frames of game state snapshots
- [ ] Memento Pattern: `GameSnapshot` struct with all entity states
- [ ] Hold Shift → reverse playback
- [ ] Visual indicator (VHS rewind effect overlay)

### Bonus C — Shadow Mario (`feature/shadow-mario`)
- [ ] Record player input each frame
- [ ] Spawn Shadow Mario at level start
- [ ] Replay recorded inputs with 3-second delay
- [ ] Shadow visual: dark/translucent Mario sprite

### Bonus D — Dynamic Lighting (`feature/dynamic-lighting`)
- [ ] GLSL fragment shader for radial light around Mario
- [ ] Underground level: pitch black except light radius
- [ ] Fireball illumination
- [ ] Optional: day/night cycle for overworld

### Bonus E — Procedural Generation (`feature/procedural-levels`)
- [ ] "Endless Mode" menu option
- [ ] Chunk-based terrain generation (8-tile wide chunks)
- [ ] Difficulty scaling over distance
- [ ] Random enemy and coin placement with rules (no impossible jumps)
