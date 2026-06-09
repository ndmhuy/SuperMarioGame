# 2-Member Workflow: Super Mario Game

> Two humans, each with their own AI agent, working on the same repo without stepping on each other.

---

## The Core Problem

Two people editing the same files = merge hell. The solution: **split by architectural layer**, not by "half the tasks."

---

## Proposed Split

| | **Member A (You — Huy)** | **Member B (Partner)** |
|:---|:---|:---|
| **Domain** | 🏗️ **Engine & Infrastructure** | 🎮 **Entities & Gameplay** |
| **Owns** | `Core/`, `Physics/`, `Graphics/`, `Utils/` | `Entities/`, level JSONs, game states UI |
| **Focus** | The "how it runs" layer | The "what plays" layer |

### Why This Split Works

```
Member A builds the ENGINE        Member B builds the CONTENT
─────────────────────────         ──────────────────────────
Game loop, state manager          Mario, Luigi, enemies, items, blocks
Physics engine, collision         AI strategies (all 8)
Camera, HUD, particles            Entity Factory (registers types)
Resource/Sound managers           Level JSON files
EventBus, InputManager            Concrete game states (menus, playing)
Animation system                  Boss fights (Boom Boom, Bowser)
TileMap rendering                 Save/Load serializer
Object Pool, Debug Console        Achievements, Statistics
```

**A provides the framework. B fills it with game content.** They rarely touch the same files.

---

## Phase-by-Phase Assignment

| Phase | Tasks | Member A | Member B |
|:---:|:---|:---:|:---:|
| **1** | Core Engine | ✅ Owns all | — waits or works on entity headers |
| **2** | Physics Engine | ✅ Owns all | — can start Entity/Character base classes |
| **3** | Entity Hierarchy | Interface contracts only | ✅ Owns all (3.1–3.14) |
| **4** | Tilemap & Levels | ✅ TileMap, Camera, LevelLoader | ✅ Level JSON design, entity configs |
| **5** | Graphics & Animation | ✅ Owns all (5.1–5.11) | — wire sprites to entities |
| **6** | Audio | ✅ Owns all | — |
| **7** | Game States & UI | ✅ MenuState shell, OptionsState | ✅ PlayingState gameplay, CharSelect, WorldMap, Pause, GameOver, Victory |
| **8** | Save/Load | — | ✅ Owns all (Serializer, Achievements, Stats) |
| **9** | Enemy AI & Bosses | — | ✅ Owns all |
| **10** | Advanced Systems | ✅ ObjectPool, DebugConsole | ✅ Config-driven entities, Replay |
| **11** | Polish & 2P | ✅ 2P camera, perf | ✅ 2P gameplay, meta-game, accessibility |

---

## Git Strategy (Critical)

### Branch Naming Convention

```
dev                          ← integration branch (both merge here)
├── A/feature/core-engine    ← Member A's branches start with A/
├── A/feature/physics
├── A/feature/graphics
├── B/feature/entities       ← Member B's branches start with B/
├── B/feature/enemy-ai
├── B/feature/game-states
└── ...
```

### Rules

1. **Never both edit the same file** in parallel. If unavoidable, coordinate synchronously.
2. **Merge to `dev` frequently** — at least once per completed sub-phase (e.g., after 1.2, not after all of Phase 1).
3. **Pull from `dev` before starting any new branch**.
4. **CMakeLists.txt is the one shared file** — only add your own source files, append at the bottom of the sources list, never reorder.

---

## Shared Interface Contracts

> [!IMPORTANT]
> These interfaces must be agreed upon FIRST, before parallel work begins. Member A defines them, Member B implements against them.

### 1. Entity Base (Member A defines, Member B implements subclasses)

```cpp
// A defines this contract:
class Entity {
public:
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;
    virtual AABB getBoundingBox() const = 0;
    virtual bool isActive() const = 0;
    virtual void destroy() = 0;
    virtual ~Entity() = default;
    
    sf::Vector2f position;
    sf::Vector2f velocity;
    bool active = true;
};
```

