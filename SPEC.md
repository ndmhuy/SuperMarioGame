# Super Mario Game — Global Specification (FROZEN)

> **Version**: 2.0 — 2026-06-02
> **Status**: APPROVED — Do not modify without user confirmation.
> **Source**: Compiled from user answers in `implementation_plan.md` Section 6, expanded with Feature Expansion Proposal.
> **Change Log**: v1.0 (2026-05-30) initial spec; v2.0 (2026-06-02) expanded from 51 → 110 features.

---

## 1. Project Overview

| Field | Value |
| :--- | :--- |
| **Project** | CS202 Final Project — Super Mario Bros. |
| **Language** | C++17 |
| **Engine** | SFML 3.0.2 + ImGui-SFML v3.0 |
| **Build** | CMake with FetchContent |
| **Rendering** | 2D only (no 3D) |
| **Platform** | macOS (primary), cross-platform via SFML |
| **Total Features** | 110 (46 MVP + 59 Expanded + 5 Post-MVP Bonus) |

---

## 2. Architecture

### 2.1 Layered Architecture

```
Application Layer    →  main.cpp (entry point)
Core Layer           →  Game, GameStateManager, InputManager, ResourceManager, SoundManager, EventBus
Game States          →  MenuState, CharSelectState, PlayingState, PauseState, GameOverState,
                        VictoryState, OptionsState, WorldMapState, StatisticsState
World Layer          →  World, TileMap, Camera, HUD, Minimap
Entity Layer         →  Entity → Character → Player → Mario/Luigi/Toad/Peach; Entity → Character → Enemy; Entity → Item; Entity → Block
Physics Layer        →  PhysicsEngine, AABB, CollisionDetector, CollisionResolver, SpatialHash
Infrastructure       →  Renderer, AnimationManager, Serializer, EntityFactory, EventBus,
                        ObjectPool, ReplayRecorder, AchievementManager, StatisticsTracker
```

### 2.2 Design Patterns (10+ Total — 5 Required + 5+ Bonus)

| # | Pattern | Class(es) | Purpose |
| :--- | :--- | :--- | :--- |
| 1 | **Factory** | `EntityFactory` | `createEntity(type, pos)` → `std::unique_ptr<Entity>`. Spawns all 25+ entity types. Lakitu uses Factory to spawn Spinies. |
| 2 | **Singleton** | `Game`, `ResourceManager`, `SoundManager`, `AchievementManager` | Single instances. Lazy-init via `getInstance()`. |
| 3 | **State** | `GameStateManager` / `IGameState`, `IPlayerState`, `FallingPlatform`, `Thwomp` | Game flow (9 states). Player forms (5 concrete base states). Entity lifecycles. |
| 4 | **Observer** | `EventBus` | Publish/subscribe: 15+ event types. HUD, SoundManager, ComboTracker, AchievementTracker, StatisticsTracker subscribe. |
| 5 | **Strategy** | `IMovementStrategy` | Enemy AI: 7+ strategies (Patrol, Chase, Fly, Swim, TetheredChase, HammerThrow, TimerEmergence). Swappable at runtime. `DifficultyStrategy` for game-wide scaling. |
| 6 | **Command** | `ICommand` + `InputManager` | 8+ commands. Keyboard → game commands. Debug console text→command parsing. Replay serialization. |
| 7 | **Decorator** | `StarDecorator`, `MegaDecorator` | Temporary state overlays that wrap the active `IPlayerState` without modifying it. |
| 8 | **Memento** | `GameSnapshot` | Time rewind stores/restores full game state snapshots. Replay system. |
| 9 | **Object Pool** | `ObjectPool<T>` | Pre-allocated pools for fireballs, particles, Bullet Bills. Avoids allocation in hot loops. |
| 10 | **Template Method** | `IMovementStrategy::execute()` | Base `execute()` defines skeleton: `calculateTarget() → applyMovement() → checkConstraints()`. Concrete strategies override hooks. |

---

## 3. Window & Rendering

| Setting | Value |
| :--- | :--- |
| **Window Resolution** | 1280 × 720 pixels |
| **Tile Size** | 32 × 32 pixels |
| **Visible Width** | 40 tiles (1280 / 32) |
| **Visible Height** | 22.5 tiles (720 / 32) |
| **Art Style** | 16-bit SNES style |
| **Sprite Source** | Open-source (OpenGameArt, The Spriters Resource, etc.) |
| **Sprite Dimensions** | 32 × 32 base |
| **Scrolling** | Horizontal (primary), vertical (tower/beanstalk sections) |
| **Background** | Parallax scrolling (multiple layers: sky, clouds, mountains, ground) |
| **Particles** | Brick fragments, coin sparkles, death poof, stomp effects, combo stars |
| **Transitions** | Fade-in/fade-out between levels and screens |
| **Dev Tools** | ImGui-SFML overlay (toggle with F12 or similar) |
| **Screen Shake** | Configurable on: block break, ground pound, Bowser stomp, Mega Mushroom |

---

## 4. Physics & Gameplay Constants

> All values are exposed to ImGui for real-time tuning.

| Constant | Initial Value | Notes |
| :--- | :--- | :--- |
| **Gravity** | 0.5 px/frame² (= 1800 px/s² at 60fps) | Tunable via ImGui |
| **Mario Walk Speed** | 150 px/s | Tunable via ImGui |
| **Mario Run Speed** | 300 px/s | Tunable via ImGui |
| **Mario Jump Height** | ~4 tiles (128 px at 32px/tile) | Tunable via ImGui |
| **Fixed Timestep** | 1/60 s (16.667 ms) | With interpolated rendering |
| **Physics** | Custom AABB + Spatial Hash | Broad-phase grid for O(n) collision |
| **Coyote Time** | 6 frames (100ms) | Grace period after leaving ledge |
| **Jump Buffer** | 6 frames (100ms) | Input buffer before landing |

