# Verification captures — Shadow Mario & AI multiplayer

Frames from real runs of the game, not mock-ups. Each was produced by the
in-process input scripts in `SuperMarioGame/tests/scripts/`, which drive the
actual menu and the actual input pipeline and write PNGs from the live window:

```bash
cd SuperMarioGame
./build/SuperMarioGame --script tests/scripts/shadow_chase_run.txt
./build/SuperMarioGame --script tests/scripts/versus_cpu.txt
```

Output lands in `saves/shots/` (gitignored). The three kept here are the ones
worth citing.

| File | What it shows |
| :--- | :--- |
| `multiplayer_menu.png` | The multiplayer page. The AI skill and style rows are greyed out because the opponent is a human, and the cursor skips them. |
| `shadow_chase_replaying.png` | Shadow Mario mid-jump with its ghost trail, retracing the player's arc from three seconds earlier. Dev panel reads `Replaying: yes`, `Spatial gap: 0.90s`; the HUD gauge has gone red and `BEHIND YOU` is showing. |
| `versus_cpu_hunter.png` | A Hard Hunter that has abandoned rightward progress and come back for Player 1 — `Doing: hunting`. Compare a Normal Speedrunner, which climbs away to the right instead. Same rules, different weights. |

Scripts name **bound** keys, not the built-in defaults: the `config.json` in
this repo has Player 1 on the arrow keys, so a script written for `D` moves
nothing. Reset from Options → KEYS if a script appears to do nothing.
