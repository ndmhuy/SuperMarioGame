# PLAN: Completing the game

## Status

* **Date**: 2026-08-19
* **Baseline**: `dev` @ `80ea8e8` — builds warning-clean, 76/76 regression checks, application runs
* **Supersedes**: [member_a_fix_plan.md](member_a_fix_plan.md) (all 12 packages complete)
* **Audit**: [code_audit_2026-08-18.md](code_audit_2026-08-18.md) — 38 findings, 37 resolved, 1 retracted

---

## 1. Audit closed

Every gameplay and correctness finding from the August audit is resolved.

| Group | Result |
| :--- | :--- |
| Member A, A-1 … A-14 | 14 / 14 closed |
| Member B, B-2 … B-15 | 14 / 14 closed (B-1 retracted — audio was never broken) |
| Camera C-1 … C-4 | closed |
| Gameplay G-1 … G-4 | closed |
| Cross-cutting X-3, X-4, X-6, X-9 | closed |

**Six cross-cutting items remain open deliberately** — they are hygiene and scope, not defects:

| ID | Item | Why it is still open |
| :-- | :--- | :--- |
| X-1 | 47 checked boxes in `TASK_DIVISION.md` never re-verified | Superseded by §2 of this document |
| X-2 | 3 of 10 game states exist | The bulk of the remaining feature work |
| X-5 | `MenuState::render` still calls `changeState` (5 sites) | Same `DevPanel` treatment as `PlayingState`; deferred until 7.1 rewrites that screen anyway |
| X-7 | `EventBus::publish` deep-copies subscribers; `Camera` move ctor with `this`-capturing lambdas | Performance and latent-risk, no observed failure |
| X-8 | 207 tracked build artefacts (205 clangd index files, 2 `.DS_Store`) | One `git rm -r --cached`, but it rewrites a lot of index state — do it on a quiet branch |
| A-13 | 4 files still guess asset paths | `ResourceManager::resolvePath` exists; adopting it everywhere is mechanical |

Two **asset** gaps block nothing in code but are visible in play:

* `flag_white` / `flag_reddish_white` are in no atlas — the flagpole has no flag.
* The player atlas holds only `_small` and `_tiny` per character. **No Super, Fire or Cape art exists.** Those states became reachable when B-2 was fixed, so they now render as small Mario.

---

## 2. Re-baselined ledger (closes X-1)

Several ledger tasks are now complete as a side effect of the audit work. Verified against the code, not the checkbox:

| Task | Ledger | Actual |
| :--- | :--- | :--- |
| **11.2** Edge cases | `[ ]` | **Complete.** All six sub-items done: checkpoint respawn, flashing invincibility, shell chains, time-out death, 100-coin 1-UP, pipe transitions. |
| **7.4** Playing State (full) | `[ ]` | **Complete.** Level load, spawn, physics, camera, HUD, minimap, collision callbacks, P-Switch, star coins, dev panel. |
| **5.7** Transitions & shake | `[ ]` | **Complete.** Fade in on entry, fade out on game over, shake presets bound to events and clamped. |
| **9.1** AI tuning | `[ ]` | **Mostly.** Lakitu spawner, ChainChomp tether, Koopa shell states done. Outstanding: Thwomp state machine, ImGui AI overlay. |
| **6.1–6.3** Audio | `[x]` | **Correct** — the audit's claim otherwise was retracted. |
| **5.2/5.4/5.6/5.8/5.9** Visuals | `[x]` | **Now genuinely true.** Were built-but-unwired; wired in B-9. |
| **1.5** Key rebinding | `[x]` | **Now genuinely true.** Was persisted-but-never-applied; fixed in B-11. |
| **2.5** Coyote/jump buffer | `[x]` | **Now genuinely true.** Were inert counters; fixed in A-4. |
| **3.13** HiddenBlock | `[x]` | **Now genuinely true.** Was unreachable by construction; fixed in B-3. |
| **4.3** Camera | `[x]` | **Partial.** Clamping and shake correct; still no lookahead or scroll modes. |

---

## 3. What is left

Seventeen ledger tasks. Grouped by what they buy.

### Tier 1 — Required for a complete game loop (~14 h) — **DONE** (`A/tier1-game-loop`)

All seven items are implemented and both ctest suites pass. What is *not* done
inside them, so the ledger stays honest:

* 7.1 has no attract mode (it needs 10.3 Replay) and no Load Game entry.
* 7.5 has no World Map entry, because 7.3 does not exist yet.
* Nobody has watched pause / victory / game over on screen. The environment this
  was built in cannot inject keystrokes or capture the screen, so the evidence is
  the two ctest suites plus a launch that reaches the new menu. **Play it before
  tagging the milestone.**

One bug fell out of the work: the game aborted on exit with SIGABRT because
`Game::shutdown` never released the `ResourceManager`'s textures and fonts. It
printed its whole clean-shutdown log and then died, which is why 208 sessions
never noticed. Fixed in `91ab4d4`.

Without these the game runs but has no shell around it.