### 2. Game State Interface (Member A defines, Member B implements concrete states)

```cpp
// A defines this contract:
class IGameState {
public:
    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleInput(sf::Event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget&) = 0;
    virtual ~IGameState() = default;
};
```

### 3. EventBus Events (Member A defines enum + publish/subscribe, Member B adds event types)

```cpp
// A defines the mechanism:
enum class EventType { /* both members can add entries here */ };
struct GameEvent { EventType type; std::any data; };
// B fires events from entities, A's systems (HUD, Sound) react to them.
```

### 4. Movement Strategy Interface (Member A defines, Member B implements all 8)

```cpp
// A defines:
class IMovementStrategy {
public:
    virtual void execute(Enemy& enemy, float dt) = 0;
    virtual ~IMovementStrategy() = default;
};
```

### 5. Physics Integration Points

```
Member A provides:
  - PhysicsEngine::update(entities, tileMap, dt)  ← takes Entity* vector
  - CollisionDetector::check(Entity&, Entity&)
  - CollisionResolver::resolve(...)

Member B provides:
  - Entity subclasses with correct bounding boxes
  - onStomped(), onHitByFireball() virtual overrides
  - Collision response behavior per entity type
```

---

## Files That MUST Be Coordinated

These are **conflict hotspots** — never edit simultaneously:

| File | Who edits when |
|:---|:---|
| `CMakeLists.txt` | Both — but append only, never reorder |
| `Constants.hpp` | Member A initially, then append-only by either |
| `EventBus.hpp` (EventType enum) | Both — but only add new entries at the end |
| `EntityFactory.cpp` (registration) | Member B primarily — Member A doesn't touch |
| `PlayingState.cpp` | Member B primarily — Member A provides hooks only |

---

## Communication Checkpoints

| When | What to sync |
|:---|:---|
| **Before Phase 1** | Agree on all interface contracts above |
| **After Phase 1 + 2** | A demos engine running. B verifies entity interfaces work. |
| **After Phase 3** | B demos entities compiling against A's interfaces. Integration test. |
| **After Phase 4** | First playable: Mario walks on tiles, camera follows. Both verify. |
| **After Phase 7** | Full game loop: menus → play → pause → game over. Both verify. |
| **After Phase 9** | All enemies functional. Full gameplay test. |
| **After Phase 11** | Final integration, polish, 2P test. |

---

## Parallel Work Timeline

```
Week 1:  A: Phase 1 (Core)          B: Phase 3.1-3.7 (Entity bases, strategies)
         ─── sync: interfaces ───
         
Week 2:  A: Phase 2 (Physics)       B: Phase 3.8-3.14 (Concrete entities, Factory)
         ─── sync: integration test ───

Week 3:  A: Phase 4 (TileMap/Cam)   B: Phase 4.4 (Level JSONs) + Phase 9 (AI tuning)
         A: Phase 5 (Graphics)      B: Phase 7 (Game States)
         ─── sync: first playable ───

Week 4:  A: Phase 6 (Audio)         B: Phase 8 (Save/Load)
         A: Phase 10 (Pool/Debug)   B: Phase 9 (Bosses)
         ─── sync: feature complete ───

Week 5:  A: Phase 11 (2P camera)    B: Phase 11 (2P gameplay, meta)
         ─── final integration & polish ───
```

---

## Quick Decision Rules

| Situation | Rule |
|:---|:---|
| Need to add a new `EventType`? | Add at the END of the enum. Never reorder. |
| Need a new constant? | Add at the END of `Constants.hpp`. |
| Need to register a new entity in Factory? | Only Member B touches `EntityFactory.cpp`. |
| Found a bug in the other person's layer? | File a GitHub Issue, don't fix it yourself. |
| Need to change an interface contract? | Discuss first, both agree, then one person changes it. |
