# R21 — Release defect batch (2026-09-02)

Branch `A/release/defect-batch-r21`, cut from `origin/dev` @ `1a7a6db`.

This is the review record for a batch that was requested as twelve defects and
turned into fifteen items. It is written so the decisions can be checked later
without replaying the session — every judgement call that could reasonably have
gone the other way is recorded here with its reasoning, including the ones the
agent got wrong first.

---

## 1. The headline finding: three defects were one bug

Reported separately as (1) half pipes, (2) a broken 1-1 Sub spawn where the
P-Switch drops you into a pit, and (4) "two sources of truth for assets, so the
level detail is different".

`tools/generate_game_levels.cpp` wrote every level **twice** — once relative to
the working directory and once to `"../" + path` — creating an untracked second
levels tree at the repository root. `ResourceManager::resolvePath` tries the
bare relative path **first**, so which tree the game read depended entirely on
where it was launched from:

| launched from | reads |
| --- | --- |
| `SuperMarioGame/` | the tracked tree — correct |
| repository root | the stray tree — stale |
| `build/` | the tracked tree, **unless** the POST_BUILD copy had not run |

The stray tree was raw generator output frozen at 2026-08-31 12:35 — **nine
minutes before** the two commits that fixed the exact defects it still
contained: `309c1a5` (sub-level floors are Ground, not Brick, so a P-Switch
cannot delete the floor under the player) and `cc6a32d` (the entrance pipe is a
full 2-column pipe, not a 1-column half). It also flattened every level's
`theme` to `overworld`, losing the underground/ice/castle backgrounds, and
carried 2–3 enemies per level against the tracked tree's 11–15.

So the same build could show or hide three different defects depending on the
user's shell history. **Whoever files a bug and whoever tries to reproduce it
were not necessarily running the same game.**

Fix: dual-write deleted, anchored `/assets/` gitignore, and
`tests/guard_asset_single_source.cpp`, which fails the build if a second tree
reappears. The gitignore alone was not enough — it keeps a stray tree out of
git, and path resolution reads the *disk*. The guard also asserts the canonical
tree is intact, so deleting the *right* tree cannot pass.

The stray tree was backed up outside the repository before removal (rule 7:
never discard uncommitted work), and the user approved the deletion explicitly.

---

## 2. Decisions taken, and why

### Approved by the user during the session

| Decision | Chosen | Alternative rejected |
| --- | --- | --- |
| Stray asset tree | Delete it **and** remove the dual-write | Hardening `resolvePath` instead — riskier, also changes what a packaged build reads, and leaves the stale copy on disk to mislead the next audit |
| Bonus D dynamic lighting | Build it for real with `sf::Shader` | Faking it with the existing vertex ring, or leaving it descoped. `SPEC.md`'s 110/115 rubric credits "GLSL shader effects" that did not exist; building it makes the claim true rather than correcting it downward |
| P2 fire key | `Period` | `M` — which is the minimap toggle, the collision `1a7a6db` was fixing |
| Phase 2 order | Editor → cheats → procedural → lighting | Parallel worktrees. All three touch `PlayingState` (3,251 lines); serialising cost wall-clock and avoided a three-way merge in the largest file in the codebase |
| `main` branch | **Not touched — the user's alone** | Fast-forwarding it. Reversed by the user mid-batch: "main is my responsibility" |
| Endless bosses | A real arena every N chunks | No bosses, or one finale boss. See §5 |

### Taken by the agent, worth a second opinion

**Pipe `y` moved by a measured rule, not a blanket offset.** Growing warp pipes
to 2×4 tiles raises every rim by 2 tiles, so the level data had to move. The
brief said "drop every pipe `y` by 2". That was applied as: *a pipe's foot sits
one row into the floor it stands on, so the rim is 3 tiles above the walking
surface* — which yields −2 for `level_1/2/3` but **−1** for the three `*_sub`
levels and `bonus_1`, whose pipes were already flush. Reason: `JUMP_HEIGHT` is
exactly 128px = 4 tiles, so seating a pipe flush puts its rim at the apex of the
jump arc, and a sub-level's exit pipe is the only way out of the room. A blanket
−2 would have made those rooms marginally unwinnable.

**A shared terrain probe went into a new header, not onto `Enemy`.** The plan
put `hasFloorAhead` on `Enemy`, which works for the two strategies but not for
`MovingPlatform` — a `Block`. Making a Block include `Enemy.hpp` to reach a
static is worse than a neutral `TerrainProbe.hpp`. Accepted.

**Kicked Koopa shells deliberately still fall into pits.** In the original games
they do. Ledge-awareness was added to HammerBro and platforms, not to shells.

**Custom levels live in `assets/levels/custom/`, not `saves/`.** `saves/` is for
save *games*. The custom-level scan is kept deliberately separate from
`LevelCatalog::levels()` so dropping a file in cannot renumber World 1-3 or
resize `CampaignProgress`.

**Immortality rescues rather than suppresses.** The user caught this: removing
the void kill without replacing it drops the player into an endless fall, which
is worse than dying. Immortal now returns the player to solid ground in the
column they fell from (then checkpoint, then spawn), keeping lives, power state,
score and timer. This mirrors `Boss::returnToArenaSpawn()` from `95521a8`, which
exists for exactly this problem — the player simply never got the equivalent.

---

## 3. Things that were believed and turned out to be false

Recorded because each one would have produced a wrong or incomplete fix.

**PressStart2P is not exactly monospace.** The plan asserted `floor(W/size)` as
a closed-form character budget. Measured through `measureTextWidth`, the advance
is hinted to whole pixels *per size* and does not always round down: **size 12
lays out at 13px/char, size 20 at 21px**, while 8, 11, 15 and 24 hit nominal. So
the closed form *overestimates* what fits, by ~8% at size 12 — which is exactly
the size the LOAD GAME page draws its slot summaries at. The verify-and-step-down
loop in `fitCharSize` is load-bearing, not defensive. A purely analytic fix
would have left the worst overflow in the game still overflowing.