| # | Task | Owner | Est. | Note |
| :-- | :--- | :--- | ---: | :--- |
| 1 | **7.5** Pause State | B | 2 h | Needs `GameStateManager::render` to draw the whole stack, not just the top — currently impossible to overlay. Do this first; 7.6/7.7/7.9/7.10 all depend on the same change. |
| 2 | **7.6** Game Over State | B | 1.5 h | Replaces the current fade-to-menu. |
| 3 | **7.7** Victory State | B | 2 h | `advanceToNextLevel()` already exists; this gives it a screen with score and star-coin summary. |
| 4 | **7.2** Character Select | B | 2 h | Mario/Luigi live; Toad/Peach gated on achievements that already unlock. |
| 5 | **7.1** Animated Menu | A | 2.5 h | Also removes X-5 by rebuilding the screen off ImGui. |
| 6 | **7.8** Options & High Scores | A | 3 h | Every backing piece exists — volume, difficulty, colourblind, and now key rebinding. This is the UI over them. |
| 7 | **Flag + power-up sprites** | — | 1 h | Two flag frames and 3 × 4 characters of Super/Fire/Cape art, or a documented palette-swap fallback. |

### Tier 2 — Depth the spec calls for (~16 h) — **DONE** (`A/tier2-bosses`)

All five items plus a shared foundation neither boss could exist without.
Caveats, so the ledger stays honest:

* Boom Boom's art is *derived*, not drawn: `tools/boss-frames/` recolours and
  scales Boomerang Bro, the nearest silhouette in the atlas. Bowser's art was
  already there.
* 9.4's second sub-item — rebalancing per-level enemy counts against the new
  difficulty modifiers — is not done and is left unticked.
* Same verification limit as Tier 1: two ctest suites pass (165 + 24 checks) and
  the game launches, but **nobody has fought either boss on screen.** This
  environment cannot inject keystrokes or capture the display.

Two more bugs fell out of the work, both pre-existing:

* `CollisionResolver` `static_cast` every `Projectile` to `Fireball`, so a
  Hammer Bro's hammer killed other enemies. Fixed in `d3e8b55` by dispatching
  through a real `Projectile` base.
* `Hammer::setupAnimations` was never called — hammers fell through the
  animation dispatcher's Player/Enemy/Item/Block chain and drew a placeholder.



| # | Task | Owner | Est. |
| :-- | :--- | :--- | ---: |
| 8 | **9.3** Bowser boss — 2 phases, health bar, arena | B | 5 h |
| 9 | **9.2** Boom Boom mid-boss | B | 3 h |
| 10 | **7.3** World Map State | A | 4 h |
| 11 | **9.4** Difficulty scaling — `DifficultyStrategy` | B | 2 h |
| 12 | **9.1** remainder — Thwomp SM, AI debug overlay | B | 2 h |

`EntityFactory` already returns `nullptr` for `Bowser` and `BoomBoom`, and the HUD already carries boss health-bar fields with no producer — the sockets are cut, the classes are missing.

### Tier 3 — Spec extras (~13 h)

| # | Task | Owner | Est. |
| :-- | :--- | :--- | ---: |
| 13 | **10.1** Object Pool → fireballs, particles, hammers | B | 2 h |
| 14 | **10.2** Config-driven entities — `entities.json` is present and read by nothing | B | 2 h |
| 15 | **10.4** Debug Console | B | 2.5 h |
| 16 | **10.3** Replay System | B | 3 h |
| 17 | **11.1** Two-Player Versus | A | 3.5 h |
| 18 | **11.3** Meta-game — NG+, daily challenge, unlockables | A | 2 h |
| 19 | **11.4** Accessibility — colourblind palette, audio cues | B | 2 h |
| 20 | **5.5** Parallax background | A | 2 h |
| 21 | **5.10** Water & lava animation | A | 2 h |
| 22 | **4.3** Camera lookahead + scroll modes | A | 1.5 h |

### Tier 4 — Hygiene (~3 h)

| # | Task | Est. |
| :-- | :--- | ---: |
| 23 | X-8 untrack 207 build artefacts | 0.5 h |
| 24 | A-13 adopt `resolvePath` in the last 4 files | 0.5 h |
| 25 | X-7 RAII EventBus subscription token | 1.5 h |
| 26 | Fast-forward `main`, tag a milestone | 0.5 h |

`main` is still at "Initial commit", now **169** behind `dev`. If the project is graded from the default branch it reads as empty.

---

## 4. Sequencing

**Do Tier 1 in the order listed.** Item 1 is a genuine blocker: `GameStateManager::render` draws only the top of the stack, so no overlay state can exist until it walks the stack. Four other tasks queue behind it.

After Tier 1 the game has a complete loop — menu, character select, play, pause, die, win, advance — and is demonstrable end to end. That is the milestone worth tagging.

Tier 2 is where the spec's headline features live; the bosses are the largest single gap.

**Split by owner, Tiers 1–2:** Member A ≈ 9.5 h, Member B ≈ 17.5 h. Given Member A wrote 16% of the sessions and Member B 53%, rebalancing toward A would be reasonable — items 8 and 11 transfer cleanly.

## 5. Standing rules for this work

From [AGENTS.md](../../AGENTS.md), the ones this project has actually broken before:

1. `git fetch --all` before reporting on repository state.
2. Log every session with `Reachable From Main` and `Verified By` filled in honestly.
3. A task is complete when it is reachable from `main()` **and observed running** — not when it compiles and a `verify_*` harness constructs it.
4. Add a `verify_regressions` case for each fix. The suite is at 76 checks; it is the reason this list is trustworthy.
