# Super Mario Game — Required Sprites and Animations List

> [!NOTE]
> **Sprite Asset Source**: The player, item, and world spritesheets implemented in this project are sourced from the **SMB1 sheets on the Mario Mayhem page**.

This document lists all the sprites and animations required for the 16-bit SNES-style Super Mario Game, categorized by characters, enemies, items, blocks, and visual effects, along with recommended web asset links.

---

## 1. Player Characters (Mario, Luigi, Toad, Peach)
Each player character needs sprite variations for their active power-up states:
*   **Small** (1-tile / 32px height)
*   **Super / Fire / Cape** (2-tile / 64px height)
*   **Mini** (0.5-tile / 16px height)
*   **Mega** (4-tile / 128px height)

| Character | Power-Up State | Animation States (State IDs) | Detailed Sprite Frames Needed | Recommended Sprite Sheet Link |
| :--- | :--- | :--- | :--- | :--- |
| **Mario** | All States | `idle`, `walk`, `run`, `jump`, `fall`, `crouch`, `slide`, `wall_slide`, `ground_pound`, `swim`, `climb`, `skid`, `damaged`, `death` | Idle (1), Walk (3), Run (3), Jump (1), Fall (1), Crouch (1), Slide (1), Wall Slide (1), Ground Pound (2), Swim (2), Climb (2), Skid (1), Damaged (1), Death (1) | [MFGG - Expanded SMW Mario/Toad Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=42226) |
| **Luigi** | All States | `idle`, `walk`, `run`, `jump`, `flutter`, `fall`, `crouch`, `slide`, `wall_slide`, `ground_pound`, `swim`, `climb`, `skid`, `double_jump`, `damaged`, `death` | Same as Mario, plus unique `flutter` kick (2 frames) and `double_jump` flip frame (1) | [SMW Central - SMW Styled Luigi Beta/SMA2](https://www.smwcentral.net/?p=section&a=details&id=42573) |
| **Toad** | All States | `idle`, `walk`, `run`, `jump`, `fall`, `crouch`, `slide`, `swim`, `climb`, `skid`, `damaged`, `death` | Same states as Mario, but optimized for Toad's proportions | [SMW Central - SMM2 SMW Toad Sheet](https://www.smwcentral.net/?p=section&a=details&id=28627) |
| **Peach** | All States | `idle`, `walk`, `run`, `jump`, `float`, `fall`, `crouch`, `slide`, `swim`, `climb`, `skid`, `damaged`, `death` | Same states as Mario, plus dress `float`/hover animation (2 frames) | [MFGG - SMW Peach](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=239) <br> [MFGG - Playable Princesses Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=41447) |
| **Fire State (All)** | Fire | `shoot` | Action frame throwing/shooting a fireball | Shared in respective character sheets |
| **Cape State (All)** | Cape | `glide`, `swoop`, `spin` | Glide (2), Swoop dive (1), Tail/Cape spin attack (3) | Shared in respective character sheets |
| **Star Overlay** | Star | `star_flash` | Palette/color cycle frame cycle (6 colors, 0.1s interval) | Handled dynamically via shader/palette swap |

---

## 2. Enemies
| Enemy Name | Type/Variant | Animation States (State IDs) | Detailed Sprite Frames Needed | Recommended Sprite Sheet Link |
| :--- | :--- | :--- | :--- | :--- |
| **Goomba** | Brown (Default) | `walk`, `squished`, `flipped` | Walk (2), Squished/Stomped (1), Flipped (1) | [SMW Central - SMW Styled Goomba](https://www.smwcentral.net/?p=section&a=details&id=41781) |
| **Red Goomba** | Red (Ledge-aware)| `walk`, `squished`, `flipped` | Walk (2 - red palette), Squished (1), Flipped (1) | [SMW Central - SMW Styled Goomba](https://www.smwcentral.net/?p=section&a=details&id=41781) |
| **Koopa Troopa** | Green (Default) | `walk`, `shell`, `shell_spin`, `flipped` | Walk (2), Hide in Shell (1), Shell Spin/Slide (2), Flipped (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Red Koopa** | Red (Ledge-aware) | `walk`, `shell`, `shell_spin`, `flipped` | Walk (2 - red palette), Hide in Shell (1), Shell Spin/Slide (2), Flipped (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Koopa Paratroopa**| Green | `fly`, `walk`, `shell`, `shell_spin`, `flipped` | Fly wings flap (2), Walk (2), Hide in Shell (1), Shell Spin (2), Flipped (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Red Paratroopa**| Red (Stationary) | `fly`, `walk`, `shell`, `shell_spin`, `flipped` | Vertical bounce fly (2 - red), Walk (2), Shell (1), Shell Spin (2), Flipped (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Boo (Ghost)** | White | `idle`, `shy`, `chase` | Float idle (1), Cover face (1), Grinning chase (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Piranha Plant** | Green/Red | `bite` | Jaw open/close biting (2 frames emerging from pipe) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Bullet Bill** | Black | `fly` | Moving horizontal projectile (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Bill Blaster** | Iron/Grey | `idle` | Stationary cannon block (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Hammer Bro** | Green helmet | `walk`, `throw_prep`, `throw` | Walk (2), Hammer swing prep (1), Hammer release (1), Rotating Hammer (4) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Thwomp** | Grey stone | `idle`, `slam`, `rise` | Normal idle (1), Slam impact/grind face (1), Rise floating (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Chain Chomp** | Black metallic | `idle`, `lunge` | Idle breathing/bite (2), Lunge attack stretch (1), Peg & Chain Link (1 each) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Lakitu** | Green shell | `fly`, `throw` | Cloud hover (2), Spiny egg throw release (1), Rolling Spiny egg (2) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Spiny** | Red spiked shell | `walk`, `shell_spin`, `flipped` | Spiked walk (2), Spiked ball roll (2), Flipped (1) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Boom Boom** | Mid-Boss | `idle`, `charge`, `spin`, `stunned`, `fly` | Idle (1), Charge walk (2), Shell spin (3), Stunned recovery (1), Winged fly (2) | [MFGG - SMW Enemies and Stuff](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=115) |
| **Bowser** | Final Boss | `idle`, `walk`, `jump`, `breathe_fire` | Idle (1), Heavy Walk (3), Jump/Stomp (1), Fire breathing (2), Fireball (4) | [MFGG - Bowser & Clown Car](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=27840) <br> [SMW Central - Alternate Bowser](https://www.smwcentral.net/?p=section&a=details&id=40901) |

---

## 3. Items & Power-ups
| Item Name | Animation States (State IDs) | Sprite / Animation Requirements | Recommended Sprite Sheet Link |
| :--- | :--- | :--- | :--- |
| **Super Mushroom** | `idle` | Classic red-spotted mushroom (1 frame) | [MFGG - Extended Items Remake 1](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=34776) |
| **Fire Flower** | `glow` | Animated rotating color petals (4 frames) | [MFGG - Extended Items Remake 1](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=34776) |
| **Coin** | `spin` | Spinning animation (4 frames) | [MFGG - Extended Items Remake 1](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=34776) |
| **Star** | `glow` | Invincibility star color cycling (4 frames) | [MFGG - Extended Items Remake 1](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=34776) |
| **1-UP Mushroom** | `idle` | Green-spotted mushroom (1 frame) | [MFGG - Extended Items Remake 1](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=34776) |
| **Cape Feather** | `float` | Floating feather animation (2 frames side-to-side) | [MFGG - Super Mario Items (SMW Style)](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=19419) |
| **Mega Mushroom** | `idle` | Large orange/yellow spotted mushroom (1 frame) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
| **Mini Mushroom** | `idle` | Tiny blue-spotted mushroom (1 frame) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
| **POW Block** | `idle`, `vibrate` | Blue POW block (1 frame), vibrating shake (2 frames) | [MFGG - Super Mario Items (SMW Style)](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=19419) |
| **P-Switch** | `idle`, `pressed` | Blue switch (1 frame), Pressed flat state (1 frame) | [MFGG - Super Mario Items (SMW Style)](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=19419) |
| **Trampoline** | `idle`, `bounce` | Spring block (1 frame), Compressed bounce states (3 frames) | [MFGG - Super Mario Items (SMW Style)](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=19419) |
| **Star Coin** | `spin` | Glowing coin with star rotating (4 frames) | [MFGG - Super Mario Items (SMW Style)](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=19419) |

---

## 4. Blocks & Level Tilesets
| Block/Tileset Name | Animation States (State IDs) | Sprite / Animation Requirements | Recommended Tileset Link |
| :--- | :--- | :--- | :--- |
| **Grass Level Tileset** (Level 1) | `idle` | Soil, grass slopes, background tree/cloud assets (static) | [MFGG - SMW Grass Tileset Remastered](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=36619) <br> [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Cave Level Tileset** (Level 2) | `idle` | Dark rock/stone tiles, Ice Block elements, BGOs (static) | [MFGG - SMW Cave Remastered](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=38785) <br> [MFGG - SMW styled Sewer Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=42107) |
| **Castle Level Tileset** (Level 3) | `idle` | Castle stone walls, hazard platforms (static) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Brick Block** | `idle`, `hit` | Brick pattern tile (1 frame), Shattered debris fragments particles (4 frames) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Question Block** | `idle`, `empty` | Glowing "?" block (animated 4 frames), Empty solid block (1 frame) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Hidden Block** | `invisible`, `empty` | Invisible editor block, Solid empty Question Block when hit (1 frame) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Warp Pipe** | `idle` | Pipe lip, body, side entrance tiles (static) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Flagpole** | `idle`, `flag_slide` | Flagpole (1), Flag sliding down (animated 2 frames) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Moving Platform** | `idle` | Platform tile with support logs (static) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Falling Platform** | `stable`, `shake`, `fall` | Stable stone (1), Shaking cracking stone (2 frames vibrating), Falling debris particles (4) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |
| **Ice Block** | `idle` | Light-blue ice sheet tile (static) | [MFGG - SMW Cave Remastered](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=38785) |
| **Conveyor Belt** | `convey` | Belt loops with moving arrows (animated 3 frames) | [MFGG - Practical Test Tileset](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=37673) |

---

## 5. UI, HUD, & Particle Effects
| Element / Effect | Animation States (State IDs) | Sprite / Visual Requirements | Recommended Asset Source |
| :--- | :--- | :--- | :--- |
| **HUD Icons** | `idle` | Mini player head icons, Coin symbol, Star outlines (static) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
| **Boss Health Bar** | `idle`, `flash` | Segmented health bars (1), Flashing low health frame (2) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
| **Floating Score Text**| `float` | Font outlines for score popups (fades out while drifting upward) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
| **Minimap Dots** | `idle` | Colored marker dots (procedural drawing - no sprites needed) | Standard procedural shape drawing (e.g. `sf::CircleShape`) |
| **Particles** | `emit` | Dust puffs, coin sparkles, death poofs, lava bubbles, splashes (animated 3-4 frames each) | [MFGG - SMM2 Misc Items Sheet](https://mfgg.net/index.php?act=resdb&param=02&c=1&id=39406) |
