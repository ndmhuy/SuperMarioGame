# Sprite Naming Conventions Documentation

This document defines the canonical sprite naming standards and clean-up rules for the **Player**, **Item**, and **World** spritesheets used in the Super Mario Game.

---

## 1. General Naming Guidelines

### Static Sprites
* All static sprites are named in lower snake_case (e.g. `solid_block_grey`, `super_mushroom`, `vine_body`).
* They do not have any numerical suffixes.

### Animated Sprites
* All animation sequences end with a zero-based suffix (e.g., `_0`, `_1`, `_2`, `_3`).
* **Standalone Prefix Rule**: The base prefix of an animation sequence (e.g., `coin`, `star`) **does not** exist as a standalone static frame key.
* For example, the animated star sequence consists of `star_0`, `star_1`, `star_2`, and `star_3`. There is no frame named just `star`.

### Duplication Resolution (0-Suffix vs Normal)
* If a frame has both a normal version (e.g. `fence_medium`) and a zero-indexed version (e.g. `fence_medium_0`), **and there are no higher indices (no `_1`, `_2`, etc.)**:
  * These frames are duplicates of each other.
  * **Rule**: Always use the **normal version** (e.g. `fence_medium`) and discard the `_0` version.

---

## 2. Spritesheet Specifications

### A. Player Spritesheet
Contains 92 frames covering Mario, Luigi, Toad, and Peach in Small and Tiny (Mini) forms.

* **Prefix Convention**: `[character]_[form]_[action]`
* **Character Names**: `mario`, `luigi`, `toad`, `peach`
* **Forms**: `small` (16x32 SMB2 style), `tiny` (16x16 Mini style)
* **Animated Actions**:
  * `_walk`: `_walk_0`, `_walk_1`
  * `_run`: `_run_0`, `_run_1`
  * `_climb`: `_climb_0`, `_climb_1`
* **Static Actions**:
  * `_idle`: Base standing pose.
  * `_wave`: Cheering pose.
  * `_hurt`: Damage reaction pose.
  * `_skid`: Slide/turn stop pose.
  * `_crouch`: Squatting pose.
  * `_crouch_hold`: Squatting while holding an item.
* **Global Static Action**:
  * `[character]_death`: The defeat frame (applies to all forms).

---

### B. Item Spritesheet
Contains 32 frames representing items, platforms, and NPCs.

* **Animated Items**:
  * `coin_0..3`
  * `star_0..3`
  * `fire_flower_green_0..3`
  * `fire_flower_blue_0..3`
  * `question_block_0..2`
* **Static Items & Elements**:
  * Powerups: `super_mushroom`, `one_up_mushroom`
  * Trampolines: `trampoline`, `trampoline_extended`, `trampoline_squished`
  * Vines: `vine_top`, `vine_body`
  * Platforms: `platform_short`, `platform_medium`, `platform_long`, `moving_platform`
  * NPCs: `toad_npc`, `princess`

---

### C. World Spritesheet
Contains 177 frames representing scenery, background textures, blocks, and flags.

* **Duplication Resolutions Applied**:
  * `fence_medium` and `fence_medium_0` -> **fence_medium**
  * `light_blue_bg` and `light_blue_bg_0` -> **light_blue_bg**
  * `tree_white_short` and `tree_white_short_0` -> **tree_white_short**
* **Animated Tiles**:
  * `axe_0..2`: Castle bridge axe.
  * `big_coin_0..2`: Large collectible coin.
  * `coin_0..1`: Level coin.
  * `fire_flower_0..1`: Dynamic flower ornament.
  * `full_flag_pole_0..4`: Goal pole flag.
  * `question_block_0..2`: World question block.
* **Static Tiles**:
  * Blocks: `solid_block_brown`, `solid_block_grey`, `solid_block_blue`
  * Particles: `brick_blue_particle`, `brick_brown_particle`, `brick_grey_particle`
  * Pipes: `pipe_red_up`, `pipe_green_up`
  * Scenery: `bush_light_green_long`, `tree_green_tall`, `fence_long`, `fence_medium`
  * Backgrounds: `black_bg`, `blue_bg`, `light_blue_bg`
