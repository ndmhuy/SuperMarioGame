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