**"No entities are generated in far endless chunks" — they are generated.**
`generateSolvable` runs on appended chunks through the identical code path. The
entities then *teleport away*: `MovingPlatform`, `PiranhaPlant`,
`FallingPlatform` and `Boss` each cache their spawn position at construction in
chunk-local coordinates and drive themselves back to it on frame 1, after the
splice translates them with `setPosition`. Compounded by a hard-coded prefab
margin (`x = 22 … exitX-8`) that leaves **45% of a 100-tile chunk** empty, and by
every ground enemy being nested inside the coin-cluster branch, so enemy density
tracks `coinClusterRate × enemySpawnRate` rather than the enemy slider.

**A `level_3` moving platform at tile (179,20) does not exist.** Named in the
plan; `level_3` has exactly one platform, at (112,20), with a clear sweep.

**Two "pre-existing" test failures were real regressions.** Checked against a
clean worktree at `1a7a6db`: `origin/dev` was **569/571**, never green, and
nobody had attributed it. Both came from `384250f`:
- `Camera::follow()`'s early return was replaced with `m_position =
  clampToBounds(target)`, so `Locked` mode started *chasing* the target it is
  defined to ignore — contradicting its own header comment. The stated motive
  was already covered by `snapTo()` at every release site, added by that same
  commit.
- `kKeyNames` gained no entry for `Slash`, so P2's ground pound *worked* but
  `keyName()` returned `""`: the rebinding UI showed it unbound and
  `applyBindings` silently skipped it, so `config.json` could never restore it.
  `Period` and `RControl` had the same hole from `1a7a6db`.

**Most unchecked TASKS.md boxes were stale, not open.** 10 of 12 substantive
items are shipped and working. Three notes were actively *wrong* and are
corrected: attract mode (merged as `6075c27`), LOAD GAME wiring (shipped R8,
`3d4263f`), and debug-console autocomplete (implemented). Each was verified by
hand against the code before the correction was written.

---

## 4. Mistakes made during the batch

**A bare `return` nearly shipped an unplayable game.** Gating the debug number
row used `return` inside `handleInput` — but code *after* that switch dispatches
jump and fire to both players. With debug mode off (the release default) the
player could not have jumped. Caught before commit; changed to a scoped `if`.

**A screenshot was mistaken for evidence.** The first LOAD GAME verification
shot showed three `EMPTY` slots. `"EMPTY"` is 5 characters; the defect needs a
populated slot (31–37 characters). It proved nothing and looked like proof. Re-run
against a real save, which is what `tests/scripts/verify_r21_load_overflow.txt`
now does, with a comment saying why.

---

## 5. Open items and follow-ups

- **`main` is 125 commits behind** and still carries the truncated `castle_end`
  sprite (112×176, left shoulder sliced off). Defect 3 is therefore only
  half-delivered by pushing `dev`: the *code* fix landed on dev as `384250f`,
  and the remaining half — `level_1.json`'s legacy fake-castle slab — is fixed
  here. **Promoting `main` is the user's decision and nobody else's**, stated
  during this batch; it is recorded here as an observation, not a
  recommendation, so the state of `main` is at least not a surprise later.
- **`SPEC.md` rubric** scores 110/115 citing "compensated by GLSL shader
  effects", and descopes those same shaders ~15 lines later. Building Bonus D
  for real resolves this. **If Bonus D does not land, this doc must be corrected
  downward** rather than left claiming a feature that does not exist.
- **`LevelSolvability` is vacuously true for castle/underground themes.**
  `groundRowAt` scans top-down, and those themes have a solid ceiling at y=0, so
  every column returns row 0. Also `endTileX = width-15`, so the entire exit
  apron — where a boss arena would go — is past the BFS goal and never
  validated. Must be fixed before solvability is trusted to guard an arena.
- **`MovingPlatform::update()` never calls `Block::update()`**, so
  `m_animator->update(dt)` never runs. Harmless today (single-frame art); a
  multi-frame platform animation would be frozen.
- **`Spiny::isEgg` is unreachable.** Lakitu drops a walking Spiny; the egg form
  and its hatch-on-landing transition exist and nothing constructs them.
- **`UiRenderer::wrapText`** was built with the text-fitting mechanism and has no
  production caller. Harness-only — **not** counted as complete.
- **A playtest ended by a Game Over** goes through `GameOverState` rather than
  popping back to the editor. Untested.

---

## 6. Verification standard applied

Per AGENTS.md directive 9, "complete" means reachable from `main()` **and
observed running**. Splitting this batch honestly:

**Observed running** — main menu carries no ImGui chrome with debug off; LOAD
GAME renders a real 32-character save summary inside its panel; HUD sits inside
the window; Lakitu drops spinies near the player; the editor places a Goomba, a
question block and a pipe as real sprites; a custom level saves to a shown path
and plays from the CUSTOM LEVELS menu; playtest round-trips back to the editor.

**Unit-covered but not observed** — in-level pipe rendering and castle seating
(scripted walking dies to the now-correctly-aggressive Lakitu before tile 190,
and the free-camera pan drifted off the ground row); Bowser's arena clamp in a
live 1-3 fight; the multiplayer music fix, which needs two players and audible
output that a screenshot cannot capture.

Every new suite was **mutation-tested** — reverted against the pre-fix code and
observed to fail. A guard that has not been seen to fail is not a guard.
`verify_r21_level_data` run against pre-fix data reports exactly 5 failures,
including *castle row 16 vs pole row 21*.
