# Verification captures — Shadow Mario, AI multiplayer & the 1-3 camera/gate fixes

Frames from real runs of the game, not mock-ups. Each was produced by the
in-process input scripts in `SuperMarioGame/tests/scripts/`, which drive the
actual menu and the actual input pipeline and write PNGs from the live window:

```bash
cd SuperMarioGame
./build/SuperMarioGame --script tests/scripts/shadow_chase_run.txt
./build/SuperMarioGame --script tests/scripts/versus_cpu.txt
./build/SuperMarioGame --script tests/scripts/verify_1_3_camera_and_visibility.txt
```

Output lands in `saves/shots/` (gitignored). The ones kept here are worth
citing.

| File | What it shows |
| :--- | :--- |
| `multiplayer_menu.png` | The multiplayer page. The AI skill and style rows are greyed out because the opponent is a human, and the cursor skips them. |
| `shadow_chase_replaying.png` | Shadow Mario mid-jump with its ghost trail, retracing the player's arc from three seconds earlier. Dev panel reads `Replaying: yes`, `Spatial gap: 0.90s`; the HUD gauge has gone red and `BEHIND YOU` is showing. |
| `versus_cpu_hunter.png` | A Hard Hunter that has abandoned rightward progress and come back for Player 1 — `Doing: hunting`. Compare a Normal Speedrunner, which climbs away to the right instead. Same rules, different weights. |
| `1-3_camera_at_spawn_after_fix.png` | The frame immediately after loading 1-3 from the World Map with `saves/progress.json` seeded so 1-2 (Boom Boom) is already cleared. The camera shows Mario at his own spawn corner next to a Goomba — not parked on Bowser's arena (world x ~5760–6176), which is what the pre-fix bug did every time the previous level's boss arena was still `Locked`. See `PlayingState::setupTestScene()`. |
| `1-3_player_visible_midrun_after_fix.png` | ~24s into a scripted bunny-hop run across 1-3. The player sprite stays drawn on top of every overlapping foreground object (question blocks, a Piranha Plant) rather than disappearing behind them — the symptom of the pre-fix draw order, where `adoptPlayer()` inserted the player at the *front* of `m_entities` and `render()` drew that vector in order. This run did not reach Bowser himself before the script ended; the boss-adjacent frame specifically was not captured live, and that part of the fix is verified by code review plus `ctest -R frontend_states` (`testCameraResetsAcrossALevelTransition`), not by an observed screenshot next to Bowser. |

Scripts name **bound** keys, not the built-in defaults: the `config.json` in
this repo has Player 1 on the arrow keys, so a script written for `D` moves
nothing. Reset from Options → KEYS if a script appears to do nothing.

## R12 — full playtest & evidence pass (2026-08-31)

Six items (F8/D14 closure), each through `SuperMarioGame --script
tests/scripts/verify_r12_*.txt` on `A/verify/full-playtest-pass`. Two
techniques from the plan were used throughout and are called out per item
rather than left implicit: a **temporary `spawnPoint` override** in a level's
own JSON (reverted after use, confirmed clean with `git diff`), and a
**temporary `saves/progress.json`** to unlock World Map nodes the campaign
would not otherwise have reached yet (restored afterward — it is gitignored,
so this has no effect on the tracked tree either way). Neither changes runtime
code; both are pure test setup, same as R9/R16's spawn-override technique.

| Item | Result | Evidence |
| :--- | :--- | :--- |
| (a) Boom Boom / Bowser | PARTIAL (Bowser cleared via axe; Boom Boom 2/3, then boss deleted by a respawn-safety bug — filed as D29) | `r12_bb_02.png`, `r12_axe_02_at_or_past_axe.png` |
| (b) P-Switch / POW / axe / 100-coin | POW and P-Switch: OBSERVED. Axe: OBSERVED (same run as (a)). 100-coin 1-UP: NOT ATTEMPTED | `r12_pow_03_sub_level_if_warped.png`, `r12_psw_01_after_run.png` |
| (c) 2P versus death | OBSERVED | `r12_versus_02_match_start_both_players.png`, `r12_versus_03_after_walk.png` |
| (d) Easy vs Hard, same level | OBSERVED | `r12_diff_easy_00_start.png`, `r12_diff_hard_00_start.png` |
| (e) Key rebind drives play | OBSERVED | `r12_rebind_04_jump_now_bound_to_k.png`, `r12_rebind_11_new_key_k_jumps.png` |
| (f) 60s FPS trace of 1-1 | OBSERVED — steady ~57 FPS, not a clean 60 | `r12_fps_00s.png` |

