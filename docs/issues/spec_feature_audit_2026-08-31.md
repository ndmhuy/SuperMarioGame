# SPEC / Feature / Report Audit — 2026-08-31

**What this is**: the reconciliation of `SPEC.md` v2.0 (the frozen 110-feature
specification), the code on `main`, and every claim the reports make
(`submission_documents/features_list.md`, `reports/report_content.py` and its
generated editions, `TASKS.md`, `TASK_DIVISION.md`, `docs/issues/*`), plus a
**prioritised remaining-work plan** with a ready-to-paste agent prompt, a
recommended Claude model and an effort level for every open item.

**Who it is for**: whoever runs the next work sessions on this project, and any
grader who wants to know how the claims were checked.

**Repo state when audited**: `main` @ `ab16759` (== `origin/main`), `dev` ==
`origin/dev`, `dev` content-identical to `main` (`main` carries 9 merge commits
`dev` lacks). `git fetch --all` was run before anything was read. Method: three
parallel read-only audit passes (SPEC↔report reconciliation; per-checkbox triage
of both task files against code reachable from `main()`; defect/issue sweep over
`docs/issues/*`, the agent log, GitHub issues and CMake).

**Methodological correction (2026-08-31 reconciliation session)**: Of the 13 playtest-derived defects (D16–D28) folded into this document on 2026-08-31, four (D20, D25, D27, D28) were already fixed or are not demonstrable when recorded. These observations were folded into this document without re-verification against the code being audited at the time: D20 (Bowser never spawns) was refuted by live evidence in the same session it was recorded; D25 (sprite stretch during transform) was struck when it was discovered no transform animation exists in the codebase at all; D27 (death SFX cut off by respawn) is not demonstrable as worded — the SFX is longer than any respawn window and is not stopped in code; D28 (colorblind setting has no effect) was already fixed by commit 2e7f5bb on 2026-08-19, twelve days before this audit flagged it. The remaining nine defects (D16–D19, D21–D24, D26) were genuine. **Future audit passes should re-verify each observation against the audited commit before recording it** to avoid repeating this fold-in-and-discover pattern.

---

## 1. Verdict in three lines

1. The project is in far better shape than its own checklists said: of 126
   unchecked boxes, **37 were already done and wired**, 56 were procedural noise
   from an abandoned branch-naming scheme, 2 deliberately descoped — only
   **31 represent real remaining work**, most of it small wiring gaps.
2. The reports were largely honest but carried **17 wrong or stale claims**
   (counts, constants, a contradictory preamble, a wrong controls table, a
   missing Endless Mode section). All were corrected in this pass — see §2.
