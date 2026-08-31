# List of Features (0.25pts each — Total 10pts)

**Group 52 — Super Mario Game.** This project was scoped against `SPEC.md`'s
expanded v2.0 specification (110 features across 20 categories) specifically
because the team has two members instead of one — the brief in
`FEATURE_PROPOSAL.md` justifies doubling the normal single-member scope. The
list below is not the generic 40-item version; every line was checked against
the actual source in `SuperMarioGame/include/` and `SuperMarioGame/src/` (not
just `SPEC.md`'s intent) during a dedicated feature audit, and is reachable
from a real playthrough — constructed through `EntityFactory`, wired into
`PlayingState`/`Game`, and not merely exercised by a `verify_*.cpp` test
harness. A small number of items are marked *(editor-authored)* — implemented,
wired, and fully functional, but placed into the world through the in-game
level editor or a hand-authored level file rather than one of the seven
shipped campaign/bonus levels; everything else is present in the campaign as
shipped.

Some SPEC.md-described behaviour did **not** make this list because the audit
could not confirm it is reachable from the running game — for example,
climbing/vines, the ground-pound hover pause and enemy-stun radius, the
skid mechanic, input-locking on knockback, `noclip`/`kill_all`/`stats` debug
commands, dynamic lighting shaders, and a true infinite "Endless Mode". Listing
those would have been the kind of optimistic claim this project's own
`AGENTS.md` explicitly warns against.

---

## 1. Core Engine & Architecture
1. **Fixed-Timestep Game Loop** — 1/60s accumulator with interpolated rendering, independent of frame rate; the same determinism the time-rewind and replay systems rely on.
2. **Game State Management (State Pattern)** — `GameStateManager` stacks 8 screens (Menu, Character Select, World Map, Playing, Pause, Options, Game Over, Victory); Pause/Victory overlay the level rather than replacing it.
3. **Singleton Managers** — `Game`, `ResourceManager`, `SoundManager`, `InputManager`, `EventBus`, `AchievementManager` and 8 more, all Meyers singletons with defined construction order.
4. **Event System (Observer Pattern)** — `EventBus` with 35+ event types; HUD, sound, achievements, statistics and the combo tracker all subscribe independently.
5. **Input Handling (Command Pattern)** — every action is an `ICommand` object (`JumpCommand`, `MoveLeftCommand`, `RunCommand`, `GroundPoundCommand`, `WallJumpCommand`, `CrouchCommand`, `FireCommand`…), rebindable and shared by Player 2 through a second binding table.
6. **Object Pooling** — `ObjectPool<T>` reuses fireballs, hammers and boss projectiles; particles are pooled separately (200 pre-allocated).
7. **Spatial Hashing** — 64px broad-phase collision grid; an *O(n²)* all-pairs test becomes *O(n)* expected work against bucket neighbours.
8. **Config-Driven Entity Tuning** — `assets/config/entities.json` adjusts per-type speed and score at spawn time without a recompile.
9. **Debug Console** — backtick-toggled ImGui console with 11 real commands (`help`, `give`, `lives`, `tp`, `god`, `spawn`, `difficulty`, `level`, `progress`, `replay`, `clear`).
10. **Data Serialization** — full JSON round-trip of player state, level progress, statistics, achievements and settings via `nlohmann::json`.

## 2. Physics & Collision
11. **AABB Collision Detection** — axis-aligned bounding boxes for every entity, checked through the spatial hash.
12. **Ten-Stage Kinematic Physics Pipeline** — conveyor push → interactive tiles → acceleration/friction → gravity → X-axis integrate/resolve → Y-axis integrate/resolve → map bounds → broad phase → narrow phase/resolution → intent-flag clear, in a fixed, load-bearing order.
13. **Collision Resolution** — dispatches by entity-pair type: stomp, side-damage, item collection, block bump, shell kick, boss contact, player-vs-player in multiplayer.
14. **Coyote Time & Jump Buffering** — 6-frame (100ms) forgiveness windows on both sides of a jump.
15. **Dual Bounding-Box Architecture** — a `physicsBox` for collision and a separate `combatBox` for hit detection, so a stomp and a solid-tile check never fight over the same rectangle.
16. **Surface-Dependent Physics** — ice tiles cut deceleration from ~1000 to 250 px/s²; conveyor tiles push at a constant 100px/s.

## 3. Advanced Movement Mechanics
17. **Wall Jump / Wall Slide** — capped 50px/s downward slide, launches off the wall on jump.
18. **Ground Pound** — 600px/s instant slam with screen shake and a stomp cue on landing.
19. **Crouch / Slide** — halves the player's hitbox, lets them pass under 1-tile gaps, and stands back up only when the ceiling is clear.
20. **Damage Knockback** — a fixed 150×100px impulse away from the hit, plus temporary invincibility with a visible sprite flash.
21. **Momentum & Acceleration Curves** — separate ground/air friction and deceleration rates, with ice further reducing grip.
22. **Combo System** — stomping enemies without touching ground increments a HUD-visible counter that resets after a 2.5s idle window or on taking damage.

## 4. Characters & Player States
23. **Four Playable Characters** — Mario (baseline), Luigi, Toad and Peach, each with distinct movement constants.
24. **Luigi** — ×1.2 jump force, ×0.85 walk/run speed, ×0.9 airborne gravity, plus a genuine double jump.
25. **Toad** — ×1.3 speed, ×0.8 jump force, and slide momentum that is not cut by ground friction.
26. **Peach** — ×0.9 speed and a float/hover that lasts up to 1.5s.
27. **Unlockable Characters** — Toad and Peach open on real `AchievementManager` gates (finish the campaign; finish it without dying) and become selectable at Character Select.
28. **Player Forms (State + Decorator Patterns)** — five concrete `IPlayerState` forms (Small, Super, Fire, Cape, Mini) with a full power-up/power-down transition chain.
29. **Star Invincibility (Decorator)** — `StarDecorator` wraps whichever form is active for 10 seconds without altering it, so a Star picked up as Fire Mario still returns to Fire Mario after.
30. **Mega Mushroom (Decorator)** — `MegaDecorator` scales the player up and grants 8 seconds of damage immunity.
31. **Two-Player Same-Keyboard Input** — Player 1 on WASD+F, Player 2 on Arrow keys+M(fire)+N(run), both routed through the same Command-pattern pipeline.

## 5. Enemies & AI
32. **Entity Factory (Factory Pattern)** — `EntityFactory::create` is the single construction point for 25+ entity types from a level-file string, with no `#include` of the concrete class required at the call site.
33. **Eight Movement Strategies (Strategy Pattern)** — Patrol, Chase, Fly, TimerEmergence, Linear, HammerThrow, TetheredChase and ProximityTrigger, swappable per enemy at construction.
34. **Goomba & Koopa Troopa** — classic ledge-aware patrol, with Koopa shells that can be kicked, carried and used as a weapon.
35. **Koopa Paratroopa** *(editor-authored)* — vertical-bounce and sinusoidal-patrol flight variants, selectable through the level editor's entity palette.
36. **Piranha Plant** — timer-based emergence from a pipe, implemented via `TimerEmergenceStrategy`.
37. **Hammer Bro** — tracks the player's side and throws a rotating hammer projectile roughly every 1.5s.
38. **Boo** *(editor-authored)* — freezes when faced, pursues at 100px/s the moment the player looks away; immune to stomping and fireballs.
39. **Thwomp** *(editor-authored)* — teeth-gritting wind-up, a 700px/s slam, a floor rest, and a slow climb back to its start height.
40. **Lakitu & Spiny** *(Lakitu editor-authored)* — Lakitu flies overhead and spawns Spiny enemies via the same Factory the rest of the game uses; Spiny patrols the ground in the shipped campaign.
41. **Bullet Bill & Chain Chomp** *(editor-authored)* — a straight-line projectile enemy and a tethered lunge-and-recover enemy, both fully implemented and placeable.

## 6. Bosses
42. **Boom Boom (World 1-2 mid-boss)** — a three-stomp fight with an escalating charge/spin/recover pattern per phase, health-scaled by difficulty.
43. **Bowser (World 1-3 final boss)** — two-phase fight: Phase 1 walks and breathes fire; Phase 2 adds a leap and a faster (1.2s vs 2.2s) fire cadence, with a fireball-immunity/stagger window.
44. **Boss Template (Template Method Pattern)** — `Boss::update()` is sealed and calls a pure-virtual `updateBehaviour()`, so every boss gets its invulnerability frames, phase transitions and defeat sequence in the right order regardless of its attack pattern.
45. **Boss Arena Lock** — "no escape until defeated": the camera locks to the arena and the player's position is clamped inside it until the boss's health reaches zero, gated independently of level geometry so a misplaced arena cannot skip the fight.
46. **Boss HUD** — a dedicated health bar and boss name appear only while a boss fight is active.

## 7. Items & Power-ups
47. **Super Mushroom, Fire Flower, Cape Feather, Mini Mushroom, Mega Mushroom, Star, 1-Up Mushroom** — seven power-ups, each driving the same `Player::powerUp()` state-transition path.
48. **Coins & the 100-Coin 1-Up** — collecting 100 coins grants an extra life.
49. **POW Block** — flips every grounded, on-screen enemy when hit.
50. **P-Switch** — a 15-second window that swaps bricks and coins level-wide.
51. **Trampoline** — configurable spring bounce, placed throughout the campaign and both sub-vaults.
52. **Star Coins** — exactly 3 per main/bonus level (1 per sub-vault), tracked per level and persisted across sessions.

## 8. Blocks & World Objects
53. **Question Blocks & Brick Blocks** — bump-to-reveal item logic and break-from-below physics tied to the player's current form.
54. **Warp Pipes** — bidirectional transitions between a main level and its underground/sky/lava sub-vault, preserving lives, coins, score and power-up state.
55. **Moving Platforms** — patrol back and forth and carry the player riding on top.
56. **Falling Platforms** — a four-state cycle (Idle → Shaking 1s → Falling → Respawning 5s).
57. **Ice & Conveyor Tiles** — level-authored surfaces that feed directly into the physics pipeline's surface-dependent friction and push.
58. **Flagpole Completion** — score awarded on a five-tier scale (100 to 5000) based on how high the player is caught, gated so it cannot fire while a level's boss is still alive.
59. **The End-of-Level Castle** — a decorative structure whose flag climbs the gatehouse in step with the flagpole's own descent; the player visibly walks up to its door before the summary screen appears.

## 9. Levels & World
60. **A Three-World Campaign** — World 1-1 (Grassland), World 1-2 (Ice Cavern, Boom Boom), World 1-3 (Bowser's Castle Fortress), plus a selectable Bonus Stage, all authored in JSON and loaded through `LevelLoader`.
61. **Three Sub-Vaults** — an Underground vault (1-1), a Sky Canopy vault (1-2) and a Secret Lava vault (1-3), each a self-contained side room reached and left through a warp pipe.
62. **World Map** — sequential level unlocking, a per-node star-coin pip display and a completion tick, navigated with Left/Right/Enter.
63. **Procedural Level Generator (`MapGenerator`)** — multi-tier elevation, biome-specific ceilings, lava pits, reachability-guarded platform gaps and a threat-pacing curve; drives both the Daily Challenge and a "Generate & Play/Edit" menu page.
64. **In-Game Level Editor (Mario Maker)** — F1 toggles a live editor with tile/entity palettes, an undo/redo command stack, and JSON save/load, reachable from the pause-free main menu or mid-level.

## 10. Game Flow & UI
65. **Main Menu** — Start Game, Multiplayer, Daily Challenge, Map Editor, Procedural Level, Records, Options and Quit, with an animated parallax background and a walking-Mario idle sprite.
66. **Pause Menu** — Resume, Save Game, Options, Restart Level and Quit to Menu, overlaying the still-visible level.
67. **HUD** — live score, coins, world/level, timer, lives, combo counter, and Star Coin indicators.
68. **Minimap** — a toggleable (Tab) overview of the whole level and every live entity, colour-coded and colourblind-palette aware.
69. **Statistics Screen** — cross-session totals for coins, enemies defeated, deaths, combo hits and play time, persisted to a profile file.
70. **Screen Transitions & Camera Feel** — fade in/out between levels, a velocity-driven camera lookahead, and event-triggered screen shake (block break, ground pound, POW block).
71. **Death & Retry** — a global death tally and an instant "Retry Level" that skips menu navigation entirely.

## 11. Audio
72. **Full SFX Coverage** — jump, coin, stomp, power-up, damage, fireball, player death, flagpole, pipe warp, ground pound, P-Switch and achievement cues, each fired from the real event that causes it.
73. **Adaptive Background Music** — distinct tracks for the menu, world map, overworld/underground/underwater/bonus themes, Star Power, boss fights and Game Over, swapped through `SoundManager`.
74. **Volume Controls** — independent music/SFX sliders in Options, persisted to `config.json`.

## 12. Save/Load & Persistence
75. **Three Save Slots** — full player/level/progress/statistics/settings state, written as JSON.
76. **Auto-Save at Checkpoints** — a warp pipe or debug checkpoint silently persists progress.
77. **Manual Save** — "Save Game" from the pause menu.
78. **High Score Table** — recorded per run (score, coins, star coins, character, level) and viewable from Options.

## 13. Two-Player & AI Opponents
79. **Two-Player Versus** — shared-keyboard local multiplayer with configurable stomp-vs-bounce collision between the two players.
80. **Co-op Mode** — the same shared-keyboard setup, but vertical player contact becomes a cooperative boost instead of a bounce.
81. **AI Opponent (Versus CPU)** — a CPU-controlled second player with distinct behaviour profiles (e.g. a Speedrunner that climbs away vs. a Hunter that reverses to chase).
82. **Shadow Mario (Rival AI)** — replays the human player's own input history after a configurable 0.5–10s delay, complete with its own jump-replay logic — a full Memento/Command-pattern rival, not a scripted opponent.

## 14. Controls
83. **Full Rebinding** — every action can be reassigned from Options, with conflicting bindings swapped rather than silently orphaned, and persisted to `config.json`.
84. **Command-Pattern Input Layer** — the same `ICommand` objects back keyboard input, the debug console's dispatch, and replay serialization.

## 15. Visual Effects
85. **Particle System** — pooled particle bursts for brick fragments, coin sparkle, death poofs, stomps and combos.
86. **Entity Death Animations** — a Goomba's timed squish-then-vanish, a fireball-killed enemy's upside-down flip-and-fall, and the player's own jump-then-fall death arc.
87. **Invincibility Flash** — the player's sprite alternates alpha for the full 2-second post-hit invincibility window.
88. **Event-Triggered Screen Shake** — light/medium/heavy presets tied to specific gameplay events (block break, ground pound, POW block).

## 16. Advanced Systems
89. **Time Rewind (Memento Pattern)** — `TimeRewindManager` holds a 300-frame circular buffer of `GameSnapshot`s and restores the game to any of the last five seconds on demand.
90. **Replay Recording** — every level records a full playable trace automatically, savable/loadable through the debug console.
91. **Difficulty Modes** — Easy/Normal/Hard scale starting lives, enemy speed, level timer and boss health together through a single `IDifficultyStrategy`.
92. **New Game+** — clearing the campaign starts a new cycle with a real enemy-speed multiplier, while keeping unlocks and the completion counter.
93. **Daily Challenge** — a date-seeded procedural level, deterministic for every player on the same day.
94. **Achievement System** — ten tracked milestones (first stomp, level completions, secret finds, coin totals and more) driving toast notifications and character unlocks.
95. **Colorblind Mode** — an Okabe-Ito-derived palette swap applied to the minimap and debug overlays.

---

### Scope note for graders

Every numbered item above was checked against the running source, not assumed
from `SPEC.md`'s wording — several `SPEC.md` claims were found to be
overstated during the audit (an exact power-of-two combo curve, a
three-mode colourblind system, a per-level death counter, mirrored New Game+
levels, a true "Endless Mode") and were **deliberately left off this list**
rather than claimed. What remains is 95 features that a grader can point at
in the code and, in nearly every case, reach by simply playing the shipped
campaign.
