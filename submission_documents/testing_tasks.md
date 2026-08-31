# Member Contributions & Manual Testing Tasks

## Overall Contribution Split

| Member | Role | Contribution Focus | Assigned % |
| :--- | :--- | :--- | :--- |
| **Member A (Nguyễn Đình Minh Huy)** | Engine & Infrastructure | Game Loop, Physics Engine, Collision Systems, Level/World Systems, Serialization, Level Editor, Replay/Rewind, Multiplayer AI (Shadow Mario, CPU opponent), Advanced Systems. | 50% |
| **Member B (Trần Gia Huy)** | Entities & Gameplay | Player States & Power-ups, Enemy AI & Bosses, Items & Blocks, Game Flow/UI, Audio, Controls, Visual Effects, Accessibility, Polish & Game Feel. | 50% |

---

## Manual Testing Task Division

Every one of the 97 features in [`features_list.md`](features_list.md) is
assigned below to the member whose domain it falls into, grouped by that
document's own 16 categories so a tester can work through it category by
category with the feature numbers open side by side. This replaces the
previous six-bullet-per-member sketch with a checklist that actually covers
the full list.

### Member A — Engine, Physics, World & Systems Testing

| # | Category (features) | Manual test |
| :-- | :--- | :--- |
| 1 | Core Engine & Architecture (**1–10**) | Play for 10+ minutes across at least two levels; confirm no frame-rate stutter, no dangling ImGui window, and that rebinding a key in Options survives a restart. Open the debug console (`` ` ``) and run every one of `help/give/lives/tp/god/spawn/difficulty/level/progress/replay/clear` at least once. |
| 2 | Physics & Collision (**11–16**) | Deliberately test edge cases: jump into a wall corner, walk onto ice (1-2) and a conveyor (1-3) back to back, and stomp an enemy at the exact frame you also touch a block from below. Confirm no jitter or tunnelling. |
| 3 | Advanced Movement (**17–22**) | Wall-jump off a shaft, ground-pound from a Mega state and a normal state, crouch-slide under a 1-tile gap, and chain 3+ stomps without touching ground to confirm the combo counter increments and resets correctly on landing-idle or damage. |
| 4 | Levels & World (**60–64, 96–97**) | Play all 3 main worlds + Bonus Stage to their flagpole, enter and exit all 3 sub-vaults through their pipes, confirm the World Map unlocks each node in order and shows the right star-coin pips, then open the Level Editor (F1), place an entity, save, and reload it. From Main Menu → Procedural Level → Play Endless, run right for at least 2 minutes: confirm the distance counter climbs in the world-label HUD field, at least one chunk boundary is crossed with no visible break in the terrain, and dying shows "WORLD ENDLESS - Nm" with a recorded high score. Use "Generate & Play" 5+ times across different themes/difficulties and confirm every generated level is completable start to flagpole (the solvability check should mean you never see an impossible gap). |
| 5 | Save/Load & Persistence (**75–78**) | Save to a slot mid-level, quit, and reload it — confirm position, lives, coins and score match. Record a run's high score and confirm it appears in Options → Records. |
| 6 | Two-Player & AI Opponents (**79–82**) | Start a Versus Human match and a Versus CPU match (try both a Speedrunner and a Hunter profile) and a Shadow Chase match; confirm Player 2 controls respond independently and Shadow Mario visibly replays your own prior path. |
| 7 | Advanced Systems (**89–95**) | Hold the rewind key mid-jump and confirm the game rewinds cleanly; save and play back a replay from the console; clear the campaign once and confirm New Game+ starts with faster enemies; run the Daily Challenge twice on the same day and confirm the level is identical both times; toggle Colorblind Mode and confirm the minimap recolors. |

### Member B — Entities, Gameplay & Polish Testing

| # | Category (features) | Manual test |
| :-- | :--- | :--- |
| 1 | Characters & Player States (**23–31**) | Play a full level as each of Mario, Luigi, Toad and Peach, noticing the speed/jump differences. Take every power-up in sequence (Small → Super → Fire → Cape, and separately Mini and Mega) and confirm the hitbox and sprite change each time; take a Star while in Fire form and confirm you return to Fire form when it ends. |
| 8 | Enemies & AI (**32–41**) | Encounter every enemy type at least once (place the editor-only ones — Paratroopa, Boo, Thwomp, Lakitu, Bullet Bill, Chain Chomp — via the Level Editor if they are not in your current playthrough) and confirm each one's movement strategy matches its description (patrol, chase, fly, timer emergence, tethered lunge). |
| 9 | Bosses (**42–46**) | Fight Boom Boom in 1-2 and Bowser in 1-3 to completion. Confirm the boss health bar only appears during the fight, the arena will not let you leave or reach the flagpole until the boss is dead, and each boss's phase 2/3 attack pattern actually changes. |
| 10 | Items & Blocks (**47–59**) | Trigger every item at least once (all 7 power-ups, a POW Block, a P-Switch, a Trampoline) and every block type (Question, Brick, a Moving Platform, a Falling Platform, ice and conveyor tiles, a Warp Pipe). Finish a level and confirm the castle's flag rises and you visibly walk up to its door before the summary screen appears. |
| 11 | Game Flow & UI (**65–71**) | Walk the full Main Menu → Pause → Options → Records loop. Toggle the Minimap (Tab) mid-level, open the Statistics screen, and die on purpose to confirm the death tally and instant "Retry Level" both work. |
| 12 | Audio (**72–74**) | With sound on, confirm each listed SFX actually plays for its trigger (jump, coin, stomp, power-up, damage, fireball, death, flagpole, pipe, ground pound, P-Switch, achievement) and that the music changes between the menu, world map, a level, a boss fight and Game Over. Move both volume sliders and confirm the change is audible. |
| 13 | Controls (**83–84**) | Rebind at least 3 actions to new keys in Options, confirm they work in-level, then use "Reset Controls" and confirm the defaults return. |
| 14 | Visual Effects (**85–88**) | Break a brick, collect a coin, stomp an enemy with a combo active, and take damage — confirm a particle burst, the correct death animation, and the invincibility flash all appear as described. |
| — | Accessibility (**95**, cross-referenced with Member A's #7) | Confirm the colourblind palette swap is visible on the minimap and any debug outlays without needing the debug console. |

### Cross-cutting: play at least one full session together

However the two domains above are split, finish with one full co-op or
versus session played together end to end (Main Menu → a full level → Victory
→ World Map → a second level → Game Over or campaign clear), so at least one
manual pass exercises the transitions between every system in this table
rather than each system in isolation.
