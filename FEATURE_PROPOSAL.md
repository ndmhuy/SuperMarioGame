# CS202 Final Project — Feature Expansion Proposal

> **Student**: Nguyễn Đình Minh Huy (25125083)
> **Project**: Super Mario Bros. — 2D Platformer in C++17
> **Date**: June 2, 2026
> **Purpose**: Justification for adopting an expanded feature set (110 features) over the initial project specification (51 features).

---

## 1. Executive Summary

I am requesting approval to implement an **expanded specification** for my CS202 Final Project that contains **110 distinct features** — more than **double** the initial 51-feature scope. This expansion is not arbitrary padding; every new feature directly deepens the application of **Object-Oriented Programming principles**, introduces additional **Software Design Patterns**, and creates opportunities to demonstrate **advanced C++ and algorithmic techniques** that go significantly beyond the baseline rubric requirements.

The expanded scope transforms the project from a competent Mario clone into a **production-quality game engine** that showcases mastery of software architecture, data structures, and systems design — all while remaining implementable within the project timeline.

---

## 2. Feature Comparison: Initial vs. Expanded

| Category | Initial Spec | Expanded Spec | Growth |
|----------|:---:|:---:|:---:|
| Core Engine & Architecture | 6 | 6 | — |
| Player Characters & States | 5 | 5 | — |
| **Gameplay Mechanics** | 0 | **10** | **+10 new** |
| Enemies & AI | 6 | **14** | **+8 new** |
| Items & Power-ups | 5 | **11** | **+6 new** |
| Blocks & World Objects | 4 | **9** | **+5 new** |
| Levels & World Design | 4 | **10** | **+6 new** |
| Graphics & Visual Effects | 6 | **12** | **+6 new** |
| Audio | 3 | 3 | — |
| **UI/UX & Menus** | 0 | **6** | **+6 new** |
| Save/Load & Persistence | 3 | 3 | — |
| Multiplayer | 2 | 2 | — |
| Dev Tools | 2 | 2 | — |
| **Advanced Systems** | 0 | **5** | **+5 new** |
| **Accessibility** | 0 | **3** | **+3 new** |
| **Meta-Game & Progression** | 0 | **4** | **+4 new** |
| Post-MVP Bonus | 5 | 5 | — |
| **TOTAL** | **51** | **110** | **+59 (+116%)** |

---

## 3. Why the Expanded Scope Deserves Approval

### 3.1 Dramatically Deeper OOP Hierarchy

The initial spec defines a clean but **shallow** inheritance tree:

```
Entity → Character → Mario, Luigi
Entity → Character → Enemy → Goomba, KoopaTroopa, Paratroopa, Boo, Bowser
Entity → Item → 5 item types
Entity → Block → 4 block types
```

The expanded spec creates a **significantly deeper and wider** hierarchy:

```
Entity (abstract)
├── Character (abstract)
│   ├── Mario
│   ├── Luigi
│   ├── Toad (unlockable — speed specialization)
│   └── Peach (unlockable — float ability)
│
├── Enemy (abstract)
│   ├── Goomba (+color variants with different AI)
│   ├── KoopaTroopa (+Red variant: ledge-aware AI)
│   ├── KoopaParatroopa (+Red variant: vertical bounce)
│   ├── Boo
│   ├── Bowser (multi-phase boss)
│   ├── PiranhaPlant (timer-based emergence AI)
│   ├── BulletBill (projectile-entity hybrid)
│   ├── HammerBro (arc-throwing AI + platform jumping)
│   ├── Thwomp (proximity-triggered state machine)
│   ├── ChainChomp (tethered physics constraint)
│   ├── Lakitu (spawner-enemy with Factory pattern)
│   ├── Spiny (spawned projectile-enemy)
│   └── BoomBoom (mid-boss with 3-hit phases)
│
├── Item (abstract)
│   ├── Mushroom, FireFlower, Coin, Star, OneUpMushroom (existing)
│   ├── CapeFeather (grants flight/glide — new physics mode)
│   ├── MegaMushroom (temporary giant state — decorator)
│   ├── MiniMushroom (half-size state — modified collision)
│   ├── POWBlock (broadcast area-of-effect)
│   ├── PSwitch (timed tile transformation — reversible command)
│   └── Trampoline (physics boost — holdable/moveable)
│
└── Block (abstract)
    ├── BrickBlock, QuestionBlock, Pipe, Flagpole (existing)
    ├── MovingPlatform (velocity + parent-motion physics)
    ├── FallingPlatform (4-state lifecycle: Stable→Shaking→Falling→Respawning)
    ├── IceBlock (friction modifier per tile type)
    ├── HiddenBlock (invisible until activated)
    └── ConveyorBelt (directional force on all entities)
```