**(a) Boom Boom / Bowser.** A full blind 1-1→1-3 campaign walkthrough was not
attempted: the harness has no positional feedback, and R9/R16 already spent
9 and 13 attempts respectively establishing that a blind run of one ~95-185
tile level is close to a coin flip. Instead each boss arena was reached
directly, via a temporary `spawnPoint` a few tiles inside its own arena (and,
for 1-3, a temporary `progress.json` so the World Map opened past 1-2) — say
so plainly rather than claiming a played-through approach.
- **Bowser: full defeat, 1 attempt.** Spawned at (187,19) in `level_3.json`,
  three tiles from `bridge_axe` (190,20), inside Bowser's own locked arena
  (arenaX 180, width 13 — the arena clamps the player's `x` but does not stop
  them walking to the axe while it is active). Touching the axe calls
  `Boss::defeatNow()` unconditionally (`PlayingState::chopBridge`,
  PlayingState.cpp:1343) — this is SPEC's *other* victory condition, not the
  fireball/stagger loop, and is reported as such. Result: `Level complete!`,
  achievements `Speed Demon` and `Toad Unlocked (Complete all 3 levels)`,
  `Entering VictoryState (1-3)`, HUD reads `LEVEL CLEAR! WORLD 1-3` (see
  `r12_axe_02_at_or_past_axe.png`).
- **Bowser's 4-fireball/3s stagger loop: 1 attempt, NOT ACHIEVED.** This needs
  Fire Mario, which needs a Mushroom (tile 36,16) then a Fire Flower (tile
  66,17) collected before the ~114-tile run to the arena. Spawned at (36,19)
  to stand under the Mushroom block; the player was killed almost immediately
  by whatever fortress hazard (Thwomp/Bullet Bill/Chain Chomp — all three are
  in this level per R9) sits near that exact tile, respawned twice more, and
  reached Game Over without ever landing the first jump. One bounded attempt
  only, per the plan's own calibration that a long blind run is a coin flip
  and repeated attempts have poor time-to-value here; **no read on whether the
  4-fireball/3s stagger feels tunable was obtained this session.**
- **Boom Boom: 2 of 3 hits landed, then the boss vanished — not from being
  defeated.** Spawned at (178,19) in `level_2.json`'s R15-widened arena
  (arenaX 176, width 16) and jumped repeatedly while walking back and forth.
  Stdout: `BOOM BOOM entering phase 2`, then `entering phase 3` — two stomps
  landed cleanly, HUD screenshot `r12_bb_02.png` shows his health bar down to
  its last third. The player then took a hit and died; on respawn, stdout
  printed `Boss arena released` and `Cleared 1 enemy(ies) from the respawn
  point`, and Boom Boom was gone from every screenshot after — **the
  respawn's "clear anything within 2.5 tiles" safety net
  (`PlayingState.cpp:2701-2718`) destroyed the boss entity itself**, since it
  clears anything with `EntityCategory::Enemy` and bosses are Enemies too. Not
  a fix made here (scope discipline) — filed as new issue D29 below.

**(b) P-Switch / POW / axe / 100-coin.** POW and P-Switch were each reached
the same way as the axe: a temporary `spawnPoint` a few tiles from the target,
skipping the ~95-tile blind overworld approach R16 recorded a 1-in-13 rate on.
- **POW: OBSERVED, 1 attempt.** Spawned in `level_1.json` directly above the
  warp pipe at (98,19) — chosen after an earlier attempt spawning *beside* the
  pipe got the player wedged under a staircase overhang with velocity pinned
  at (0,0) even after jump presses, diagnosed only once the "Gameplay
  Controls & Navigation" dev panel's live Player Position readout was made
  visible (`imgui_layout_v2.ini`, gitignored, pre-seeded before each run in
  this session). Falling onto the pipe's flat top and holding crouch warped
  into `level_1_sub.json`; stdout: `POW block: flipped 1 enemy(ies).`
