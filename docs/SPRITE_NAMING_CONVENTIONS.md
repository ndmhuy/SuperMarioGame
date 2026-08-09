# Sprite Naming Conventions Documentation

This document defines the canonical sprite naming standards and metadata rules for the **Player**, **Item**, **World Scenery & Items**, **Enemy & Projectile**, and **Particles** sprite sheets used in the Super Mario Game.

---

## 1. General Naming Guidelines

### Static Sprites
* All standard game object sprites use lower `snake_case` (e.g., `solid_block_grey`, `super_mushroom`, `p_switch_normal`).
* Particles and special engine effect frames use `PascalCase` (e.g., `BrickBreak`, `CoinSparkle`, `Stomp`).
* Static sprites do not carry numerical suffixes.

### Animated Sprites
* All animation sequences end with a zero-based numerical suffix (e.g., `_0`, `_1`, ..., `_N`).
* **Unbound Suffix Rule**: Animation sequences are not bound to a fixed max frame count. Frames sequence sequentially starting from `_0` up to `_N` as needed.
* **Standalone Prefix Rule**: Base animation prefixes (e.g., `star`, `fire_flower`, `coin`) **do not** exist as standalone static frame keys.
* For example, an animated star sequence consists of `star_0`, `star_1`, `star_2`, `star_3`, etc.

### Duplication Resolution (0-Suffix vs Normal)
* If a frame has both a normal version (e.g. `fence_medium`) and a zero-indexed version (e.g. `fence_medium_0`), **and no higher indices (`_1`, `_2`) exist**:
  * Always use the **normal version** (e.g., `fence_medium`) and remove the `_0` duplicate.

---

## 2. Spritesheet Specifications & Inventories

### A. Player Spritesheet
Contains frames covering Mario, Luigi, Toad, and Peach across `small` and `tiny` (mini) forms.

* **Prefix Convention**: `[character]_[form]_[action]`
* **Characters**: `mario`, `luigi`, `toad`, `peach`
* **Forms**: `small` (16x32 SMB2 style), `tiny` (16x16 Mini style)
* **Actions per Character/Form**:
  * `_idle`: Base standing pose
  * `_wave`: Cheering pose
  * `_walk_0`, `_walk_1`, ...: Walking animation cycle
  * `_hurt`: Damage reaction pose
  * `_jump`: Jump / float pose
  * `_skid`: Slide / turn stop pose
  * `_crouch`: Squatting pose
  * `_crouch_hold`: Squatting while holding item pose
  * `_climb_0`, `_climb_1`, ...: Climbing animation cycle
  * `[character]_death`: Defeat frame (applies globally across character forms)

---

### B. Item Spritesheet
Contains frames representing collectibles, power-ups, interactive objects, and NPCs.

* **Power-Ups & Collectibles**:
  * `super_mushroom`, `one_up_mushroom`, `mushroom_red`, `mushroom_green`, `mushroom_blue`
  * `fire_flower_0`, `fire_flower_1`, ...
  * `fire_flower_green_0`, `fire_flower_green_1`, ...
  * `fire_flower_blue_0`, `fire_flower_blue_1`, ...
  * `star_0`, `star_1`, ...
  * `coin_0`, `coin_1`, ...
  * `big_coin`, `big_coin_outline` (**Star Coin / Giant Coin**)
  * `cape_feather`, `super_leaf`, `mini_mushroom`, `mega_mushroom`
* **Interactive Blocks & Objects**:
  * `question_block_0`, `question_block_1`, ...
  * `p_switch_normal`, `p_switch_pressed`
  * `pow_block_0`, `pow_block_1`, ...: POW block vibration and detonation sequence
  * `trampoline`, `trampoline_extended`, `trampoline_squished`
  * `vine_head`, `vine_body`
* **Platforms & NPCs**:
  * `platform_short`, `platform_medium`, `platform_long`, `moving_platform`
  * `toad_npc`, `princess_npc`

---

### C. Enemy & Projectile Spritesheet
Contains frames covering SMB1 style enemies, custom bosses, and projectiles.

* **Prefix Convention**: `[enemy_name]_[color/type]_[action/direction]`
* **Goombas (Brown, Blue, Grey)**:
  * `goomba_[color]_move_0`, `goomba_[color]_move_1`, ..., `goomba_[color]_squished`
* **Koopas & Paratroopas (Green, Red, Blue)**:
  * `koopa_[color]_move_[left/right]_0`, `koopa_[color]_move_[left/right]_1`, ...
  * `koopa_[color]_fly_[left/right]_0`, `koopa_[color]_fly_[left/right]_1`, ...
  * `koopa_[color]_shell`, `koopa_[color]_shell_leg_popout`
* **Boos (Ghost Enemies)**:
  * `boo_move_0`, `boo_move_1`, ...
  * `boo_attack_0`, `boo_attack_1`, ...
  * `boo_seen_0`, `boo_seen_1`, ...
  * `boo_funny_0`, `boo_funny_1`, ...