**Academic value**: This hierarchy demonstrates:
- **4 levels of inheritance depth** (Entity → Character → Enemy → specific enemy)
- **20+ concrete leaf classes** (vs. 14 in the initial spec)
- **Polymorphism** across 4 abstract base classes
- **Multiple specialization strategies** within the same hierarchy (color variants, phase-based bosses)

---

### 3.2 Expanded Design Pattern Coverage

The initial spec demonstrates **6 design patterns**. The expanded scope exercises **10+ distinct patterns**, with several patterns used in multiple, non-trivial contexts:

| Pattern | Initial Spec Usage | Expanded Spec Usage |
|---------|-------------------|---------------------|
| **Factory** | EntityFactory creates 14 types | EntityFactory creates **25+ types**; Lakitu uses Factory to spawn Spinies at runtime |
| **Singleton** | Game, ResourceManager, SoundManager | Same + **AchievementManager**, **StatisticsTracker** |
| **State** | GameStateManager (6 states), PlayerState (4 forms) | GameStateManager (**8+ states** incl. WorldMap, Statistics), PlayerState (**7 forms**: +Cape, Mega, Mini), **FallingPlatform** (4-state lifecycle), **Thwomp** (3-state AI) |
| **Observer** | EventBus with 4 event types | EventBus with **15+ event types**; **ComboTracker**, **AchievementTracker**, **StatisticsTracker** all subscribe independently |
| **Strategy** | 3 movement strategies (Patrol, Chase, Fly) | **7+ strategies**: +SwimMovement, TetheredChase, HammerThrow, TimerEmergence; **DifficultyStrategy** for game-wide scaling; **color variants** swap strategies |
| **Command** | 5 input commands + undo/redo | **8+ commands**: +GroundPoundCommand, WallJumpCommand, CrouchCommand; **Debug console** parses text→commands; **Replay system** serializes command streams |
| **Decorator** | *(not used)* | **Star overlay**, **Mega Mushroom** as temporary state decorators on base player state |
| **Memento** | *(post-MVP only)* | **Time Rewind** + **Replay System** both use state snapshots |
| **Object Pool** | *(not used)* | **Fireball pool**, **Particle pool**, **Enemy respawn pool** — demonstrates memory management |
| **Template Method** | *(not used)* | **Enemy base class** defines `update()` skeleton: `applyStrategy() → checkBounds() → animate()`, subclasses override hooks |

**Academic value**: The initial spec demonstrates 6 patterns in isolated contexts. The expanded spec demonstrates **10+ patterns** with **cross-cutting interactions** (e.g., Observer triggers Factory which creates entities managed by Strategy, all logged by Command). This showcases **pattern composition**, which is a graduate-level software engineering concept.

---

### 3.3 Advanced C++ and Data Structure Techniques

The expanded features require C++ techniques that go beyond basic OOP:

| Technique | Feature That Requires It |
|-----------|------------------------|
| **Spatial hashing** (grid-based collision broadphase) | 25+ entity types need O(n) collision instead of O(n²) |
| **Object pooling** (pre-allocated memory recycling) | Fireballs, particles, Bullet Bills — avoids allocation in hot loops |
| **Circular buffer** (fixed-size ring buffer) | Time rewind stores 300 frames of game state snapshots |
| **Serialization of polymorphic hierarchies** | Save/load must serialize 25+ entity types through base `Entity*` |
| **Config-driven architecture** (JSON → runtime objects) | All entity definitions externalized — demonstrates data-driven design |
| **GLSL fragment shaders** | Dynamic lighting, invincibility rainbow effect |
| **State machine composition** | Player state × power-up state × movement state (e.g., Fire+Swimming+WallSliding) |
| **Acceleration curves** (non-linear physics) | Momentum system with separate air/ground friction coefficients |
| **Procedural generation** (chunk-based with constraints) | Endless mode generates valid, playable terrain |
| **Input replay determinism** | Replay system requires frame-perfect deterministic physics |

---

### 3.4 Rubric Alignment — Maximizing Every Category

Here is how the expanded spec **maximizes scoring** in every rubric category:

#### Functionality (65 points)

| Criterion | Points | Initial Coverage | Expanded Coverage |
|-----------|:------:|-----------------|------------------|
| Player Inputs, Movement, Collision | 20 | Walk, run, jump, fireball. AABB collision. | **+Wall jump, ground pound, crouch/slide, swimming, climbing, coyote time, momentum curves, damage knockback.** 10 movement mechanics vs. 4. |
| Enemy Behavior | 10 | 5 enemies with 3 AI strategies. | **13 enemies with 7+ AI strategies**, color variants with different behaviors, mid-boss, multi-phase final boss. |
| Power-Ups and Items | 10 | 5 items, 4 player states. | **11 items, 7 player states** (Small, Super, Fire, Star, Cape, Mega, Mini). P-Switch tile transformation. POW Block broadcast. |
| 3 Level Completion | 15 | 3 themed levels, checkpoints, flagpole. | **3 core levels + bonus rooms, vertical sections, autoscroll sections, world map, star coins** (3 per level). |
| Sounds | 10 | 10+ SFX, per-level BGM, volume sliders. | **+Dynamic music layering, surface-dependent footsteps, combo SFX escalation, achievement notification sounds.** |