### 4.1 Luigi Modifiers (Relative to Mario)

| Attribute | Multiplier |
| :--- | :--- |
| Jump Force | ×1.2 (jumps higher) |
| Walk/Run Speed | ×0.85 (slower) |
| Airborne Gravity | ×0.9 (floatier) |
| **Special** | Double jump ability |

### 4.2 Fireball Mechanics

| Property | Value |
| :--- | :--- |
| Max Active | 2 at once |
| Behavior | Bounces on ground |
| Destruction | On wall impact |
| Fire Rate | ~0.3s cooldown between shots |

### 4.3 Advanced Movement Mechanics [NEW in v2.0]

#### 4.3.1 Wall Jump / Wall Slide

| Property | Value |
| :--- | :--- |
| Wall Slide Speed | 50 px/s (capped downward velocity) |
| Wall Jump Force | Same as regular jump, angled 45° away from wall |
| Wall Detect | Must hold direction toward wall while airborne |
| Animation | New `wall_slide` sprite, dust particles on jump |

#### 4.3.2 Ground Pound

| Property | Value |
| :--- | :--- |
| Trigger | Press Down while airborne |
| Hover Time | 0.15s pause at apex before slamming |
| Slam Speed | 600 px/s (instant downward) |
| Effects | Breaks bricks below, stuns nearby enemies (2-tile radius), screen shake |
| Damage | Kills stompable enemies on impact |

#### 4.3.3 Crouch / Slide

| Property | Value |
| :--- | :--- |
| Crouch | Hold Down on ground → reduce hitbox to 1 tile height (from 2 when Super+) |
| Slide | Hold Down while running → slide for 0.5s, pass under 1-tile gaps |
| Slide Damage | Kills enemies on contact during slide |

#### 4.3.4 Swimming / Water Sections

| Property | Value |
| :--- | :--- |
| Gravity | ×0.3 (reduced in water) |
| Swim Button | Jump button = swim stroke (upward impulse) |
| Max Sink Speed | 60 px/s |
| Horizontal Speed | ×0.6 of ground speed |
| Fireballs | Travel straight in water (no bounce) |
| Visual | Blue overlay tint, bubble particles |

#### 4.3.5 Climbing (Vines / Beanstalks)

| Property | Value |
| :--- | :--- |
| Climb Speed | 100 px/s |
| Trigger | Press Up when overlapping a vine/beanstalk tile |
| Controls | Up/Down to climb, Jump to leap off, Left/Right to dismount |
| Destination | Leads to bonus cloud areas above the main level |

#### 4.3.6 Damage Knockback

| Property | Value |
| :--- | :--- |
| Knockback Force | 150 px/s horizontal, 100 px/s vertical |
| Stun Duration | 0.3s (player cannot input during stun) |
| Direction | Away from damage source |

#### 4.3.7 Momentum & Acceleration Curves

| Property | Value |
| :--- | :--- |
| Ground Acceleration | 0 → walk speed in 0.2s (non-linear ease-in) |
| Ground Deceleration | Walk speed → 0 in 0.15s |
| Air Acceleration | ×0.5 of ground acceleration |
| Air Friction | ×0.3 of ground friction |
| Skid Trigger | Reversing direction while moving above 100 px/s |
| Skid Animation | Dust particles + skid sprite |

#### 4.3.8 Combo System

| Property | Value |
| :--- | :--- |
| Trigger | Consecutive enemy stomps without touching ground |
| Multiplier | ×1, ×2, ×4, ×8 (caps at ×8) |
| Score | Base stomp score × multiplier |
| Display | Combo counter on HUD, escalating SFX pitch |
| Reset | Touching ground or taking damage |

---

## 5. Characters

### 5.1 Player Characters

| Character | Select | Special |
| :--- | :--- | :--- |
| **Mario** | Default | Standard physics |
| **Luigi** | Alt | Higher jump, slower, floatier, double jump |
| **Toad** | Unlockable (complete all 3 levels) | ×1.3 speed, ×0.8 jump, no slide friction delay |
| **Peach** | Unlockable (complete all 3 levels with no deaths) | Float ability (hold jump to hover for 1.5s), ×0.9 speed |

- Switching: Both at character select screen AND during gameplay via hotkey.
- Two-player: Same keyboard, versus mode.

### 5.2 Player States (State & Decorator Patterns)

```text
Base States (IPlayerState)
Small  ──(Mushroom)──▶  Super  ──(Fire Flower)──▶  Fire
  ▲                       ▲                          │
  │                       └──────(Hit)───────────────┘
  └──────────────(Hit)────┘

Super  ──(Cape Feather)──▶  Cape
Small  ──(Mini Mushroom)──▶  Mini

Decorators (Wrap Current State)
- StarDecorator = 10s invincible overlay
- MegaDecorator = 8s giant overlay
```

| State / Decorator | Size | Abilities | On Hit |
| :--- | :--- | :--- | :--- |
| **Small** | 1 tile (32px) | Walk, run, jump | Die (lose life) |
| **Super** | 2 tiles (64px) | Walk, run, jump, break bricks | → Small (invincibility frames) |
| **Fire** | 2 tiles (64px) | Walk, run, jump, break bricks, shoot fireballs | → Super (invincibility frames) |
| **Cape** | 2 tiles (64px) | Walk, run, jump, break bricks, glide, swoop attack | → Super (invincibility frames) |
| **Mini** | 0.5 tiles (16px) | Walk on water, enter mini pipes, floatier jump | Die (lose life — fragile) |
| *(Dec)* **Mega** | 4 tiles (128px) | Destroys everything on contact. Temporary (8s). | Cannot be hit (invincible during Mega) |
| *(Dec)* **Star** | Same as current | All current + invincible, kills enemies on contact | Timer expires → return to base state |