* **Boomerang Bros & Hammer Bros (Green, Blue)**:
  * `boomerang_bro_move_0`, `boomerang_bro_move_1`, ...
  * `boomerang_bro_throw_0`, `boomerang_bro_throw_1`, ...
  * `boomerang_bro_boomerang_0`, `boomerang_bro_boomerang_1`, ...
  * `hammer_bros_[color]_move_[left/right]_0`, `hammer_bros_[color]_move_[left/right]_1`, ...
  * `hammer_bros_[color]_throw_[left/right]`
* **Thwomps**:
  * `thwomper_dormant` (idle/waiting), `thwomper_active` (slamming/angry)
* **Chain Chomps**:
  * `chained_chomp_head_[left/right]_0`, `chained_chomp_head_[left/right]_1`, ...
  * `chained_chomp_chain`, `chained_chomp_chain_squished`
* **Buzzy Beetles (Black, Blue, Grey)**:
  * `beetle_[color]_move_[left/right]_0`, `beetle_[color]_move_[left/right]_1`, ...
  * `beetle_[color]_shell`, `beetle_[color]_hide`
* **Piranha Plants (Green, Blue)**:
  * `pirhana_[color]_0`, `pirhana_[color]_1`, ...
* **Lakitus & Spinies**:
  * `lakitu_left`, `lakitu_right`, `lakitu_hide`
  * `spiny_move_[left/right]_0`, `spiny_move_[left/right]_1`, ...
  * `spiny_ball_0`, `spiny_ball_1`, ...
* **Cheep Cheeps & Bloopers**:
  * `cheep_cheep_[green/grey/red]_move_[left/right]_0`, `cheep_cheep_[green/grey/red]_move_[left/right]_1`, ...
  * `squid_move_0`, `squid_move_1`, ...
* **Bowser Boss**:
  * `bowser_move_[left/right]_0`, `bowser_move_[left/right]_1`, ...
  * `bowser_fire_[left/right]_0`, `bowser_fire_[left/right]_1`, ...
* **Projectiles & Blasters**:
  * Hammers: `hammer_black_0`, `hammer_black_1`, ..., `hammer_grey_0`, `hammer_grey_1`, ...
  * Fireballs: `flower_fireball_0`, `flower_fireball_1`, ..., `flower_fireball_hit_0`, `flower_fireball_hit_1`, ...
  * Lava Podoboos: `lava_fireball_up`, `lava_fireball_down`
  * Bullet Bills: `bullet_bill_bullet_left`, `bullet_bill_bullet_right`
  * Blaster Cannons: `bullet_bill_body`, `bullet_bill_combined`, `bullet_bill_head`, `bullet_bill_neck`, `bullet_bill_grey`

---

### D. World Scenery Spritesheet
Contains frames representing scenery, tilesets, background blocks, and goal flags.

* **Key Mappings & Special Items**:
  * `axe_0`, `axe_1`, ...: Castle bridge axe trigger.
  * `big_coin_0`, `big_coin_1`, ... (**Star Coin**): Represents the **Star Coin** collectible item (includes blue, white, and HUD variants).
  * `coin_0`, `coin_1`, ...: Level tile coin.
  * `full_flag_pole_0`, `full_flag_pole_1`, ...: Goal pole flag animation.
  * `question_block_0`, `question_block_1`, ...: World question block.
* **Terrain & Scenery**:
  * Solid blocks: `solid_block_brown`, `solid_block_grey`, `solid_block_blue`
  * Bricks & Particles: `brick_blue_one_side`, `brick_grey_one_side`, `brick_brown_particle`, `brick_blue_particle`, `brick_grey_particle`
  * Pipes: `pipe_green_up`, `pipe_green_down`, `pipe_green_side`, `pipe_red_up`, `pipe_purple_up`, `pipe_dark_green_up`, `pipe_white_grey_up`
  * Plants & Trees: `bush_light_green_long`, `tree_green_tall`, `tree_white_short`, `fence_medium`, `fence_long`
  * Castles & Flags: `castle_small`, `castle_white`, `castle_tower_brown`, `pole_flag_grey`, `pole_flag_yellow`
  * Backgrounds: `black_bg`, `blue_bg`, `dark_blue_bg`, `light_blue_bg`, `red_bg`, `lava_bg`

---

### E. Particles Spritesheet
Contains frames representing high-performance visual particle effect overlays using `PascalCase` identifiers:

* `Stomp`: Enemy stomp impact poof effect.
* `WallDust`: Wall-slide and wall-jump dust particle.
* `CoinSparkle`: Sparkle burst when collecting coins or star coins.
* `Combo`: Multi-stomp and score combo sparkle.
* `BrickBreak`: Brick block shattering debris particle.
* `WaterBubble`: Underwater swimming air bubble effect.
* `DeathPoof`: Enemy defeat smoke puff.
* `LavaEmber`: Castle lava ember spark particle.