#### Design and Implementation (35 points)

| Criterion | Points | Initial Coverage | Expanded Coverage |
|-----------|:------:|-----------------|------------------|
| OOP Design | 10 | 4 abstract bases, 14 concrete classes, SOLID principles. | **4 abstract bases, 25+ concrete classes**, Template Method in Enemy, Decorator for power-ups, **deeper polymorphism**. |
| 5 Design Patterns | 25 | 6 patterns (5 required + 1 bonus). | **10+ patterns** with cross-cutting interactions. Each pattern used in **multiple distinct contexts**. |

#### Additional Requirements (15 points)

| Criterion | Points | Initial Coverage | Expanded Coverage |
|-----------|:------:|-----------------|------------------|
| AI | 5 | 3 strategies, Boo proximity, boss phases. | **7+ strategies**, Lakitu spawner AI, HammerBro arc-throwing, Thwomp state machine, **config-driven variant behaviors**. |
| Multiple Players | 5 | Mario + Luigi, character select, 2P versus. | **4 characters** (Mario, Luigi, Toad, Peach), each with unique abilities. Unlockable progression. |
| 3D Game | 5 | Not implemented (2D only). | Still 2D, but **GLSL shader effects** (dynamic lighting, color cycling) demonstrate GPU programming. |

#### Creativity and Originality

The expanded spec includes features **not found in the reference games** (SMB 1985, NSMB DS):
- **Combo scoring system** with escalating multipliers
- **Achievement/statistics system** with persistent tracking
- **Difficulty modes** (Easy/Normal/Hard) with Strategy pattern
- **Speed run timer** with personal best tracking
- **Debug console** with text-to-command parsing
- **Colorblind accessibility** mode
- **New Game+** with mirrored levels and increased difficulty
- **Daily challenge** with date-seeded procedural generation

---

### 3.5 Software Engineering Best Practices

The expanded scope necessitates practices that demonstrate professional software engineering maturity:

| Practice | How It's Applied |
|----------|-----------------|
| **Separation of Concerns** | Physics, rendering, AI, input, audio are fully decoupled layers |
| **Data-Driven Design** | Entity definitions, level layouts, and game balance are all in external JSON — no recompilation to tweak |
| **Performance Optimization** | Spatial hashing, object pooling, tile culling, entity deactivation outside camera |
| **Accessibility** | Colorblind mode, difficulty scaling, audio cues for menu navigation |
| **Configuration Management** | Key rebinding, volume persistence, difficulty settings, save slots |
| **Testability** | ImGui debug overlay, debug console, entity inspector, AI state visualization |

---

## 4. Implementation Feasibility

Despite the 2.16× feature increase, the expanded scope remains feasible because:

1. **Modular architecture**: The layered engine design means new entities, items, and blocks are added by subclassing — not by modifying existing code (Open/Closed Principle).

2. **Pattern reuse**: New features leverage existing infrastructure. Adding Piranha Plant is just a new `Enemy` subclass + a new `IMovementStrategy`. The Factory, Observer, and rendering pipeline already handle it.

3. **Incremental complexity**: Features are tiered by priority. Core gameplay (Tier 1: wall jump, ground pound, new enemies) comes first. Meta-features (Tier 3-4: achievements, replays) build on top of stable core systems.

4. **Proven architecture**: The 6 design patterns from the initial spec provide the scaffolding for all new features. No architectural rewrites are needed.

---

## 5. Summary: Why Expanded > Initial

| Dimension | Initial (51 features) | Expanded (110 features) |
|-----------|:---:|:---:|
| Concrete entity classes | 14 | **25+** |
| Design patterns | 6 | **10+** |
| AI strategies | 3 | **7+** |
| Player states/forms | 4 | **7** |
| Advanced C++ techniques | 3-4 | **10+** |
| Levels of inheritance | 3 | **4** |
| Event types (Observer) | 4 | **15+** |
| Command types | 5 | **8+** |
| Rubric coverage | 105/115 | **110/115** |

The expanded specification transforms this project from a **requirement-satisfying submission** into a **portfolio-quality game engine** that demonstrates mastery across every dimension the rubric evaluates. Every new feature exists to deepen OOP application, add design pattern complexity, or showcase advanced C++ techniques — there is no filler.

I respectfully request approval to proceed with this expanded scope.

---

**Nguyễn Đình Minh Huy**
CS202 — Object-Oriented Programming
June 2026