3. **Two live gameplay defects** were found in passing (World 1-3 plays the
   underwater theme; Spiny's despawn compares world-Y to a screen constant),
   plus four residuals from the August 18 audit that were claimed or assumed
   closed and are not (A-10 partial, A-11 descoped-in-place, X-7 unadopted,
   X-8's `.member_profile.json` still tracked).

## 2. Report corrections applied in this pass (branch `A/docs/spec-feature-audit`)

`submission_documents/features_list.md`:
- Preamble no longer lists Endless Mode as unconfirmed (it shipped as #96 on
  2026-08-31; the preamble had never been updated and contradicted the list).
- #3: "and 8 more, all Meyers singletons" → 13 total ("and 7 more"), with the
  `ResourceManager` never-destroyed-instance exception stated.
- #4: EventBus "35+ event types" → **28** (`include/Core/EventBus.hpp`).
- #12: "Ten-Stage" pipeline → **Nine-Stage** (stages 1, 1.1, 1.5, 2, 3, 4, 4.5,
  5, 6 in `PhysicsEngine.cpp`; broad+narrow share stage 5).
- #38: Boo pursuit 100 → **80 px/s** (`BOO_SPEED`, Constants.hpp:62).
- #39: Thwomp slam 700 → **600 px/s** (`ENEMY_THWOMP_SLAM_SPEED`, Constants.hpp:80).
- #51: trampolines in "both" → **all three** sub-vaults.
- #76: auto-save claim corrected — only the debug checkpoint key publishes
  `CheckpointActivated`; the warp-pipe trigger was deliberately removed.
- #97: solvability wording no longer claims a shipping gate — if all bounded
  reseeds fail, the last layout is kept and logged (`MapGenerator.cpp:411-414`).

`reports/report_content.py` (and therefore every generated edition):
- "86 harnesses" (×4 sites) → historical framing with the true in-tree count
  (`{F['harnesses']}` = 23); "497 checks" → 504 at the last logged run;
  "92 validated frames" → 236; "Twelve items" (×3 sites) → thirteen;
  menu caption "Seven entries" → eight; physics pipeline sentence now says nine
  numbered stages presented as ten rows.
- Controls table fixed: P1 has no arrow bindings and no J-fire; P2's run is N
  and Right Shift is P2's second **jump** (`InputManager.cpp:70-96`).
- New §8.12 documents `MapGenerator`, `LevelSolvability` and Endless Mode —
  shipped systems the report never mentioned.
- §13 "What is not done" now lists: the 1-3 BGM defect, the inert
  `AnimationManager`, the solvability keep-anyway fallback, and the six
  never-placed enemy types / missing hidden block (which leaves the
  `secret_finder` achievement unreachable in the campaign).

`docs/issues/completion_plan.md`: Tier 4 corrected from "DONE" to "3 of 4" —
item 25 (X-7 RAII subscription token) was written but never adopted.

`TASKS.md` / `TASK_DIVISION.md`: the 37 verified-done boxes ticked, with an
audit-note header; `Commit:`/`Merge:` boxes left as documented-moot.

## 3. SPEC features that exist in no report and no code (the honest gap list)

Named in the features-list preamble as exclusions (re-verified still absent):
climbing/vines (§4.3.5) · ground-pound hover pause + 2-tile enemy stun (§4.3.2)
· skid mechanic (§4.3.7) · knockback input-lock (§4.3.6) ·
`noclip`/`kill_all`/`stats` console commands (§16.5) · dynamic lighting shaders
(§19.4).

Never named anywhere until now:
slide-kills-enemies (§4.3.3) · swim stroke / ×0.6 water speed / water fireballs
— **no shipped level contains a single water tile** (§4.3.4) · in-gameplay
character-switch hotkey (§5.1) · Cape swoop; Mini walk-on-water & mini-pipes
(§5.2) · red enemy variants — ctor params exist, `EntityFactory` hardcodes
`false` (§6.3) · mid-level checkpoint flags in level data — only the debug key
publishes `CheckpointActivated` (§9.2) · timed 15 s bonus rooms (§9.4) ·
vertical-scroll / autoscroll sections (§9.5) · attract mode & Load Game menu
row (§10.2) · floating score text (§10.5) · surface footsteps — 3 WAVs loaded,
never played (§11.4) · dynamic music layers (§11.5) · save-slot preview (§12.3)
· water sine-wave / heat shimmer (§15.4) · Bullet Bill & floating-text pools
(§16.1) · menu audio navigation cues (§17.3) · Bowser-stomp & Mega-movement
screen shakes (§3/§15.1) · hidden block placed in any shipped level (§8).

These are candidates for either implementation (see §5) or a formal SPEC
descope addendum (task R6) — not for silent omission.

## 4. Open defects (code is wrong or dishonestly idle)

| ID | Defect | Where | Severity |
| :-- | :--- | :--- | :-- |
| D1 | ~~World 1-3 plays "underwater" as level BGM; the registered `castle` track is never used~~ | ~~`SoundManager::playLevelBGM` maps index 2 → underwater; index 2 in `LevelCatalog` is Bowser's Castle~~ | **RESOLVED (R1):** Fixed in `SoundManager::playLevelBGM` to map 0→overworld, 1→underworld, 2→castle, 3→sub_space. |
| D2 | ~~Flipped Spiny despawn compares world `position.y` to `Constants::WINDOW_HEIGHT` (screen constant); correct only while levels are exactly 720 px tall~~ | ~~`src/Entities/Spiny.cpp:35`~~ | **RESOLVED (R1):** Despawn now checks against the tilemap's pixel height instead of screen constant. |
| D3 | ~~Solvability result discarded: both `PlayingState` call sites ignore `generateSolvable`'s return; an unverified layout ships silently~~ | ~~`PlayingState.cpp:137, 1791, 1849`; `MapGenerator.cpp:411-414`~~ | **RESOLVED (R1):** Solvability result now surfaced with HUD note ("layout unverified") or warn-level logging. |
| D4 | ~~`tests/verify_sound_visual.cpp` exists but is registered in no CMake target — dead test that can never run~~ | ~~`SuperMarioGame/CMakeLists.txt`~~ | **RESOLVED (R1):** Test registered via `add_verify_test()` in CMakeLists.txt. |
| D5 | ~~`AnimationManager` compiles, passes its harness, is constructed by nothing reachable from `main()` — last survivor of the B-9 inert-subsystem finding~~ — **resolved 2026-08-31 (R2): deleted.** No caller ever read it back (not even its own harness's `registerAnimations()`); real per-entity setup already lives in `setupAnimations()`/`Animator`. | `src/Graphics/AnimationManager.cpp` (removed) | Medium — closed |
| D6 | ~~Game aborts on machines with no audio device instead of degrading to silent~~ | ~~logged 2026-08-20+ (`agent_history.log` ~:4118); CI was given a device instead~~ | **RESOLVED (R3):** `SoundManager` now probes device availability at init and degrades to silent no-op when unavailable. |
| D7 | ~~`EventBus::ScopedSubscription` has zero adopters; `PlayingState.hpp` holds 14 raw `SubscriptionId`s with manual unsubscribes (X-7)~~ — **resolved 2026-08-31 (R4).** All raw `SubscriptionId`s outside `EventBus` migrated (`PlayingState` ×14, `AchievementManager`, `StatisticsTracker`, `Camera`); `Hud`/`ParticleSystem`'s were dead and deleted instead. Migration surfaced and fixed a real static-destruction-order bug (see completion_plan.md item 25). | `include/Core/EventBus.hpp:72-89`, `PlayingState.hpp` | Medium — closed |
| D8 | A-10 acceptance unmet: `CollisionResolver.cpp` has 9 `dynamic_cast`s (bar was <4), `PhysicsEngine.cpp` has 20 | both files | Medium, quality |
| D9 | ~~`.member_profile.json` still git-tracked — the file AGENTS.md defines as private/gitignored (X-8 residual; leaks member notes)~~ | ~~repo root~~ | **RESOLVED (R6):** `git rm --cached .member_profile.json`, added to `.gitignore`; historical copies remain in git history (history not rewritten). |
| D10 | Test suite is not hermetic: reads/writes the real `saves/`; one run destroyed `saves/highscores.json` | report §13 admits it; standing CI rule violated | Medium |
| D11 | ~~`docs/Group52_08/` has `52.md` but no `52.pdf` (every other week has both)~~ | ~~docs/Group52_08~~ | **RESOLVED (R6):** Generated `52.pdf` from `52.md` using `scripts/generate_pdf.py`. |
| D12 | GitHub issues stale: #9 fully implemented, #2 a June status note, #11 33/37 resolved | github.com/ndmhuy/SuperMarioGame | Low, hygiene |
| D13 | `main` is 9 merge-commits ahead of `dev`, breaking the fast-forward-delivery invariant the 2026-08-20 entry established | git | Low, process |
| D14 | Windows crash fix: two log entries (unconfirmed vs. MSVC-verified) never reconciled; no Windows CI | log :4210 vs :4303 | **RECONCILED (R12, 2026-08-31), NOT CLOSED — status remains unconfirmed on Windows.** `:4210` (2026-08-22 16:05, `A/fix/windows-crash-and-playtest`) states plainly: "NOT CONFIRMED ON WINDOWS. The crash was only ever observable on Windows, and both verification runs were on macOS." `:4303` (2026-08-22 21:47, same branch, ~5.5h later) claims "Rebuilt SuperMarioGame.exe cleanly with MSVC C++17 (0 errors, 0 link issues)" and "Verified By: ran the game (SuperMarioGame.exe --script...)", with `Files Modified: logs/agent_history.log` only — no source or build-config change accompanies the claim. Three things make `:4303`'s claim unsubstantiated rather than merely undocumented: (1) `.github/workflows/ci.yml` runs on `ubuntu-latest` only — there has never been Windows CI to have produced this build; (2) this and every other session in this project's history that could check has run on macOS/Linux, so no environment in this project's own toolchain can build or run a `.exe`; (3) the log itself carries a second, older instance of the same pattern — an entry at `:604` claims "100% success on MSVC" and one at `:2569` adds an MSVC-only `/FS` compile flag, both from sessions with the same macOS/Linux constraint. Absent an actual Windows/MSVC environment or a CI run to point to, `:4303`'s claim cannot be treated as verification; `:4210`'s honest "not confirmed" is this project's true status. Nothing here is fixed or retracted from the code side — the crash's root-cause fix (deferred entity spawning, `:4132` onward) is independently verified on macOS by the UB analysis in `docs/learning/mid-frame-entity-spawn-crash.html` and by two scripted runs holding 1-3 through Bowser's fireballs without a crash. What remains unconfirmed is specifically the Windows/MSVC platform claim, not the fix's logic. | Low, evidence |
| D15 | ~~A-11 friend sprawl: 12 friends each in `Entity.hpp`/`Character.hpp` — now commented as deliberate; either get user sign-off as descoped or fix~~ | ~~`Entity.hpp:156`, `Character.hpp:42`~~ | **RESOLVED (R6):** Recorded in SPEC.md §21 Descope Addendum as descoped-by-choice per user decision. |
| D16 | ~~Star power-up does not switch Mario's sprite to an invincible-state visual (no flicker/rainbow cycling on the player sprite itself)~~ | ~~player render path, star state~~ | **RESOLVED (R17):** Wired `SpriteColorFilter::getRainbowColor()` to `Player::render()` while `StarDecorator` is active. |
| D17 | ~~Piranha Plant not centered in its pipe housing, and floats above the pipe at maximum extension height instead of stopping flush~~ | ~~Piranha Plant entity/animation offsets~~ | **RESOLVED (R17):** Two root causes fixed: entity x-placement in hand-authored level JSON (`LevelLoader.cpp`), and hardcoded 64.0f emerge height vs. plant's 48.0f height (`TimerEmergenceStrategy.cpp`). |
| D18 | ~~Enemy killed by fireball flips horizontally about the **bottom edge** of its AABB, not the AABB's center line, so the flipped sprite appears to sink/shift~~ | ~~fireball-kill flip logic~~ | **RESOLVED (R17):** Flip pivot moved from bottom edge to AABB center line. |
| D19 | ~~Boom Boom arena is too small, and the level-end flag is reachable (and can be touched) while the Boom Boom fight is still in progress, letting players skip the fight~~ | ~~Boom Boom arena/level data, flag trigger gating~~ | **PARTIALLY RESOLVED (R15):** Arena too small — fixed by R15 (arenaW 7→16, arenaX 185→176, five obstructing ice tiles removed). Level-end flag reachable mid-fight — NEVER REPRODUCED; already fixed by commit eb7affe (2026-08-31 08:04), ancestor of audit base commit. |
| D20 | ~~Bowser does not spawn at all in his designated arena — boss is entirely absent from the reachable campaign, contradicts F8 playtest assumption that Bowser can be fought~~ | ~~Bowser spawn/placement, castle level data~~ | **STRUCK — NEVER OPEN:** Refuted by R15 live evidence — `level_3.json` contains `"type": "bowser"`, loading World 1-3 logs boss detection, jump-stomp scores 5000 (Bowser's value) with Bowser active. Likely already fixed by eb7affe (08:04, same morning). |
| D21 | ~~Pressing the P-Switch inside a sub-level can soft-lock the level: it turns ground tiles into coins, removing standable ground~~ | ~~P-Switch tile-swap logic, sub-level tile tables~~ | **RESOLVED (R16):** Root cause differed from plan's guess — `PlayingState::beginPSwitch` already correctly scoped to Brick↔Coin; actual bug was `MapGenerator::generateSubLevel` theming every non-Castle sub-level floor/ceiling as Brick instead of Ground. |
| D22 | ~~A half-rendered pipe exists in a sub-level near the entrance to the warp-pipe exit~~ | ~~sub-level tilemap/pipe asset placement~~ | **RESOLVED (R16):** Located and fixed tile/asset placement. |
| D23 | ~~Sub-level pipes are built from the long "L" pipe piece instead of a full pipe (single asset or a properly constructed full assembly)~~ | ~~sub-level pipe tile placement~~ | **RESOLVED (R16):** Replaced with correct full-pipe asset or properly constructed multi-tile pipe. |
| D24 | ~~Question blocks are inconsistent: a hit block does not turn solid afterward in the overworld, but does in sub-levels — pick one behavior and apply it everywhere~~ | ~~question-block hit/solidify logic (overworld vs sub-level code paths)~~ | **RESOLVED (R16):** Unified code paths; one consistent behavior now applied everywhere. |
| D25 | ~~Mario's sprite stretches unpleasantly during the mushroom transform animation; needs a design proposal to change the sprite/AABB and hitbox so the transform doesn't distort the sprite~~ | ~~Mario transform sprite + AABB/hitbox sizing~~ | **STRUCK — NEVER OPEN:** No transform animation exists in the codebase. Every power-up is instant state swap (`Player::powerUp` → `changeState/setBaseState + applyStateSize + refreshStateAnimations`, same frame). `Entity::drawSprite` applies uniform scale to both axes (no non-uniform stretch possible). Small/Super/Fire/Cape all report width 24, so no Mushroom→Super aspect mismatch. Only place this class of bug existed was `MegaDecorator::getSize()` (commit c75d9a8), an ancestor of dev. |
| D26 | ~~End-of-level flow plays two music cues (touch-flag jingle, then end-level-menu track), but the second cuts off/replaces the first before it finishes~~ | ~~end-level music sequencing (SoundManager / end-state transition)~~ | **RESOLVED (R19):** Plan's described cues were incorrect. Reproducible case: `castle_complete` (fired by BossDefeated) cut off by `level_complete` (fired by LevelComplete) on boss levels. Fixed with deferred swap that only defers when both outgoing and incoming tracks are one-shot. |
| D27 | ~~Death SFX does not finish playing before the player respawns — cut off by the respawn transition~~ | ~~death SFX / respawn timing~~ | **NOT DEMONSTRABLE AS WORDED:** `lost_life.wav` is 3.267s (longer than 1.6s max fall+respawn window). No code stops it; pool only reuses Stopped channels. SFX plays on over resumed BGM. Observation real, stated mechanism absent — needs someone with audio to characterise what was actually heard. NOT fixed, NOT invalid. |
| D28 | ~~Colorblind setting toggle has no visible effect on rendering~~ | ~~colorblind option wiring (OptionsState / render filter)~~ | **STRUCK — NEVER OPEN:** Already fixed by commit 2e7f5bb (2026-08-19, 12 days before audit). `ColorPalette::get()` reads `Game::getColorblindMode()` and is wired into Minimap, PlayingState HUD/debug colours, and OptionsState. |
| D29 | Dying near an active boss and respawning can silently delete the boss entity itself, ending the fight without defeating it — found live in R12's Boom Boom attempt (2 of 3 hits landed, then a death+respawn removed him: stdout printed `Boss arena released` / `Cleared 1 enemy(ies) from the respawn point`, and he never appeared in any screenshot afterward) | `PlayingState.cpp:2701-2718` (the respawn-safety "clear anything within 2.5 tiles of the checkpoint" loop, added to fix a real spawn-killing bug) only checks `entity->getCategory() == EntityCategory::Enemy` before calling `destroy()` — bosses (`Boss`/`BoomBoom`/`Bowser`) are Enemies too, and the loop has no exemption for one that is `m_activeBoss`. If the respawn point lands within 2.5 tiles of the boss (very plausible inside a boss arena, which is often smaller than 5 tiles), the boss is deleted outright rather than merely being kept clear of the player for a moment. | Medium — a boss fight can be trivially and silently ended (not won) by one convenient death, and nothing in the log or HUD calls this out as different from a normal defeat |
| D30 | `LOAD GAME` always resumes into World 1-1 regardless of which level a save slot recorded — found live in R12 while investigating a route to inject Fire Mario into World 1-3 for the Bowser fireball-stagger check | `MenuState.cpp` (Load page confirm) constructs `PlayingState(false, false, MapGeneratorConfig(), 0, 0, MatchConfig{}, false, pendingLoadSlot)` — levelIndex is hardcoded `0`; the constructor's own comment says this "fresh Level-1 PlayingState exists only to give `loadFromSlot` somewhere to run." `Serializer::saveGame`/`loadGame` read and write `level.id` faithfully, but `PlayingState::loadFromSlot` (`PlayingState.cpp:2100-2117`) only calls `adoptPlayer()` — it restores the player's position/state/coins/lives but never reads `lvlId` to pick which level file backs the running `PlayingState`. A slot saved deep in 1-2 or 1-3 loads the player at that saved (x,y) inside **1-1's own tilemap**, which is not the level that position was ever validated against. | Medium — save/load already shipped (R8, F3) and is reachable from `main()`, but is silently wrong for any slot not saved in World 1-1; a player who saves mid-1-3 and reloads will not be back in 1-3 |

## 5. Remaining feature work (from the checkbox triage)

| ID | Feature gap | Size |
| :-- | :--- | :-- |
| F1 | **Campaign population & balance**: 6 implemented enemy types (Paratroopa, Boo, Bullet Bill, Thwomp, Chain Chomp, Lakitu) in zero shipped levels; no hidden block placed (secret_finder unreachable); per-level enemy counts never balanced against difficulty modifiers. User's 2026-08-31 playtest confirms too few enemies to exercise `verify_enemies_behavior`'s scenarios live — that harness names the specific behaviors (per-enemy AI, stomp/fireball reactions, despawn) that currently have no in-level target to test against; population work must place enough of each type for that harness's cases to be observed in a real run, not just constructed by the harness itself | Large, highest player-visible value |
| F2 | **Dead-wiring batch**: `StarKillSpin`/`PlayerDeathHop` death effects never spawned; `SpriteColorFilter::getRainbowColor()` never called (star power shows no rainbow); `WaterBubble`/`LavaEmber`/`Combo`/`WallDust` particles never emitted; 3 footstep WAVs never played; no combo SFX escalation; no per-row menu cue | Medium, many small wins |
| F3 | **Load Game UI**: `loadFromSlot` reachable only from the dev panel; `Serializer::getSlotPreview` unused; no main-menu row | Medium |
| F4 | **Camera/view clamp**: player can walk out of the left view edge (only boss arenas clamp) | Small |
| F5 | **Attract mode**: 30 s idle → replay playback (SPEC §10.2); `ReplayRecorder` playback already exists | Small-Medium |
| F6 | **Debug console autocomplete** (planned in completion_plan; parser exists) | Small |
| F7 | **Dynamic music layers** (SPEC §11.5) — single `sf::Music` today | Large — recommend descope |
| F8 | **Full playtest & verification pass**: nobody has fought Bowser on screen, pressed P-Switch/POW/axe in a live run, died in a 2P match, or recorded a 60 fps measurement; Bowser stagger numbers untuned | Medium, evidence work |
| F9 | **SPEC descope addendum**: formally mark every §3 item that will not be built | Small, docs |

## 6. The plan — prompts, models, effort

Rules that bind every task below: branch off `dev` with the given name; **no
auto-merge, no push** (user's call); read `AGENTS.md` first; append the full
log entry (`Git Fingerprint`, `Fetched Remotes`, `Reachable From Main`,
`Verified By`); "complete" = reachable from `main()` **and observed running**
(`ctest` + a scripted or manual run). Model tiers: Haiku 4.5 for mechanical
work, Sonnet 5 for scoped implementation, Opus 5 for design-sensitive refactors
and level design; effort = the reasoning-effort setting to run it at.

### R1 — Small-defect batch (D1 + D2 + D3 + D4) — **Sonnet 5, effort: medium** — branch `A/fix/audit-31-08-defect-batch` — **COMPLETE / merged to dev**

> Read AGENTS.md, then fix four audited defects in SuperMarioGame/ (all
> file:line refs verified 2026-08-31, main @ ab16759):
> 1. `src/Core/SoundManager.cpp:213-218` `playLevelBGM(int levelIndex)`: index
>    2 maps to "underwater" but `LevelCatalog` index 2 is Bowser's Castle
>    (`src/Utils/LevelCatalog.cpp:11-15`; callers pass the catalog index —
>    `PlayingState.cpp:111,2620,2675,2741`). Map 0→overworld, 1→underworld,
>    2→castle, 3→sub_space-bonus_room. The "castle" key is already registered
>    (`SoundManager.cpp:173`). Decide and document what a generated/endless
>    level (no catalog index) should play.
> 2. `src/Entities/Spiny.cpp:35`: a flipped Spiny despawns when world
>    `position.y > Constants::WINDOW_HEIGHT + 100` — a screen constant, correct
>    only while maps are 720 px tall. Despawn against the tilemap's pixel
>    height (see how `PlayingState`'s void-kill plane at ~:773-782 derives it)
>    and check no other entity repeats the pattern (grep WINDOW_HEIGHT under
>    src/Entities/).
> 3. `MapGenerator::generateSolvable` (`src/Utils/MapGenerator.cpp:388-416`)
>    returns false when all reseeds fail, but every call site
>    (`PlayingState.cpp:137,1791,1849`) discards the result. Surface it:
>    propagate the flag and show a small HUD/dev-panel note ("layout
>    unverified") or at minimum bump attempts and log at warn level from the
>    call site. Do not make it a hard failure — the fallback behaviour is
>    deliberate and documented in `MapGenerator.hpp:48-49`.
> 4. `tests/verify_sound_visual.cpp` is registered in no target in
>    SuperMarioGame/CMakeLists.txt. Read it; if still meaningful, register via
>    `add_verify_test(... NO_CTEST)` like its `*_visual` siblings; if
>    superseded, delete it and say so in the log.
> Build, run `ctest` (expect 13/13), and observe fix 1 live: launch the game
> with a script under tests/scripts/ that enters World 1-3 and confirm the
> castle track plays (log `Verified By: ran the game`). Commit per fix with
> conventional messages. Do not merge or push.

### R2 — AnimationManager: wire it or delete it (D5) — **Sonnet 5, effort: medium** — branch `A/fix/animation-manager-disposition` — **COMPLETE / merged to dev**

> Read AGENTS.md. `src/Graphics/AnimationManager.{cpp,hpp}` compiles and passes
> `tests/verify_all_entities_visual.cpp` but is constructed by nothing
> reachable from `main()` — the last inert subsystem from the 2026-08-18 audit
> (finding B-9). First inventory what it offers versus what entities already do
> via `Animator`/`SpriteSheet` (`src/Graphics/`, entity `setupAnimations`
> implementations). Then either (a) adopt it at the call sites where it
> genuinely removes duplication, or (b) delete the class, its harness
> references and its CMake entry, recording the rationale. Do NOT leave it
> half-adopted. Whichever way: build, ctest 13/13, run the game and watch one
> animated entity of each family (player, enemy, item) render correctly, and
> write `Reachable From Main` honestly. Commit; no merge, no push.

### R3 — Audio-device-less startup (D6) — **Sonnet 5, effort: medium** — branch `A/fix/no-audio-device-degrade` — **COMPLETE / merged to dev**

> Read AGENTS.md. On a machine with no audio device the game aborts at startup
> (logged 2026-08-20 in logs/agent_history.log, ~line 4118 — CI was given a
> dummy device as a workaround). Make `SoundManager`
> (`src/Core/SoundManager.cpp`) degrade to a silent no-op: probe device
> availability once at init, guard `sf::Sound`/`sf::Music` construction and
> `play*()` behind it, log one warning. Do not scatter null-checks — one
> guarded seam. Verify: normal build still plays menu music (run the game);
> then reproduce the no-device path (e.g. run with a null audio driver env or
> temporarily stub the probe) and show clean startup. ctest 13/13. Consider
> removing the CI dummy-device workaround in `.github/workflows/ci.yml` if the
> fix makes it unnecessary — separate commit. No merge, no push.

### R4 — Adopt ScopedSubscription (D7 / X-7) — **Sonnet 5, effort: medium** — branch `A/fix/scoped-subscription-adoption` — **COMPLETE / merged to dev**

> Read AGENTS.md. `EventBus::ScopedSubscription`
> (`include/Core/EventBus.hpp:72-89`) is an RAII unsubscribe token with zero
> users. `include/Core/PlayingState.hpp` holds 14 raw `SubscriptionId` members
> (~lines 111-116, 272-275, 346, 438-440) unsubscribed by hand — the header's
> own comment describes the hazard. Migrate every raw SubscriptionId in the
> codebase (PlayingState first; grep for `SubscriptionId` elsewhere —
> SoundManager, HUD, Camera, AchievementManager, StatisticsTracker) to
> ScopedSubscription, delete the manual unsubscribe blocks, and make the
> destructor ordering safe (subscriptions must die before the bus). Acceptance:
> zero raw `SubscriptionId` members outside EventBus itself; build; ctest
> 13/13; run the game through menu → play → pause → quit-to-menu → play again
> (the resubscribe path is where double-subscribe bugs live) — use or extend a
> tests/scripts/ script. Then correct docs/issues/completion_plan.md item 25
> status. No merge, no push.

### R5 — dynamic_cast reduction (D8 / A-10) — **Opus 5, effort: high** — branch `A/refactor/collision-dispatch` — **OPEN**

> Read AGENTS.md and SPEC.md §2.2. Acceptance bar from
> docs/issues/member_a_fix_plan.md WP12: fewer than 4 dynamic_casts in
> `src/Physics/CollisionResolver.cpp` (today: 9 — lines ~214, 277, 353, 367,
> 397, 482, 487, 524) and reduce `src/Physics/PhysicsEngine.cpp` (today: 20).
> This is a design task, not a mechanical one: prefer double-dispatch or
> virtual hooks on `Entity` (`onStomped`, `onHitFromBelow`, `isCollidable`,
> `getGravityMultiplier` are the established pattern — extend it) over a
> visitor megastructure; respect the Open/Closed rule in AGENTS.md (no new
> switch-on-type). Where a cast is genuinely the honest answer (e.g. the
> player-vs-player pair in multiplayer), keep it and comment why. Do NOT
> regress behaviour: the full `verify_regressions` suite (504 checks) plus
> `verify_enemies`, `verify_blocks` must stay green; then play World 1-1 and a
> versus match via tests/scripts/. Commit in reviewable steps (one dispatch
> family per commit). No merge, no push.

### R6 — Repo hygiene + SPEC descope addendum (D9, D11, D15, F9) — **Haiku 4.5, effort: low** — branch `A/docs/hygiene-and-descope-addendum` — **COMPLETE / merged to dev**

> Read AGENTS.md. Four bounded chores:
> 1. `.member_profile.json` is git-tracked but AGENTS.md defines it as private
>    and gitignored. `git rm --cached .member_profile.json`, add to
>    `.gitignore`, and note in the log that historical copies remain in git
>    history (do NOT rewrite history).
> 2. Generate the missing `docs/Group52_08/52.pdf` from `52.md` with
>    `scripts/generate_pdf.py`, matching how other weeks' PDFs were produced.
> 3. Add a dated "Descope addendum (2026-08-31)" section at the END of SPEC.md
>    (do not alter frozen sections) listing the items from
>    docs/issues/spec_feature_audit_2026-08-31.md §3 that the team will not
>    build, each with one line of rationale; mark any the user still wants as
>    "planned" instead — ask the user which, don't guess.
> 4. Entity.hpp/Character.hpp friend-sprawl (audit A-11): present the tradeoff
>    to the user (12 friends each, now commented as deliberate) and record
>    their decision in the addendum — descoped-by-choice or queued.
> No code changes. Commit each chore separately. No merge, no push.

### R7 — Dead-wiring batch (F2 + F4 + F6) — **Sonnet 5, effort: medium** — branch `A/feature/wire-dormant-vfx-sfx` — **OPEN** (item 2 DONE by R17)

> Read AGENTS.md and SPEC.md §11/§15/§17.3. Wire the implemented-but-dormant
> pieces. **Caution**: the "verified dormant 2026-08-31" inventory was compiled by
> the same audit pass that mis-recorded D20/D25/D27/D28, so each remaining item
> should be re-verified against dev before implementation. Item 2 has been
> independently confirmed (getRainbowColor genuinely has no non-test caller), so
> this is a caution, not a dismissal. Remaining items: 1, 3, 4, 5, 6, 7.
> 1. Spawn `DeathEffectType::StarKillSpin` on star-power kills and
>    `PlayerDeathHop` on player death (`include/Graphics/EntityDeathEffect.hpp:10-11`
>    has the types; only `EnemyFlip` is ever spawned — find the kill sites in
>    `CollisionResolver`/`PlayingState`).
> ~~2. Call `SpriteColorFilter::getRainbowColor()`
>    (`src/Graphics/SpriteColorFilter.cpp:37`) from `Player::render`
>    (`Player.cpp:626-636`) while a StarDecorator is active.~~ — **DONE (R17): wired in D16 fix.**
> 3. Emit the four declared-but-never-emitted particle types
>    (`ParticleEmitter.hpp:5-14`): WaterBubble in water zones, LavaEmber over
>    lava, Combo on combo milestones, WallDust while wall-sliding.
> 4. Play the three loaded footstep WAVs (`SoundManager.cpp:51`) on a
>    surface-dependent walk cadence (tile surface type is already queryable —
>    see the ice/conveyor handling in PhysicsEngine stages 1/1.5).
> 5. Combo SFX escalation: subscribe SoundManager to `EventType::ComboHit`
>    (published `Player.cpp:691`) with rising pitch or per-tier samples.
> 6. Per-row menu navigation cue in MenuState/OptionsState (CharSelect and
>    Pause already have one — reuse that cue).
> 7. Clamp both players inside the camera view in PlayingState (only boss
>    arenas clamp today); keep the void-kill plane behaviour intact.
> Each item: smallest change that uses the existing subsystem; no new
> managers. Build, ctest 13/13, and observe each effect in a live run (star
> kill, death, water/lava level via editor, wall slide, combo, menu, view
> edge). Log honestly per item. Commit per item. No merge, no push.

### R8 — Load Game UI (F3) — **Sonnet 5, effort: medium** — branch `A/feature/load-game-menu` — **OPEN**

> Read AGENTS.md, SPEC.md §10.2/§12.3. Make saved games loadable without the
> dev panel: add a LOAD GAME row to `MenuState` (rows at
> `src/Core/MenuState.cpp:26-27,95-110`) opening a 3-slot picker that renders
> `Serializer::getSlotPreview` (exists, unused — `include/Utils/Serializer.hpp`)
> — character, level, score, star coins, play time per slot, empty slots
> labelled. Confirm loads via the same path the dev panel uses
> (`PlayingState::loadFromSlot`, only caller today `DevPanel.cpp:576`; the
> adoptPlayer use-after-free fix lives on that path — reuse it, don't fork
> it). Keyboard-only navigation consistent with the other menus; Escape backs
> out. Verify: save in slot 2 from the pause menu, quit to menu, LOAD GAME →
> slot 2, confirm state restored (lives/coins/level) in a live run; ctest
> 13/13. Update features_list.md #75-78 wording if slot preview ships. No
> merge, no push.

### R9 — Campaign population & balance (F1) — **Opus 5, effort: high** — branch `A/feature/campaign-population-pass` — **OPEN**

> Read AGENTS.md, SPEC.md §6/§9, and docs/issues/spec_feature_audit_2026-08-31.md
> §5-F1. Six implemented enemies (KoopaParatroopa, Boo, BulletBill, Thwomp,
> ChainChomp, Lakitu) appear in zero of the 7 files under
> SuperMarioGame/assets/levels/, and no shipped level places a hidden_block —
> which also makes the `secret_finder` achievement unreachable. This is level
> design: place each type where its AI reads well (Boo in the dark 1-2 cavern;
> Thwomp/BulletBill/ChainChomp in the 1-3 fortress; Paratroopa in 1-1 skies;
> Lakitu over an open 1-1 stretch — it spawns Spinies, so cap its patrol), add
> at least one hidden block per main level, and rebalance per-level enemy
> counts against the Easy/Normal/Hard modifiers (TASKS.md line ~582). Respect
> the JSON schema used by the existing files and the editor (entity rows;
> LevelLoader parses them — do not invent keys). Every edited level MUST pass
> `LevelSolvability` (extend the ctest in tests/verify_regressions.cpp to
> assert the 7 shipped levels stay solvable — new permanent regression case),
> and MUST be played through live via a tests/scripts/ script or by hand,
> including one full run on Hard. Also confirm the secret_finder achievement
> actually fires on the new hidden block. Log `Verified By: ran the game` with
> what was observed. Commit per level. No merge, no push.

### R10 — Attract mode (F5) — **Sonnet 5, effort: medium** — branch `A/feature/attract-mode` — **OPEN**

> Read AGENTS.md, SPEC.md §10.2. After 30 s idle on the main menu, play a
> recorded demo (ReplayRecorder playback already exists —
> `src/Core/ReplayRecorder.cpp`, used by the debug console's `replay` command)
> over the menu or in a demo PlayingState, dismissed instantly by any key back
> to the menu. Ship one bundled demo recording under assets/ (record a ~30 s
> World 1-1 run; the recorder serialises to JSON). Guard: never during an
> active game, never in Endless/versus. TASKS.md line ~468 notes it needs the
> replay system — that dependency is met. Verify live: wait 30 s, watch the
> demo, press a key, get the menu back; ctest 13/13. No merge, no push.

### R11 — Hermetic test suite (D10) — **Sonnet 5, effort: high** — branch `A/fix/hermetic-tests` — **OPEN**

> Read AGENTS.md (g-rule-13: CI must be hermetic) and report §13's admission:
> the suite reads/writes the real `saves/`, one run destroyed
> saves/highscores.json (unrecoverable — logged 2026-08-31 ~line 4277), and a
> high-score assertion is state-dependent. Introduce a save-directory seam
> (env var or `Serializer` base-path setter), point every test at a per-test
> temp dir, and assert no test touches the working tree's saves/ (add a CTest
> guard case that fails if it does — the parity-test discipline). While there:
> the 14 test targets each recompile ~133 sources (~10 min CI); add a CMake
> OBJECT library so sources compile once (log :4122 documents the cost).
> Acceptance: `ctest` green from a clean clone in an empty scratch dir with a
> pre-seeded saves/ fixture proving the real one is untouched; CI time drop
> recorded in the log. Then delete the report's "not hermetic" bullet in
> reports/report_content.py §13 and rebuild the report per its build.sh. No
> merge, no push.

### R12 — Full playtest & evidence pass (F8, D14 closure) — **Sonnet 5, effort: high** — branch `A/verify/full-playtest-pass` — **PARTIAL, evidence captured**

> Read AGENTS.md rules 9-10 and docs/verification/README.md (its stated limits
> are your checklist). Nobody has: fought Bowser on screen, pressed a P-Switch
> or POW or the bridge axe in a live run, died in a 2-player match, tested
> difficulty modes or key rebinding in-game, or recorded a 60 fps measurement.
> Using the --script harness (SuperMarioGame --script tests/scripts/<file>,
> back up saves/ first — a previous session destroyed highscores.json), write
> scripts + capture screenshots into docs/verification/ for: (a) full campaign
> walkthrough 1-1 → 1-3 including the Boom Boom and Bowser fights to
> completion, noting whether the 4-fireball/3 s stagger feels tunable; (b)
> P-Switch, POW, axe, 100-coin 1-UP; (c) a 2P versus round ending in a death;
> (d) Easy vs Hard same level; (e) rebind a key in Options and prove the new
> binding drives play (scripts name BOUND keys — check saves/config.json
> first, per the README warning); (f) an FPS trace over 60 s of 1-1 (the
> Game.cpp:176 readout) recorded in the log. Update docs/verification/README.md
> and tick TASKS.md 676-680 ONLY for what was actually observed. Reconcile the
> two Windows-crash log entries (:4210 vs :4303) with one dated note stating
> the MSVC verification status. Fix nothing beyond one-line tuning constants;
> file defects found as new issue entries in docs/issues/. No merge, no push.

**Result, 2026-08-31 (see docs/verification/README.md's R12 section for full
detail with screenshots):** (b), (c), (d), (e), (f) OBSERVED live; (a) PARTIAL
— Bowser cleared via the axe's alternate-victory mechanic (`defeatNow()`,
not the fireball/stagger loop, which was attempted once and failed before
Fire Mario was obtained), Boom Boom 2 of 3 hits landed then the boss was
deleted by a respawn-safety bug (new D29); 100-coin 1-UP not attempted. No
full blind 1-1→1-3 walkthrough was attempted — each boss arena was reached
directly via a temporary `spawnPoint`/`progress.json`, disclosed per item.
D14 reconciled (see its row above): status remains unconfirmed on Windows.
Two new defects filed: D29 (boss deletable via respawn-clear radius), D30
(LOAD GAME ignores the saved level index, always resumes World 1-1). No
source file was changed beyond test scripts and level `spawnPoint` values
(all reverted, `git diff` clean on tracked level JSON).

### R13 — GitHub issue hygiene (D12) — **Haiku 4.5, effort: low** — needs user approval to post

> With the user's explicit go-ahead (posting is outward-facing): close #9
> (implemented as InputManager::applyBindings — cite
> docs/issues/member_b_input_sync.md rows and commit refs), close #2 (June
> status note, overtaken), and comment on #11 re-scoping it to the four
> survivors (A-10 partial, A-11 descoped-pending-signoff, X-7 → R4, X-8
> residual → R6), linking docs/issues/spec_feature_audit_2026-08-31.md. Draft
> the three comments for user review BEFORE posting anything.

### R14 — dev catch-up (D13) — **user decision, 5 minutes**

`git checkout dev && git merge main` (fast-forward-able content, merge commit
only) restores the every-delivery-is-a-fast-forward invariant. A one-command
user action after the next batch of task branches lands; noted here so it is
not forgotten. Alternatively adopt "merge to main only via dev" strictly and
let the wrinkle age out — but pick one and record it.

### R15 — Boss encounters: Bowser spawn + Boom Boom arena/flag gating (D19, D20) — **Sonnet 5, effort: high** — branch `A/fix/boss-encounter-batch` — **COMPLETE / merged to dev**

> Read AGENTS.md. Two boss-level defects reported from live play 2026-08-31:
> 1. Bowser never spawns in his designated castle arena — trace the spawn
>    trigger/entity-placement path for the Bowser fight (level data +
>    whichever system instantiates bosses on trigger) and find why the
>    placement is inert; this blocks F8's playtest assumption that Bowser can
>    be fought at all.
> 2. The Boom Boom arena is too small for the fight to read well, and the
>    level-end flag is reachable (and touchable) while the fight is still in
>    progress, letting players skip the boss. Enlarge the arena per level
>    data/tilemap, and gate the flag trigger so it's inert (or physically
>    blocked) until Boom Boom is defeated.
> Acceptance: build, ctest green, then play both fights live via a
> tests/scripts/ script or by hand — confirm Bowser spawns and can be fought
> to a resolution, and confirm the flag cannot be reached mid-Boom-Boom-fight.
> Log `Verified By: ran the game`. Commit per fix. No merge, no push.

### R16 — Sub-level tile/pipe/block batch (D21, D22, D23, D24) — **Sonnet 5, effort: medium** — branch `A/fix/sublevel-tile-batch` — **COMPLETE / merged to dev**

> Read AGENTS.md. Four sub-level tile defects from live play 2026-08-31:
> 1. **P-Switch soft-lock (highest priority — game-breaking)**: pressing the
>    P-Switch in a sub-level turns ground tiles into coins, removing standable
>    ground and stranding the player. Find the P-Switch tile-swap logic and
>    scope it to only the tile types the mechanic is meant to affect (blocks
>    ↔ coins), never load-bearing ground.
> 2. A half-rendered pipe exists in a sub-level near the warp-pipe exit —
>    locate and fix the tile/asset placement.
> 3. Sub-level pipes use the long "L" pipe piece instead of a full pipe;
>    replace with the correct full-pipe asset or properly constructed
>    multi-tile pipe, consistent with overworld pipes.
> 4. Question blocks turn solid after being hit in sub-levels but not in the
>    overworld — find the two code paths (likely diverged overworld vs.
>    sub-level tile logic) and unify on one behavior; ask the user which
>    behavior to keep if not obvious from SPEC.
> Acceptance: build, ctest green; play through the affected sub-level(s) live,
> confirming the P-Switch no longer soft-locks, the pipe renders fully, and
> question-block solidify behavior is consistent. Commit per item. No merge,
> no push.

### R17 — Player/enemy visual-state batch (D16, D17, D18) — **Sonnet 5, effort: medium** — branch `A/fix/player-enemy-visual-batch` — **COMPLETE / merged to dev**

> Read AGENTS.md. Three visual-state defects from live play 2026-08-31:
> 1. Star power-up doesn't switch Mario's sprite to an invincible-state visual
>    — find where star state is tracked (likely the same StarDecorator used
>    by R7 item 2's rainbow-filter work) and wire the sprite/animation swap;
>    coordinate with R7 if that task hasn't landed yet, since both touch
>    star-state rendering.
> 2. Piranha Plant isn't centered in its pipe and floats above the pipe at
>    maximum extension — fix its rest position and travel offset.
> 3. An enemy killed by fireball flips about the bottom edge of its AABB
>    instead of the AABB's center line — fix the flip pivot in the
>    fireball-kill code path.
> Acceptance: build, ctest green; observe each fix live (star pickup, a
> Piranha Plant pipe, a fireball kill). Commit per item. No merge, no push.

### R18 — Mario transform sprite/hitbox redesign (D25) — **Opus 5, effort: high** — branch `A/design/mario-transform-sprite` — **CANCELLED**

> D25 is struck as never-open (see §4 defect row). No work required. Rationale: no transform animation exists in the codebase. Power-ups are instant state swaps; the only stretch bug existed in `MegaDecorator::getSize()` (commit c75d9a8), an ancestor of dev.

### R19 — End-game audio batch (D26, D27, D28) — **Sonnet 5, effort: medium** — branch `A/fix/endgame-audio-batch` — **COMPLETE / merged to dev**

> Read AGENTS.md. Three audio defects from live play 2026-08-31:
> 1. End-of-level flow plays the touch-flag jingle then the end-level-menu
>    track, but the second cuts off the first before it finishes — sequence
>    them (wait for the first to finish, or crossfade deliberately) instead of
>    the second replacing the first mid-playback.
> 2. Death SFX is cut off by the respawn transition — either delay respawn
>    until the SFX finishes or let it play out over the respawn (don't extend
>    invulnerability/lock input to stall it, just don't stop the sound).
> 3. The colorblind option toggle has no visible effect — trace whether it's
>    wired to any render filter at all; if it was never implemented, treat as
>    a real accessibility gap, not cosmetic, and implement an actual filter
>    (e.g. a documented colorblind-friendly palette swap) rather than closing
>    it as a no-op.
> Acceptance: build, ctest green; observe each fix live (finish a level,
> die and respawn, toggle colorblind mode and confirm a visible change).
> Commit per item. No merge, no push.

### Explicitly descoped (record, don't build)

F7 dynamic music layers (large, low grading value — one `sf::Music` swap
architecture is honest and documented); SPEC §19.4 lighting shaders, §9.5
autoscroll, §9.4 timed bonus rooms, climbing, swimming-as-a-state, skid,
hover-pause/stun, knockback input-lock, red variants, cape swoop, mini
abilities, character-switch hotkey, floating score text, extra pools —
candidates for R6's addendum unless the user pulls any into scope. A\*
pathfinding and split-screen speedrun stay descoped per TASKS.md's own text.

### Suggested order

R1 → R16 (P-Switch soft-lock is game-breaking, fix before anything else
touches sub-levels) → R15 (Bowser must spawn before R9/R12 can rely on him) →
R6 (cheap, unblock honesty) → R17 → R19 → R7 → R9 (the visible win, now that
enemies have a stable target level set and bosses work) → R18 (design-review
gated, can run in parallel once proposed) → R8 → R11 → R12 (evidence last,
after the gameplay changes settle) → R4 → R2 → R3 → R5 → R10 → R13/R14
whenever the user is at the keyboard. After each merged batch: regenerate the
report (`Report/SuperMarioGame/build.sh`), features_list.pdf, and re-run this
audit's §2 checklist against the new claims.

---

## 7. Playtest sweep, 2026-08-31 — merge note

The user's 2026-08-31 hands-on playtest surfaced 13 issues; all have been
folded into this document as first-class findings rather than kept as a
separate untriaged list:

- **Defects** D16-D28 (§4): star sprite not switching to invincible state,
  Piranha Plant pipe centering/height, fireball-kill flip pivot, Boom Boom
  arena/flag gating, Bowser not spawning, P-Switch sub-level soft-lock,
  half-rendered sub-level pipe, long-L sub-level pipe, question-block
  overworld/sub-level inconsistency, mushroom-transform sprite stretch,
  end-game double-music cutoff, death SFX cutoff, dead colorblind toggle.
- **Feature note** F1 (§5) updated: the "not enough enemies to test" report
  is folded into the existing campaign-population gap, with the
  `verify_enemies_behavior` harness named as the acceptance reference.
- **New phases** R15-R19 (§6) plan the fixes: R15 boss encounters (Bowser
  spawn + Boom Boom gating), R16 sub-level tiles (P-Switch soft-lock first —
  game-breaking), R17 player/enemy visual state, R18 the design-sensitive
  Mario-transform sprite proposal, R19 end-game audio. The suggested order
  in §6 places R16 and R15 ahead of the earlier-planned batches since a
  soft-lock and a missing final boss block downstream verification work
  (R9, R12).

---

*Method note: findings compiled from three parallel read-only audit passes over
`main` @ `ab16759` on 2026-08-31; every file:line above was re-verified against
the working tree before this document was written. The checkbox triage tables
(126 items, classification and evidence per line) are recorded in the session
log entry for this date in `logs/agent_history.log`.*
