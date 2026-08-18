# Required Sound Effects (SFX) List

This document lists all the required sound effects (SFX) and background music (BGM) for the Super Mario Game, compiling both the frozen game specification (SPEC v2.0) and the active C++ source code references.

---

## 1. Action & Gameplay Sound Effects (SFX)

The table below lists all required gameplay sound effects, their canonical C++ Sound ID, target filename, description, code integration status, and physical asset status.

| Event / Action | C++ Sound ID | Target Filename | Description | Code Integration | Asset Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Jump (Small)** | `jump` | `sfx/jump.wav` | Triggered when small player jumps. | To-Be-Wired | **Present** |
| **Jump (Super)** | `jump_super` | `sfx/jump_super.wav` | Triggered when super player jumps. | To-Be-Wired | **Present** |
| **Coin Collect** | `coin` | `sfx/coin.wav` | Triggered on coin collection. | Referenced | **Present** |
| **Stomp Enemy** | `stomp` | `sfx/stomp.wav` | Triggered when stomping an enemy. | Referenced | **Present** |
| **Power-up Collect**| `powerup` | `sfx/power_up.wav` | Triggered when picking up a power-up. | To-Be-Wired | **Present** |
| **Power-down / Hurt**| `powerdown` | `sfx/damage.wav` | Triggered when getting hit/shrinking. | To-Be-Wired | **Present** |
| **Fireball Throw** | `fireball` | `sfx/fireball.wav` | Triggered when throwing a fireball. | To-Be-Wired | **Present** |
| **1-UP Life** | `oneup` | `sfx/one_up.wav` | Triggered on extra life award. | To-Be-Wired | **Present** |
| **Player Death** | `death` | `sfx/lost_life.wav` | Triggered when player dies. | To-Be-Wired | **Present** |
| **Flagpole Slide** | `flagpole` | `sfx/flagpole.wav` | Triggered when sliding down flag pole. | Referenced | **Present** |
| **Pipe Warp** | `pipe` | `sfx/pipe.wav` | Triggered when warping through a pipe. | Referenced | **Present** |
| **Block Hit / Bump** | `bump` | `sfx/bump.wav` | Triggered when hitting a block. | Referenced | **Present** |
| **Block Shatter** | `break_block` | `sfx/break_brick_block.wav` | Triggered when breaking a brick block. | Referenced | **Present** |
| **Power-up Spawn** | `powerup_appears`| `sfx/mushroom_fireflower_appears.wav` | Triggered when item pops out of block. | Referenced | **Present** |
| **Kick Shell/Object**| `kick` | `sfx/kick.wav` | Triggered when kicking shells/items. | Referenced | **Present** |
| **Timer Warning** | `timer_warning` | `sfx/time_warning.wav`| Played when level time drops < 50s. | To-Be-Wired | **Present** |
| **Bowser Fall** | `bowser_fall` | `sfx/bowserfall.wav` | Triggered when Bowser is defeated. | To-Be-Wired | **Present** |
| **Thwomp Slam** | `thwomp` | `sfx/thwomp.wav` | Triggered when Thwomp slams down. | To-Be-Wired | **Present** |
| **Vine Grow** | `vine_grow` | `sfx/vine_grow.wav` | Triggered when a vine grows. | To-Be-Wired | **Present** |
| **Pause** | `pause` | `sfx/pause.wav` | Played when pausing the game. | To-Be-Wired | **Present** |
| **Enter Level** | `enter_level` | `sfx/enter_level.wav` | Played when entering a level. | To-Be-Wired | **Present** |
| **Stage Clear** | `stage_clear` | `sfx/stage_clear.wav` | Played upon level completion. | To-Be-Wired | **Present** |
| **World Clear** | `world_clear` | `sfx/world_clear.wav` | Played upon world completion. | To-Be-Wired | **Present** |
| **Game Over** | `game_over` | `sfx/game_over.wav` | Played when player loses all lives. | To-Be-Wired | **Present** |
| **Ground Pound** | `groundpound` | `sfx/groundpound.wav`| Triggered when player slams down. | To-Be-Wired | **Missing** |
| **Wall Jump** | `walljump` | `sfx/walljump.wav` | Triggered when kicking off a wall. | To-Be-Wired | **Missing** |
| **P-Switch Event** | `pswitch` | `sfx/pswitch.wav` | Triggered when pressing a P-Switch. | To-Be-Wired | **Missing** |
| **POW Block Strike** | `pow` | `sfx/pow.wav` | Triggered when striking a POW block. | To-Be-Wired | **Missing** |
| **Achievement** | `achievement` | `sfx/achievement.wav`| Triggered when unlocking achievement. | To-Be-Wired | **Missing** |
| **Water Splash** | `splash` | `sfx/splash.wav` | Triggered when falling in water. | To-Be-Wired | **Missing** |
| **Combo Streaks** | `combo_1` to `combo_4` | `sfx/combo_X.wav` | Escalating SFX based on stomp chain count. | To-Be-Wired | **Missing** |
| **Menu Hover** | `menu_click` | `sfx/click.wav` | Triggered when moving highlight. | To-Be-Wired | **Missing** |
| **Menu Select** | `menu_confirm` | `sfx/confirm.wav` | Triggered when selecting an option. | To-Be-Wired | **Missing** |
| **Blaster Cannon** | `cannon` | `sfx/cannon.wav` | Triggered when a Bill Blaster fires. | To-Be-Wired | **Missing** |
| **Fireball Hit** | `fireball_hit` | `sfx/fireball_hit.wav`| Triggered when fireball hits wall. | To-Be-Wired | **Missing** |
| **Trampoline Spring**| `spring` | `sfx/spring.wav` | Triggered when bouncing off trampolines. | To-Be-Wired | **Missing** |

