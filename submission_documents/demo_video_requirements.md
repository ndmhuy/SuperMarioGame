# Super Mario Game — Feature Demo Video Requirements & Checklist

This document details all the required gameplay features, mechanics, level demonstrations, and grading rubric criteria to showcase in the **Feature Demo Video** for the Super Mario Game project.

---

## 1. Core Player Locomotion & Physics
- [ ] **Basic Movement**: Walking, sprinting/running with inertia/momentum, and deceleration skid.
- [ ] **Jump Mechanics**: Variable-height jump (tap vs. hold), jump buffering, and coyote time.
- [ ] **Character Abilities**:
  - **Mario**: Balanced run and jump baseline.
  - **Luigi**: Higher floaty jump multiplier (1.2×) and double jump.
  - **Toad**: High-speed sprint.
  - **Peach**: Horizontal dress float / hover glide.
- [ ] **Advanced Actions**:
  - Crouching (under low ceilings and into warp pipes).
  - Wall slide and wall jump.
  - Ground pound slam.

---

## 2. Power-Up Progression & States
- [ ] **Transformation Lifecycle**:
  - Small Form $\rightarrow$ Super Form (Super Mushroom).
  - Super Form $\rightarrow$ Fire Form (Fire Flower) with fireball projectile shooting.
  - Super Form $\rightarrow$ Cape Form (Cape Feather) with glide and spin attack.
  - Mini Form (Mini Mushroom) with half-size collision box.
  - Step-down damage transition (Fire/Cape $\rightarrow$ Super $\rightarrow$ Small $\rightarrow$ Death).
- [ ] **Temporary Power-Ups**:
  - **Starman**: 60 FPS rainbow hue cycling, invincibility immunity, and contact-kill trajectory on enemies.
  - **Mega Mushroom**: Giant form scaling and obstacle destruction.

---

## 3. Items & World Interactive Objects
- [ ] **Collectibles**:
  - Standard Coins (HUD counter increments and 100-coin life reward).
  - Star Coins (3 per level with HUD tracking).
  - 1-Up Mushroom (+1 extra life).
- [ ] **Interactive World Objects**:
  - **POW Block**: Screen-wide ground impact knocking out all grounded enemies.
  - **P-Switch**: Timed tile transformation turning brick blocks into coins and vice versa.
  - **Trampoline**: Spring compression animation and launch velocity.
  - **Bridge Axe**: Chopping bridge tiles to drop Bowser into lava.

---

## 4. Blocks & World Environment
- [ ] **Question Blocks**: Hitting from below to spawn items/coins, converting to Used state.
- [ ] **Brick Blocks**: Bumping from below by Small Mario, shattering with particle bursts by Super Mario.
- [ ] **Hidden Blocks**: Invisible blocks that appear when bumped from below.
- [ ] **Platforms & Terrains**:
  - Moving Platforms (horizontal/vertical path traversal).
  - Falling Platforms (stand $\rightarrow$ shake $\rightarrow$ drop $\rightarrow$ respawn).
  - Ice Blocks / Slippery terrain friction modifier.
  - Conveyor Belts (directional momentum on standing entities).

---

## 5. Enemies & AI Behaviors
- [ ] **Goomba**: Ground patrol, squash/flatten animation on stomp, flip death on fireball hit.
- [ ] **Koopa Troopa**: Ground patrol, retreat into shell on stomp, kicking shell to defeat enemies.
- [ ] **Koopa Paratroopa**: Aerial hopping/patrol, losing wings to become standard Koopa upon stomp.
- [ ] **Piranha Plant**: Periodic pipe emergence timer, staying retracted when player is on or near pipe.
- [ ] **Boo**: Sneak chase when Mario's back is turned, freezing in place when faced.
- [ ] **Hammer Bro**: Jumping between brick platforms while lobbing hammers in parabolic arcs.
- [ ] **Thwomp**: Proximity detection directly beneath, instant downward slam, and slow return to ceiling.
- [ ] **Chain Chomp**: Active player tracking, straining toward Mario on its leash, aggressive lunges, and elastic chain rebound.
- [ ] **Lakitu & Spiny**: Aerial player tracking, spawning and dropping spiny projectile eggs.
- [ ] **Bullet Bill**: Firing from blasters with continuous linear trajectory.
- [ ] **Mid-Boss (Boom Boom)**: 3-hit phase escalation, shell spin dash, and defeat sequence.
- [ ] **Final Boss (Bowser)**: Multi-phase battle, fireball breathing, high jumps, staggered opening, arena door locking, and bridge axe defeat.

---

## 6. Level Design & Progression
- [ ] **Level 1-1 (Overworld)**:
  - Full terrain traversal, pipes, question blocks, and enemies.
  - Sublevel warp pipe transition and subterranean coin room.
  - Victory Flagpole slide, catch-height score bonus, fanfare, and gatehouse entry.
- [ ] **Level 1-2 (Underground & Ice)**:
  - Dark subterranean tileset, ice blocks, moving platforms, falling platforms, and sublevel vault.
- [ ] **Level 1-3 (Castle Fortress)**:
  - Castle traps, lava hazards, Thwomps, Bowser boss arena lock, bridge axe chop, and game completion.
- [ ] **Bonus Challenge Level**:
  - Extra level layout testing high platforming skill and secret items.
- [ ] **Screen Transitions & Screen Shake**:
  - Pipe wipe / iris / fade transitions across level transitions.
  - Directional screen shake on POW hit, Thwomp slam, and boss defeat.

---

## 7. Audio & Music Implementation
- [ ] **Background Music (BGM)**:
  - Main Menu theme, World Map theme, Overworld, Underworld, Castle, Boss Battle, Starman invincibility, and Level Clear fanfare.
- [ ] **Sound Effects (SFX)**:
  - Jumping, footsteps, pipe warps, coin pickups, power-ups, brick breaks, enemy stomps, fireball throws, damage, and death.

---

## 8. Advanced Systems & Developer Tools
- [ ] **Character Selection Screen**: Choosing between Mario, Luigi, Toad, and Peach before starting.
- [ ] **Multiplayer & AI Modes**:
  - 2-Player Co-op / VS mode.
  - CPU-controlled AI companion following player 1.
- [ ] **Time Rewind Feature**: Live rewind key (`R`) reversing player position, state, and entities with full-screen scanline vignette.
- [ ] **Save / Load Persistence**: Saving progress to JSON save slots and reloading saved game state.
- [ ] **ImGui Dev Tools Panel**:
  - Live physics parameter tuning (gravity, walk speed, run speed, jump force).
  - Entity spawner, god mode toggle, and AABB collision box overlay.
- [ ] **In-Game Level Editor**:
  - Placing tiles, blocks, and enemies directly on the grid.
  - Saving custom maps to JSON files and playtesting them live.
- [ ] **Procedural Level Generator**:
  - Generating randomized, seed-based solvable platformer stages with configurable themes and difficulty.
- [ ] **Minimap System**:
  - Real-time radar displaying player, enemy, and collectible positions.
