# Sprite Naming Conventions Documentation

This document defines the canonical sprite naming standards and clean-up rules for the **Player**, **Item**, **World**, and **Enemy/Projectile** spritesheets used in the Super Mario Game.

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

### A. Player Spritesheet ([player.json](file:///f:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/assets/spriteSheet/player/player.json))
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

### B. Item Spritesheet ([item.json](file:///f:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/assets/spriteSheet/item/item.json))
Contains 40 frames representing items, platforms, and NPCs.

* **Animated Items**:
  * `coin_0..3`
  * `star_0..3`
  * `fire_flower_green_0..3`
  * `fire_flower_blue_0..3`
  * `question_block_0..2`
  * `pow_block_0..7`: POW block vibration and activation sequence.
* **Static Items & Elements**:
  * Powerups: `super_mushroom`, `one_up_mushroom`
  * Trampolines: `trampoline`, `trampoline_extended`, `trampoline_squished`
  * Vines: `vine_top`, `vine_body`
  * Platforms: `platform_short`, `platform_medium`, `platform_long`, `moving_platform`
  * NPCs: `toad_npc`, `princess`

---

### C. World Spritesheet ([world_scenery_item.json](file:///f:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/assets/spriteSheet/world_scenery_item/world_scenery_item.json))
Contains 174 frames representing scenery, background textures, blocks, and flags.

* **Duplication Resolutions Applied**:
  * `fence_medium` and `fence_medium_0` -> **fence_medium**
  * `light_blue_bg` and `light_blue_bg_0` -> **light_blue_bg**
  * `tree_white_short` and `tree_white_short_0` -> **tree_white_short**
* **Animated Tiles & Special Mappings**:
  * `axe_0..2`: Castle bridge axe.
  * `big_coin_0..2` (**Star Coin**): Represents the **Star Coin** collectible item.
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

---

### D. Enemy & Projectile Spritesheet ([enemy_projectile.json](file:///f:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/assets/spriteSheet/enemy_projectile/enemy_projectile.json))
Contains 202+ frames covering SMB1 style enemies, custom projectiles, and bosses.

* **Prefix Convention**: `[enemy_name]_[color/type]_[action/direction]`
* **Goombas (Brown, Blue, Grey)**:
  * Animations: `goomba_[color]_move_0..1`
  * Static: `goomba_[color]_squished`
* **Koopas & Paratroopas (Green, Red, Blue)**:
  * Walk Animations: `koopa_[color]_move_[direction]_0..1`
  * Flight Animations: `koopa_[color]_fly_[direction]_0..1`
  * Shell Statics: `koopa_[color]_shell`, `koopa_[color]_shell_leg_popout`
* **Boos (Ghost Enemy)**:
  * Animations: `boo_move_0..1`, `boo_attack_0..1`, `boo_seen_0..1`, `boo_funny_0..1`
* **Thwomps**:
  * Statics: `thwomper_active` (falling/angry), `thwomper_dormant` (idle)
* **Chain Chomps**:
  * Head Animations: `chained_chomp_head_[direction]_0..1`
  * Link Statics: `chained_chomp_chain`, `chained_chomp_chain_squished`
* **Buzzy Beetles (Black, Blue, Grey)**:
  * Walk Animations: `beetle_[color]_move_[direction]_0..1`
  * Shell/Hide Statics: `beetle_[color]_shell`, `beetle_[color]_hide`
* **Piranha Plants (Green, Blue)**:
  * Animations: `pirhana_[color]_0..1`
* **Hammer Bros (Green, Blue)**:
  * Walk Animations: `hammer_bros_[color]_move_[direction]_0..1`
  * Throw Statics: `hammer_bros_[color]_throw_[direction]`
* **Lakitus**:
  * Statics: `lakitu_left`, `lakitu_right`, `lakitu_hide`
* **Spinies**:
  * Walk Animations: `spiny_move_[direction]_0..1`
  * Ball Animations: `spiny_ball_0..1`
* **Cheep Cheeps (Red, Green, Grey)**:
  * Swim Animations: `cheep_cheep_[color]_move_[direction]_0..1`
* **Bloopers**:
  * Swim Animations: `squid_move_0..1`
* **Bowsers**:
  * Walk Animations: `bowser_move_[direction]_0..3`
  * Fire Breathing Animations: `bowser_fire_[direction]_0..1`
* **Projectiles & Special Elements**:
  * Hammer projectiles: `hammer_[color]_0..3`
  * Fireball projectiles: `flower_fireball_0..3` (Impact: `flower_fireball_hit_0..2`)
  * Lava projectiles (Podoboos): `lava_fireball_up`, `lava_fireball_down`
  * Bullet Bills: `bullet_bill_bullet_left`, `bullet_bill_bullet_right`
  * Blaster Blocks: `bullet_bill_body`, `bullet_bill_combined`, `bullet_bill_head`, `bullet_bill_neck`