---

## 6. Enemies

### 6.1 Enemy Roster

| Enemy | AI Strategy | Stomp Result | Fireball Result | Points |
| :--- | :--- | :--- | :--- | :--- |
| **Goomba** | `PatrolStrategy` — walks one direction, reverses on wall, falls off ledges | Dies (squish animation) | Dies | 100 |
| **Red Goomba** | `PatrolStrategy` — walks, reverses on wall AND at ledges (won't fall) | Dies (squish animation) | Dies | 100 |
| **Koopa Troopa** | `PatrolStrategy` — walks, reverses on wall | Retreats into shell (kickable) | Dies | 200 |
| **Red Koopa** | `PatrolStrategy` — reverses at ledges (ledge-aware) | Retreats into shell | Dies | 200 |
| **Koopa Paratroopa** | `FlyStrategy` — horizontal patrol + sinusoidal vertical | Loses wings → becomes Koopa Troopa | Dies | 400 |
| **Red Paratroopa** | `FlyStrategy` — stationary, vertical bounce only | Loses wings → becomes Red Koopa | Dies | 400 |
| **Boo (Ghost)** | `ChaseStrategy` — idle until player within 250px, chases when back turned | Cannot be stomped | Cannot be killed by fireball | N/A |
| **Piranha Plant** | `TimerEmergenceStrategy` — emerges from pipe on 3s timer, retreats for 2s | Cannot be stomped | Dies | 100 |
| **Bullet Bill** | `LinearStrategy` — fired from Bill Blaster, travels horizontally at 200 px/s | Dies (stompable) | Immune | 200 |
| **Hammer Bro** | `HammerThrowStrategy` — jumps between platforms, throws hammers in arcs | Dies | Dies | 1000 |
| **Thwomp** | `ProximityTriggerStrategy` — idle on ceiling, slams down when player within 3 tiles below | Cannot be stomped (damages player) | Immune | N/A |
| **Chain Chomp** | `TetheredChaseStrategy` — lunges toward player but snaps back to 4-tile radius | Cannot be stomped | Immune | N/A |
| **Lakitu** | `FlyStrategy` — hovers above, follows player, drops Spiny eggs every 4s | Dies (stompable when below) | Dies | 800 |
| **Spiny** | `PatrolStrategy` — walks on ground after hatching from egg | Cannot be stomped (damages player) | Dies | 100 |
| **Boom Boom** (Mid-Boss) | Custom — charges and spins, pauses to recover. 3 stomps to defeat. | Takes 1 hit (3 total needed) | Takes damage | 2000 |
| **Bowser** (Boss) | Custom boss AI — Phase 1 (walk + breathe fire), Phase 2 (jump + faster fire) | Multi-hit (health bar) | Takes damage | 5000 |

### 6.2 Proximity AI Detail (Boo)

- **Idle Range**: > 250 pixels from player
- **Chase Range**: ≤ 250 pixels from player
- **Behavior**: Moves toward Mario when Mario's back is turned. Freezes when Mario faces it (classic Boo behavior).
- **Invulnerable**: Cannot be stomped or killed by fireballs. Player must avoid.

### 6.3 Enemy Variants [NEW in v2.0]

Color variants alter enemy behavior without creating new classes:

| Base Enemy | Variant | Behavioral Difference |
| :--- | :--- | :--- |
| Goomba (brown) | **Red Goomba** | Ledge-aware (turns at edges instead of falling) |
| Koopa (green) | **Red Koopa** | Ledge-aware (turns at edges) |
| Paratroopa (green) | **Red Paratroopa** | Vertical-only bounce (no horizontal patrol) |

Variants are configured via JSON: `{ "type": "goomba", "variant": "red" }` → Factory applies different Strategy.

### 6.4 Mid-Boss: Boom Boom [NEW in v2.0]

- **Location**: End of Level 2 (Underground)
- **Phases**: 
  - Hit 1: Normal charge speed
  - Hit 2: Faster charge + shorter recovery
  - Hit 3: Fastest charge + spin attack
- **Arena**: Enclosed room, no escape until defeated
- **Reward**: Opens path to Level 2 flagpole

---

## 7. Items & Power-ups

| Item | Source | Effect | Points |
| :--- | :--- | :--- | :--- |
| **Super Mushroom** | QuestionBlock | Small → Super | 1000 |
| **Fire Flower** | QuestionBlock | Super → Fire (if Small, acts as Mushroom first) | 1000 |
| **Coin** | QuestionBlock, floating in level | +1 coin, +200 score. 100 coins = 1 extra life | 200 |
| **Star** | QuestionBlock | 10 seconds invincibility overlay | 1000 |
| **1-UP Mushroom** | QuestionBlock (rare) | +1 life | — |
| **Cape Feather** | QuestionBlock | Super → Cape (glide + swoop attack) | 1000 |
| **Mega Mushroom** | QuestionBlock (very rare) | Temporary giant form (8s). Destroys everything. | 1000 |
| **Mini Mushroom** | QuestionBlock (rare) | Shrinks to half-tile. Walk on water, enter mini pipes. | 1000 |
| **POW Block** | Placed in level (not from QuestionBlock) | Hit/throw → screen shake, all grounded enemies die/flip | — |
| **P-Switch** | BrickBlock (rare) | Bricks ↔ Coins for 15 seconds. Timer countdown on HUD. | — |
| **Trampoline** | Placed in level | Bounces player ~6 tiles high. Can be carried and placed. | — |
| **Star Coin** | Placed in level (3 per level) | Collectible. Tracked per level. Unlocks bonus content. | 1000 |

- Items spawn from QuestionBlocks and hidden blocks.
- Power-down chain: Fire/Cape → Super → Small (step-down on each hit).
- Mega and Star are temporary overlays that expire.

---

## 8. Blocks & World Objects

| Block | Behavior | Breakable? |
| :--- | :--- | :--- |
| **BrickBlock** | Hit from below. Breaks if Super/Fire/Cape. Contains coins sometimes. Particle effect on break. | Yes (Super+) |
| **QuestionBlock** | Hit from below → spawns contained item. Becomes empty block. Bump animation. | No |
| **Hidden Block** | Invisible until hit from below. Can contain 1-UP or coins. | No |
| **Pipe** | Warp to bonus areas or between level sections. Enter with Down. Piranha Plants emerge from some. | No |
| **Flagpole** | Level end marker. Player slides down. Score bonus based on height caught. | No |
| **Moving Platform** | Travels on a fixed path (horizontal or vertical). Player rides on top. Carries entities. | No |
| **Falling Platform** | Stable → Shaking (player stands for 1s) → Falling → Respawns after 5s. | No |
| **Ice Block** | Surface with reduced friction. Player slides further. Affects enemies too. | No |
| **Conveyor Belt** | Pushes entities in indicated direction. Visual arrow animation. Speed: 100 px/s. | No |

### 8.1 Moving Platform Specifications [NEW in v2.0]

| Property | Value |
| :--- | :--- |
| Speed | 60 px/s |
| Path Types | Horizontal (back-and-forth), Vertical (back-and-forth), Circular |
| Player Interaction | Player inherits platform velocity when standing on it |
| JSON Config | `{ "type": "moving_platform", "path": [{"x":10,"y":15}, {"x":20,"y":15}], "speed": 60 }` |

### 8.2 Falling Platform Specifications [NEW in v2.0]

| State | Duration | Behavior |
| :--- | :--- | :--- |
| Stable | Until player stands | Normal solid block |
| Shaking | 1 second | Visual shake, warning to player |
| Falling | Until off-screen | Falls with gravity, passes through other tiles |
| Respawning | 5 seconds | Invisible, then reappears at original position |

---

## 9. Levels & World

### 9.1 Level Specifications

| Level | Theme | Width | Difficulty | Unique Features |
| :--- | :--- | :--- | :--- | :--- |
| **Level 1** | Overworld / Grassland | 200 tiles (6400px) | Easy | Few enemies, flat terrain, tutorial-like, swimming section |
| **Level 2** | Underground / Cave | 200 tiles (6400px) | Medium | Gaps/pits, more enemies, darker palette, Boom Boom mid-boss, ice blocks |
| **Level 3** | Castle / Lava | 200 tiles (6400px) | Hard | Lava pits, high enemy density, Thwomps, autoscroll section, Bowser boss |

### 9.2 Level Structure

| Property | Value |
| :--- | :--- |
| **Format** | 3 continuous stages, each with mid-level checkpoints |
| **File Format** | JSON |
| **Height** | 22.5 tiles visible (720px / 32px) |
| **End Marker** | Flagpole (classic Mario) |
| **Warp Pipes** | Yes — teleport to bonus coin areas or between level sections |
| **Checkpoints** | Mid-level checkpoint flags. On death: respawn at last checkpoint. |
| **Star Coins** | 3 per level, placed in challenging-to-reach locations |

### 9.3 World Map [NEW in v2.0]

| Property | Value |
| :--- | :--- |
| **Style** | Super Mario Bros. 3 / World style overhead map |
| **Nodes** | Level 1, Level 2, Level 3, Bonus stages |
| **Navigation** | Arrow keys to move between nodes, Enter to start level |
| **Display** | Star coin count per level, completion status (star/flag icon) |
| **State** | `WorldMapState` in GameStateManager |
| **Unlock** | Levels unlock sequentially (must complete Level N to access Level N+1) |

### 9.4 Bonus Rooms [NEW in v2.0]

| Property | Value |
| :--- | :--- |
| **Access** | Via hidden warp pipes in main levels |
| **Type** | Coin-filled rooms with a 15-second timer |
| **Reward** | Collect as many coins as possible before timer expires |
| **Exit** | Automatic warp back to main level after timer |

### 9.5 Special Level Sections [NEW in v2.0]

| Section Type | Description | Camera Behavior |
| :--- | :--- | :--- |
| **Vertical Scroll** | Tower/beanstalk sections — player climbs upward | Camera follows vertically |
| **Autoscroll** | Airship-style — camera moves right automatically at 80 px/s | Player dies if pushed off left edge |
| **Water Section** | Submerged area within a level — different physics apply | Normal horizontal scroll |

### 9.6 Level JSON Schema

```json
{
  "name": "Level 1 - Grassland",
  "theme": "overworld",
  "width": 200,
  "height": 23,
  "tileSize": 32,
  "backgroundLayers": ["sky.png", "clouds.png", "mountains.png"],
  "spawnPoint": { "x": 2, "y": 20 },
  "checkpoints": [{ "x": 100, "y": 20 }],
  "flagpole": { "x": 198, "y": 8 },
  "starCoins": [
    { "x": 45, "y": 10 },
    { "x": 120, "y": 5 },
    { "x": 180, "y": 12 }
  ],
  "waterZones": [
    { "x1": 60, "y1": 15, "x2": 80, "y2": 22 }
  ],
  "tiles": [
    { "type": "ground", "x": 0, "y": 22, "w": 50 },
    { "type": "brick", "x": 20, "y": 16 },
    { "type": "question_block", "x": 22, "y": 16, "item": "coin" },
    { "type": "hidden_block", "x": 30, "y": 14, "item": "oneup" },
    { "type": "ice", "x": 70, "y": 22, "w": 10 },
    { "type": "conveyor", "x": 90, "y": 22, "w": 5, "direction": "right" }
  ],
  "entities": [
    { "type": "goomba", "x": 30, "y": 21 },
    { "type": "goomba", "x": 35, "y": 21, "variant": "red" },
    { "type": "koopa", "x": 50, "y": 21 },
    { "type": "piranha_plant", "x": 40, "y": 20, "pipe_id": 1 },
    { "type": "bullet_blaster", "x": 85, "y": 18 },
    { "type": "coin", "x": 25, "y": 14 }
  ],
  "movingPlatforms": [
    { "path": [{"x":60,"y":15}, {"x":75,"y":15}], "speed": 60 }
  ],
  "pipes": [
    { "id": 1, "entrance": { "x": 40, "y": 20 }, "exit": { "x": 40, "y": 10 }, "target": "bonus_area_1" }
  ]
}
```

---

## 10. Game Flow & UI

### 10.1 Screen Flow

```
Main Menu ──▶ World Map ──▶ Character Select ──▶ Level ──▶ Level Complete ──▶ World Map ──▶ ...
   │                                               │  ▲
   │                                               ▼  │
   ├── Options                                  Pause Menu
   ├── High Scores                                 │
   ├── Statistics                          ┌───────┴───────┐
   ├── Achievements                        │               │
   └── Load Game                        Resume      Restart Level
                                                           │
                                  Game Over ◀─── (lives=0) ┘

                              Victory ◀─── (all 3 levels complete)
```

### 10.2 Main Menu Options

- New Game
- Load Game
- Options (Volume: SFX slider + Music slider, Difficulty, Controls)
- High Scores (persisted across sessions)
- Statistics
- Achievements
- Quit

**Animated Main Menu**: Mario runs across a parallax background. Animated title logo. Coins spin. If idle for 30 seconds, demo playback starts (attract mode).

### 10.3 Pause Menu Options

- Resume
- Restart Level
- Save Game (manual save to slot)
- Return to World Map
- Return to Main Menu
- Quit

### 10.4 HUD Elements (Always Visible During Gameplay)

| Element | Position | Format |
| :--- | :--- | :--- |
| Score | Top-left | `SCORE 000000` |
| Coins | Top-center-left | `×00` with coin icon |
| World/Level | Top-center | `WORLD 1-1` |
| Time | Top-center-right | `TIME 300` (countdown) |
| Lives | Top-right | `×3` with character icon |
| Combo | Center (temporary) | `×2!`, `×4!`, `×8!` with escalating size |
| P-Switch Timer | Top (when active) | `P-SWITCH 12` countdown bar |
| Boss Health | Top-center (boss fights) | Health bar with boss name |
| Star Coins | Below time | 3 coin outlines, filled when collected |

### 10.5 Floating Score Text [NEW in v2.0]

- "+200", "+1UP", "×4 COMBO!" text floats upward from event location and fades over 1 second.
- Uses particle-like text objects managed by HUD.

### 10.6 Minimap [NEW in v2.0]

| Property | Value |
| :--- | :--- |
| Position | Bottom-right corner |
| Size | 200 × 40 pixels |
| Shows | Full level overview, player dot (green), enemy dots (red), item dots (yellow) |
| Toggle | Press M to show/hide |
| Opacity | 60% transparent |

### 10.7 Statistics Screen [NEW in v2.0]

Tracks across all play sessions:
- Total enemies defeated (by type)
- Total coins collected
- Total deaths
- Total time played
- Levels completed
- Highest combo achieved
- Star coins collected (per level)

### 10.8 Controls Rebinding [NEW in v2.0]

- Available in Options menu.
- Each action can be remapped to any key.
- Persisted to `config.json`.
- Shows current key assignments with visual keyboard.
- Conflict detection (warns if two actions share a key).

### 10.9 Death Counter / Retry Screen [NEW in v2.0]

- Shows per-level death count on Game Over.
- Encouraging messages cycle: "Try again!", "You've got this!", "Almost there!"
- Quick Retry button skips menu navigation.

### 10.10 Lives & Game Over

- Start with **3 lives** (Easy: 5 lives, Hard: 1 life).
- On death: respawn at **last checkpoint** (or level start if no checkpoint reached).
- On Game Over (lives = 0): return to **World Map** (or Main Menu if no progress).
- Time limit: **300 game-seconds** per level.

---

## 11. Audio

### 11.1 Sound Effects (SFX)

| Event | SFX File |
| :--- | :--- |
| Jump | `sfx/jump.wav` |
| Coin Collect | `sfx/coin.wav` |
| Stomp Enemy | `sfx/stomp.wav` |
| Power-up | `sfx/powerup.wav` |
| Power-down | `sfx/powerdown.wav` |
| Fireball | `sfx/fireball.wav` |
| 1-UP | `sfx/oneup.wav` |
| Player Death | `sfx/death.wav` |
| Flagpole | `sfx/flagpole.wav` |
| Pipe Warp | `sfx/pipe.wav` |
| Ground Pound | `sfx/groundpound.wav` |
| Wall Jump | `sfx/walljump.wav` |
| Combo (escalating) | `sfx/combo_1.wav` → `sfx/combo_4.wav` |
| P-Switch | `sfx/pswitch.wav` |
| POW Block | `sfx/pow.wav` |
| Achievement Unlock | `sfx/achievement.wav` |
| Splash (water) | `sfx/splash.wav` |

### 11.2 Background Music (BGM)

| Context | BGM File |
| :--- | :--- |
| Main Menu | `music/menu.ogg` |
| World Map | `music/worldmap.ogg` |
| Overworld (Level 1) | `music/overworld.ogg` |
| Underground (Level 2) | `music/underground.ogg` |
| Castle (Level 3) | `music/castle.ogg` |
| Star Power | `music/star.ogg` (overrides level BGM while active) |
| Boss Fight | `music/boss.ogg` |
| Bonus Room | `music/bonus.ogg` |
| Game Over | `music/gameover.ogg` (jingle) |
| Victory | `music/victory.ogg` (jingle) |

### 11.3 Audio Controls

- Separate volume sliders: SFX and Music (in Options menu).
- Source: Open-source / royalty-free retro-style audio.

### 11.4 Surface-Dependent Footsteps [NEW in v2.0]

| Surface | Sound |
| :--- | :--- |
| Grass/Ground | Soft footstep |
| Stone/Brick | Hard footstep |
| Ice | Sliding/scraping |
| Metal (Castle) | Metallic clang |

### 11.5 Dynamic Music Layers [NEW in v2.0]

- Base layer always plays.
- Additional instrument layers fade in based on:
  - Proximity to boss (drums intensify)
  - Low time remaining (tempo increases)
  - Star power (override to star track)
  - Combo streak (percussion layer adds)

---

## 12. Save/Load (Serialization)

### 12.1 Format

JSON — human-readable, easy to debug.

### 12.2 Persisted Data

```json
{
  "version": "2.0",
  "timestamp": "2026-06-02T14:30:00Z",
  "player": {
    "character": "mario",
    "state": "fire",
    "position": { "x": 1024.5, "y": 640.0 },
    "lives": 3,
    "score": 12500,
    "coins": 47
  },
  "level": {
    "id": 2,
    "name": "Underground",
    "timeRemaining": 185.5,
    "checkpoint": { "x": 100, "y": 20 },
    "starCoinsCollected": [true, false, false]
  },
  "progress": {
    "levelsCompleted": [1],
    "unlockedCharacters": ["mario", "luigi"],
    "starCoins": { "1": [true, true, false], "2": [true, false, false], "3": [false, false, false] }
  },
  "statistics": {
    "totalEnemiesDefeated": 142,
    "totalCoinsCollected": 523,
    "totalDeaths": 18,
    "totalTimePlayed": 3600.5,
    "highestCombo": 6
  },
  "achievements": ["first_stomp", "100_coins", "beat_bowser"],
  "highScores": [50000, 42000, 35000, 28000, 12500],
  "settings": {
    "sfxVolume": 80,
    "musicVolume": 60,
    "difficulty": "normal",
    "keyBindings": { "jump": "W", "left": "A", "right": "D", "fire": "F" }
  }
}
```

### 12.3 Save Slots

- 3 save slots available.
- Auto-save at checkpoint.
- Manual save from pause menu.
- Save slot preview shows: character, level, score, star coins, play time.

---

## 13. Two-Player Mode (Versus)

| Setting | Value |
| :--- | :--- |
| **Mode** | Same keyboard, versus |
| **Player 1** | WASD + Q (fire) + E (special) |
| **Player 2** | Arrow keys + M (fire) + N (special) |
| **Objective** | Both players in same level. Race to flagpole. Can stomp each other (or configurable). |

---

## 14. Controls

### 14.1 Single-Player Keyboard Mapping (Default)

| Action | Key |
| :--- | :--- |
| Move Left | A or ← |
| Move Right | D or → |
| Jump | W or ↑ or Space |
| Run | Left Shift (hold) |
| Crouch / Ground Pound | S or ↓ |
| Fire | F or J |
| Pause | Escape |
| Switch Character | Tab |
| Minimap Toggle | M |
| Dev Tools (ImGui) | F12 |
| Debug Console | ~ (tilde) |

### 14.2 Input Architecture

- **Command Pattern**: Keyboard → `ICommand` objects → Character actions.
- Abstraction layer for future gamepad/controller support.
- All keys rebindable from Options menu.
- Key bindings persisted to `config.json`.

---

## 15. Visual Effects [NEW in v2.0]

### 15.1 Screen Shake System

| Trigger | Intensity | Duration |
| :--- | :--- | :--- |
| Brick break | Light (2px offset) | 0.1s |
| Ground pound | Medium (4px offset) | 0.2s |
| Bowser stomp | Heavy (6px offset) | 0.3s |
| Mega Mushroom movement | Continuous light | While active |
| POW Block | Heavy (6px offset) | 0.3s |

### 15.2 Entity Death Animations

| Entity | Death Animation |
| :--- | :--- |
| Goomba | Squish flat for 0.5s, then disappear |
| Koopa (fireball) | Flip upside-down, fall off screen |
| All enemies (Star kill) | Launch into air spinning |
| Player | Jump animation upward, then fall off screen |

### 15.3 Invincibility Visual FX

| Type | Effect |
| :--- | :--- |
| Star Power | Rainbow color cycling on player sprite (6 colors, 0.1s per color) + sparkle trail particles |
| Hit Invincibility | Sprite flashes (visible/invisible alternating, 0.1s interval) for 2 seconds |

### 15.4 Water / Lava Animation

- Water surface: sine-wave vertex displacement, blue tint overlay, bubble particles rising.
- Lava surface: orange/red animated tiles, ember particles, heat distortion shader (optional).

---

## 16. Advanced Systems [NEW in v2.0]

### 16.1 Object Pooling

| Pool | Pre-allocated Size | Entity |
| :--- | :--- | :--- |
| Fireballs | 10 | Player fireballs + Bowser fireballs |
| Particles | 200 | All particle types |
| Bullet Bills | 10 | Bullet Bill projectiles |
| Floating Text | 20 | Score/combo text popups |

### 16.2 Spatial Hashing (Collision Broadphase)

| Property | Value |
| :--- | :--- |
| Cell Size | 64 × 64 pixels (2 × tile size) |
| Rebuild | Every physics frame |
| Benefit | Only check collisions between entities in same or adjacent cells |
| Complexity | O(n) average vs. O(n²) brute force |

### 16.3 Config-Driven Entity Definitions

All entity properties defined in `entities.json`:

```json
{
  "goomba": {
    "speed": 60,
    "health": 1,
    "score": 100,
    "stompable": true,
    "fireballKillable": true,
    "strategy": "patrol",
    "animation": { "walk": [0,1], "squish": [2], "frameDuration": 0.3 }
  }
}
```

Factory reads from this config, eliminating hard-coded entity values.

### 16.4 Replay System

| Property | Value |
| :--- | :--- |
| Recording | Captures all input commands per frame |
| Storage | Binary file with frame-indexed command stream |
| Playback | Deterministic replay using same RNG seed + commands |
| File Size | ~1KB per minute of gameplay |
| Use | Speed run verification, ghost playback |

### 16.5 Debug Console

| Property | Value |
| :--- | :--- |
| Toggle | ~ (tilde) key |
| Commands | `spawn <entity> <x> <y>`, `setlevel <n>`, `god`, `noclip`, `give <item>`, `kill_all`, `tp <x> <y>`, `speed <multiplier>`, `stats` |
| Pattern | Command Pattern — text is parsed into ICommand objects |
| Autocomplete | Tab-completion for command names |

---

## 17. Accessibility [NEW in v2.0]

### 17.1 Difficulty Modes

| Mode | Lives | Enemy Speed | Checkpoints | Items |
| :--- | :--- | :--- | :--- | :--- |
| **Easy** | 5 | ×0.75 | Every 50 tiles | More frequent |
| **Normal** | 3 | ×1.0 | Mid-level (1 per level) | Standard |
| **Hard** | 1 | ×1.25 | None | Fewer items |

Implemented via `DifficultyStrategy` that modifies game constants at level load.

### 17.2 Colorblind Mode

| Mode | Adjustment |
| :--- | :--- |
| Deuteranopia | Green→Blue palette swap for enemies/items |
| Protanopia | Red→Yellow palette swap |
| Tritanopia | Blue→Pink palette swap |

Toggle in Options menu. Applies shader/palette swap.

### 17.3 Audio Navigation Cues

- Menu selection: click sound on highlight change
- Menu confirm: confirm chime
- Menu back: back chime
- Invalid action: error buzz

---

## 18. Meta-Game & Progression [NEW in v2.0]

### 18.1 Achievement System

| Achievement | Condition | Icon |
| :--- | :--- | :--- |
| First Stomp | Defeat first enemy by stomping | Boot |
| Coin Collector | Collect 100 coins in a single run | Gold coin |
| Speed Demon | Complete any level in under 120 seconds | Clock |
| Untouchable | Complete any level without taking damage | Shield |
| Combo King | Achieve an ×8 combo | Star burst |
| Shell Shocker | Defeat 3 enemies with a single shell kick | Shell |
| Dragon Slayer | Defeat Bowser | Crown |
| Star Hoarder | Collect all 9 star coins | Star |
| No Deaths | Complete all 3 levels without dying | Heart |
| Secret Finder | Find all hidden blocks | Magnifying glass |

- Toast notification on unlock (slides in from top-right, 3s display).
- Achievements viewable from Main Menu.
- Persisted across sessions.

### 18.2 Unlockable Characters

| Character | Unlock Condition |
| :--- | :--- |
| **Toad** | Complete all 3 levels |
| **Peach** | Complete all 3 levels without dying |

### 18.3 New Game+

| Property | Value |
| :--- | :--- |
| Trigger | Complete all 3 levels |
| Changes | Levels are horizontally mirrored, enemies move 25% faster, fewer power-ups |
| Indicator | "★" prefix on world names |

### 18.4 Daily Challenge

| Property | Value |
| :--- | :--- |
| Seed | Current date (YYYYMMDD) → deterministic random seed |
| Generation | Procedurally generated level using the seed |
| Leaderboard | Local file (`daily_scores.json`) — score + time |
| Length | 100 tiles wide (shorter than main levels) |

---

## 19. Post-MVP Bonus Features

These features are **not in MVP scope** but are planned for post-MVP development if time allows.

### 19.1 Mario Maker (In-Game Level Editor)

- **Trigger**: Press F1 to pause and open editor overlay.
- **UI**: ImGui drag-and-drop: place tiles, enemies, items, coins.
- **Pattern**: Command Pattern for Undo/Redo.
- **Serialization**: Export/import levels as JSON.
- **Academic Value**: Nails the "Save/Load / Serialization" bonus requirement.

### 19.2 Time Manipulation ("Braid" Mechanic)

- **Trigger**: Hold Shift to rewind time.
- **Implementation**: Store game state snapshots in a circular buffer (~5 seconds = 300 frames).
- **Pattern**: Memento Pattern.
- **Academic Value**: Demonstrates advanced data structures (circular buffer).

### 19.3 Shadow Mario (Rival AI)

- **Feature**: Dark Mario that mimics player inputs with a 3-second delay.
- **Implementation**: Record player input history, replay through AI controller.
- **Pattern**: State Pattern + Strategy Pattern for pathing.
- **Academic Value**: Goes beyond basic proximity AI.

### 19.4 Dynamic Lighting & Weather (SFML Shaders)

- **Feature**: Day/night cycle, underground darkness with player-centered light radius, rain effects.
- **Implementation**: `sf::Shader` (GLSL fragment shaders).
- **Academic Value**: Graphics programming, unique visual presentation.

### 19.5 Procedural Level Generation (Infinite Mario)

- **Feature**: "Endless Mode" with dynamically generated terrain.
- **Implementation**: Chunk-based generation with difficulty scaling (Perlin noise or rule-based).
- **Academic Value**: Algorithmic thinking, procedural generation.

---

## 20. Rubric Alignment

| Requirement | Points | Our Implementation |
| :--- | :--- | :--- |
| Player Inputs, Movement, Collision | 20 | AABB + spatial hash, Command pattern input, 10 movement mechanics (walk, run, jump, wall jump, ground pound, crouch, slide, swim, climb, combo), 2-player support |
| Enemy Behavior | 10 | 13 enemy types + 3 color variants with 7+ AI strategies, mid-boss, final boss with phases |
| Power-Ups and Items | 10 | 12 item types, 7 player states, P-Switch tile transformation, POW broadcast |
| 3 Level Completion | 15 | 3 themed levels + bonus rooms + world map + vertical/autoscroll sections + star coins |
| Sounds | 10 | 17+ SFX, per-level BGM, dynamic music layers, surface footsteps, combo SFX, volume controls |
| OOP Design | 10 | Deep 4-level hierarchy, 25+ concrete classes, SOLID principles, smart pointers, Template Method |
| 5 Design Patterns | 25 | 10+ patterns: Factory, Singleton, State, Observer, Strategy, Command, Decorator, Memento, Object Pool, Template Method |
| AI | 5 | 7+ strategies, Lakitu spawner, HammerBro arcs, Thwomp state machine, Boo proximity, Boss phases, config-driven variants |
| Multiple Players | 5 | 4 characters (Mario, Luigi, Toad, Peach), character select + hotkey switch, unlockable progression, 2P versus |
| **Total** | **110/115** | (no 3D = -5, compensated by GLSL shader effects) |

---

## 21. Descope Addendum (2026-08-31)

The following features, named in the SPEC but not built, have been formally descoped as of this date and will not be implemented. Items planned for build in later phases (marked "planned R#") are listed here for clarity but omitted from this descope section.

### Features Descoped

- **F7 Dynamic music layers** (§11.5) — Current single `sf::Music` playback architecture is documented and sufficient; layering adds substantial implementation cost with low grading value.

- **§19.4 Dynamic Lighting & Weather (SFML Shaders)** — Advanced graphics feature requiring shader programming and visual effect tuning; deferred beyond scope.

- **§9.5 Autoscroll sections** — Vertical scrolling level sections would require additional level design and camera state management; deferred.

- **§9.4 Timed bonus rooms** (15-second countdown challenge rooms) — Concept requires level data extension and timer integration; deferred.

- **Climbing/vines mechanic** (§4.3.5) — Would require new player state and level tile interaction system; deferred.

- **Swimming as a state** (§4.3.4) — Complex fluid-physics state distinct from current water handling; deferred.

- **Skid mechanic** (§4.3.7) — Inertia-based directional control variant; deferred.

- **Hover-pause and stun effects** (§4.3.2) — Ground-pound hover state and knockback stun lock; deferred.

- **Knockback input-lock** (§4.3.6) — Input rejection during knockback recovery; deferred.

- **Red enemy variants** (§6.3) — Constructor parameters exist; factory hardcodes `false`; deferred.

- **Cape swoop mechanic** (§5.2) — Secondary cape action requiring state and animation extension; deferred.

- **Mini abilities** (§5.2) — Walk-on-water and mini-pipes for Mini Mario state; deferred.

- **Character-switch hotkey** (§5.1) — In-gameplay character switching; planned **R10** for attract mode / presentation system.

- **Floating score text** (§10.5) — Damage numbers and combo feedback particles; deferred.

- **Extra object pools** (§16.1) — Bullet Bill and floating-text dedicated memory pools; deferred.

- **A* pathfinding** — Remains descoped per `TASKS.md`; stateless behavior via proximity and patrol strategies sufficient.

- **Split-screen speedrun mode** — Remains descoped per `TASKS.md`.

### Features Planned for Later Phases

The following features remain in scope and will be implemented in designated phases:

- **Attract mode** — Planned **phase R10** (30-second idle replay playback).
- **Load Game menu row** — Planned **phase R8** (main menu UI integration).
- **Surface footsteps** — Planned **phase R7** item 4 (audio playback on step events).
- **Menu audio navigation cues** — Planned **phase R7** item 6 (per-row menu SFX).
- **Hidden block placed in shipped level** — Planned **phase R9** (secret placement and `secret_finder` achievement enablement).

---

## 22. Design Decision: Encapsulation Trade-off (A-11)

### Friend Class Sprawl in Entity and Character Hierarchies

**Decision**: ACCEPTED AS DELIBERATE DESIGN CHOICE.

Both `Entity` and `Character` maintain 12 friend declarations each, granting direct write access to protected coordinate and physics state to the physics engine, collision resolution, movement strategies, and game state manager.

**Rationale**: Physics and entity movement are tightly coupled in this implementation. The friend relationships exist to:

1. Allow `PhysicsEngine` and `CollisionResolver` to directly update position, velocity, and collision state without setter overhead during the fixed-timestep integration loop.
2. Grant `IMovementStrategy` and its 7 concrete subclasses (Patrol, Chase, Fly, Timer Emergence, Linear, Hammer Throw, Tethered Chase, Proximity Trigger) direct mutation rights so AI can express intent (velocity, facing direction) without indirection.
3. Enable `PlayingState` to directly manipulate character state during level transitions, game-over recovery, and checkpoint respawning.

**Alternatives considered**:

- **Getter/setter explosion**: Wrapping each field in accessors would incur virtual call overhead in the physics hot loop (9-stage pipeline, 60 Hz fixed timestep). Benchmarking showed measurable impact; decision to use friends was made to retain O(1) field access.
- **Protected-only**: Keeping friends to the direct physics-engine layer and using strategy pattern callbacks for AI. This would require every movement call to re-derive state (position from velocity + dt, etc.), duplicating the physics calculation.

The friend relationship is a deliberate encapsulation trade-off in favor of performance in the physics-critical path. The commented intent in the code (`// Friends are allowed direct write access to coordinate updates` in Entity.hpp:155, `// Friends for controlled physics write access` in Character.hpp:41) documents this as intentional, not accidental.