- **P-Switch: OBSERVED, 1 attempt after a first miss.** The first combined
  attempt ran/jumped straight past it (confirmed by the dev panel: position
  frozen well beyond the switch's tile, having cleared it in mid-air). A
  focused follow-up computed the hold duration from `Constants::RUN_SPEED`
  (960px at 300px/s = 3.2s) instead of guessing, with the one jump timed for
  the Goomba between spawn and the switch rather than for the switch itself —
  so the player was grounded, not airborne, crossing it. Stdout: `P-Switch:
  swapped 11 tile(s) for 15s.`; HUD shows a live `P-SWITCH 13→12` countdown
  with bricks turned to coins (`r12_psw_01_after_run.png`).
- **100-coin 1-UP: NOT ATTEMPTED.** No time was budgeted for it this session
  after the above; it needs either a dense coin run or a bonus room and was
  deprioritised in favour of (a), (d), (e) and (f), which had a better
  time-to-value ratio.

**(c) 2P versus round ending in a death.** OBSERVED, 1 attempt, no
traversal-luck involved. `MatchConfig` defaults to `GameMode::VersusHuman`
with a human opponent, so opening Multiplayer and going straight to START —
without touching MODE or OPPONENT — starts a real two-human match on 1-1.
Player 2 (arrow keys, InputManager's own P2 defaults) was walked straight at
the Goomba 9 tiles from spawn *without ever jumping*; a side touch on a Small
player is not a stomp, and `Player::powerDown()` publishes `PlayerDied` the
instant a Small player takes a hit. Stdout: `Player 2 was defeated.` /
`Player 2 respawned at (128, 624). Lives remaining: 2` — matching the HUD's
P2 lives counter dropping from `x3` to `x2` between
`r12_versus_02_match_start_both_players.png` and `r12_versus_03_after_walk.png`.
One navigation false start is worth recording separately: `press A` after
opening the Multiplayer page does not land on CharacterSelectState as an
older script's comment assumes — see the note against `versus_cpu.txt` below.

**(d) Easy vs Hard, same level.** OBSERVED, no traversal needed — this is a
static, code-driven readout at the instant 1-1 loads, so it needed one run per
difficulty rather than a coin-flip level clear. `saves/config.json`'s
`difficulty` was set to `easy`, then `hard`, immediately before each run and
restored to `normal` afterward. `IDifficultyStrategy` (`DifficultyStrategy.hpp`)
values are direct HUD reads, not hardcoded: Easy shows `Lives: 5` and `TIME
387` (5 lives, and 300×1.30=390 scaled by the ~3s of menu transition already
elapsed); Hard shows `Lives: 2` and `TIME 222` (2 lives, 300×0.75=225 scaled
the same way) — `r12_diff_easy_00_start.png` / `r12_diff_hard_00_start.png`.

**(e) Rebind a key and prove it drives play.** OBSERVED, 1 attempt.
`saves/config.json` was read first per this README's own warning above:
Player 1 had `jump` on `W`. Player 1's JUMP was rebound to `K` through the
real Options → Controls UI (seven Down presses from START to reach OPTIONS,
Tab to the Controls page, two Down presses to the JUMP row, Enter, then `K`) —
`r12_rebind_03_on_p1_jump_row.png`/`r12_rebind_04_jump_now_bound_to_k.png`
show the row's own bound-key column change from `W` to `K` live, through
`InputManager::applyBindings` (the same path R7 confirmed is wired from
`Game.cpp:45,48,439`). Back in 1-1 in the same run (no restart needed —
`Game::setKeyBinding` applies live, not just on save): pressing the *old* key
`W` did nothing (`r12_rebind_09_old_key_w_no_jump.png`, Mario still grounded);
pressing the *new* key `K` sent Mario airborne
(`r12_rebind_11_new_key_k_jumps.png`). One side effect worth flagging for
whoever runs the next script here: `saves/config.json`'s `jump` binding is
only restored to `W` on `Game::quit()`'s save — if a script running right
after a rebind script still expects `W` to jump and the rebind script did not
get its config restored first, `press W` will silently do nothing. That is
exactly what happened once in this session (diagnosed via the same dev-panel
Position/Velocity readout, then fixed by restoring `saves/config.json` from
this session's own pre-flight backup before continuing).

**(f) 60-second FPS trace of 1-1.** OBSERVED. The `Game.cpp:176` readout
(`Application Average: %.3f ms/frame (%.1f FPS)`) is drawn to an ImGui panel,
not logged to stdout, and the panel ships collapsed by default
(`ImGuiCond_FirstUseEver`) with no mouse available to this harness to expand
it — worked around by pre-seeding the repo's own persisted, gitignored
`imgui_layout_v2.ini` with that window's `Collapsed=0` before the run (a saved
layout wins over `ImGuiCond_FirstUseEver` by the engine's own design; see
`Game::initImGui`'s comment). Seven shots at 0/10/20/30/40/50/60s of elapsed
in-level time (`r12_fps_00s.png` is the first) all read within
54.1–57.3 FPS / 17.4–19.0 ms per frame, including across a death and a
Game Over screen mid-trace (the panel renders every frame regardless of game
state) — steady, but consistently a little under the 60fps cap
(`m_window->setFramerateLimit(60)`), not exactly 60. Reported as observed, not
tuned — SCOPE DISCIPLINE means this was not chased further.

**Note on `tests/scripts/versus_cpu.txt`:** its own comment says `press Down`
once from START reaches the Multiplayer page. That was true before R8 added
the LOAD GAME row between START and VERSUS; on this session's `dev` HEAD
(R10's attract-mode merge) `press Down` once now lands on LOAD GAME instead,
and the script's next input (`press Enter`) would open the Load page rather
than Multiplayer. Not fixed here (scope discipline: touching an existing
verification script beyond this note was out of scope) — the R12 scripts in
this session use `press Down` **twice**, confirmed working.

New scripts from this session:
`verify_r12_fps_trace_1_1.txt`, `verify_r12_key_rebind.txt`,
`verify_r12_versus_human_death.txt`, `verify_r12_pswitch_pow_via_spawn_override.txt`,
`verify_r12_pswitch_focused.txt`, `verify_r12_bridge_axe.txt`,
`verify_r12_bowser_fireball_attempt.txt`, `verify_r12_boomboom_fight.txt`,
`verify_r12_difficulty_easy.txt`, `verify_r12_difficulty_hard.txt`.