---

## 2. Surface-Dependent Footsteps (SFX)

Footstep sounds play dynamically during player movements depending on the ground block material:

| Surface Type | Sound / Sound ID | Target Filename | Description | Asset Status |
| :--- | :--- | :--- | :--- | :--- |
| **Grass / Soil** | `footstep_grass` | `sfx/footstep_grass.wav` | Soft rustling stride. | **Missing** |
| **Stone / Brick** | `footstep_stone` | `sfx/footstep_stone.wav` | Hard solid thump. | **Missing** |
| **Ice Block** | `footstep_ice` | `sfx/footstep_ice.wav` | Sliding/scraping sound. | **Missing** |
| **Metal (Conveyor/Castle)** | `footstep_metal` | `sfx/footstep_metal.wav` | High metallic clang. | **Missing** |

---

## 3. Background Music (BGM)

The following BGM tracks are required to be loaded:

| Context / Level | BGM File | Description | Asset Status |
| :--- | :--- | :--- | :--- |
| **Main Menu** | `music/menu.ogg` | Menu interface screen loop. | **Missing** |
| **World Map** | `music/worldmap.ogg` | Overworld level selection screen. | **Missing** |
| **Overworld (Level 1)**| `music/overworld.ogg` | Level 1 theme music. | **Missing** |
| **Underground (Level 2)**| `music/underground.ogg`| Cave/Sewer style theme. | **Missing** |
| **Castle (Level 3)** | `music/castle.ogg` | Castle hazards theme. | **Missing** |
| **Star Power** | `music/star.ogg` | Invincible overlay music. | **Missing** |
| **Boss Fight** | `music/boss.ogg` | Bowser / Boom Boom boss battles. | **Missing** |
| **Bonus Room** | `music/bonus.ogg` | Secret/coin room music. | **Missing** |
| **Game Over** | `music/gameover.ogg` | Defeat game over jingle. | **Missing** |
| **Victory** | `music/victory.ogg` | Level completion victory fanfare. | **Missing** |
