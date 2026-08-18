# AUDIT: Full-project code review — 37 findings across both domains

## Status

* **Date**: 2026-08-18
* **Reviewed refs**: `origin/dev` @ `77b6d48`, cross-checked against `origin/B/feature-fix/enemies-behavior` @ `d954a66`
* **GitHub issue**: [#11](https://github.com/ndmhuy/SuperMarioGame/issues/11)
* **Hosted report**: https://claude.ai/code/artifact/cdc52910-00f5-4927-96a4-4036bc2c597f
* **Scope**: `SuperMarioGame/src`, `SuperMarioGame/include`, `SuperMarioGame/assets`, `SuperMarioGame/tests`, `CMakeLists.txt`, `TASK_DIVISION.md`
* **Totals**: 37 findings — 7 critical, 14 high, 16 medium. Two criticals (A-2, B-4) are already fixed on an unmerged branch.

---

## 0. Correction to the first pass

The first pass of this audit was run against a **stale local `dev`, 9 commits behind `origin/dev`**, in violation of [REPORT_RULES.md §2](../REPORT_RULES.md) ("the reporter must fetch all remote branches before writing"). One finding was wrong as a result and is retracted below. All other findings were re-verified against `origin/dev` and stand.

| Original finding | Corrected status |
| :--- | :--- |
| **B-1** — "There is no audio; Phase 6 absent" | **RETRACTED.** Phase 6 is implemented on `origin/dev` (merged in `9976209 feat: wire sfx and music to the game`): 41 audio files (12 BGM + 29 SFX), `loadSoundBuffer` called, `playMusic` called, 16 EventBus subscriptions in `SoundManager`. The ledger entry is accurate. |
| **A-2** — "Player cannot take damage" | **Fixed upstream, unmerged.** `Player::takeDamage` override calling `powerDown()` exists on `B/feature-fix/enemies-behavior`. Still broken on `dev`. |
| **B-4** — "Flying enemies dragged by gravity" | **Fixed upstream, unmerged.** Boo, BulletBill, KoopaParatroopa, Lakitu, PiranhaPlant and Thwomp all override `getGravityMultiplier()` on that branch. Still broken on `dev`. |
| **B-8** — "Kicked shell passes through enemies" | **Partially addressed upstream.** That branch adds shell pick-up/carry mechanics, but `resolveEntityVsEntity` still has no enemy–enemy branch, so shell chains remain impossible. |

---

## 1. Branch topology

`git fetch --all` then ahead/behind against `origin/dev`:

| Branch | Ahead | Behind | Last commit | State |
| :--- | ---: | ---: | :--- | :--- |
| `origin/main` | 0 | 153 | 2026-05-29 | **Tip is "Initial commit."** Nothing has ever landed on the default branch. |
| `origin/dev` | — | — | 2026-08-13 | Integration branch; the de-facto trunk. |
| `origin/B/feature-fix/enemies-behavior` | 1 | 0 | **2026-08-16** | **Newest work in the repo, unmerged.** Fixes A-2 and B-4. |
| `origin/B/feature/sfx-music` | 0 | 2 | 2026-08-13 | Merged into `dev`. |
| `origin/B/feature/graphic-visual` | 0 | 1 | 2026-08-13 | Merged into `dev` via PR #10. |
| `origin/B/feature/entities` | 0 | 51 | 2026-07-24 | Fully merged; stale. |
| `origin/B/feature/core-engine` | 0 | 134 | 2026-06-12 | Fully merged; stale. |
| `origin/A/spritesheet-studio` | 1 | 100 | 2026-07-13 | 1 unmerged commit. |
| `origin/A/core-engine` | 0 | 131 | 2026-06-16 | Fully merged; stale. |
| `origin/A/feature/physics` | 0 | 110 | 2026-06-22 | Fully merged; stale. |
| `origin/A/feature/save-load` | 0 | 83 | 2026-07-18 | Fully merged; stale. |
| `origin/A/feature/level-editor` | 0 | 70 | 2026-07-19 | Fully merged; stale. |
| `origin/A/physics-input-test` | 0 | 90 | 2026-07-18 | Fully merged; stale. |
| `origin/feature/core-engine` | 0 | 132 | 2026-06-16 | Orphan duplicate of `B/feature/core-engine`. |

**Big-picture reading.**

1. **`main` is 153 commits behind and its tip is the initial commit.** There is no releasable ref and no tagged milestone. If the deliverable is judged from the default branch, it is empty. Fast-forwarding `main` to `dev` should happen before submission.
2. **The most recent work in the repo is unmerged.** `B/feature-fix/enemies-behavior` (2026-08-16) sits one commit ahead of `dev` and fixes two of this audit's critical findings. Merging it is the single highest-value action available.
3. **Member A's branches are all fully merged and inactive since 2026-07-19.** All recent activity — the last month — is Member B's. Worth naming explicitly in the weekly report rather than leaving implied.
4. **Eleven of fourteen branches are fully merged and stale.** They should be deleted; keeping them makes the branch list unreadable and hides which two actually carry work.
5. **`origin/feature/core-engine`** duplicates `B/feature/core-engine` with no owner prefix — it predates the naming convention and should go.

---

## 2. Findings

Severity: **C** critical (crash, UB, or a feature that cannot run) · **H** high (a shipped mechanic that silently does nothing) · **M** medium (design, performance, duplication, hygiene).

### Member A — player & world

| ID | Sev | Finding | Location |
| :-- | :-- | :--- | :--- |
| A-1 | **C** | **Every coin tile in every level is dropped at load.** The writer emits `"coin_tile"`; the loader hand-rolls its own comparison chain accepting only `"coin"`, so every coin tile becomes `Empty`. The correct inverse, `parseTileTypeName`, handles both and is never called. Measured: 9 tiles lost in `level_1`, 12 in `level_2`, 15 in `level_3`, 12 in `bonus_1`, 11 in `level_1_sub` — all 7 level files. | [LevelLoader.cpp:106](../../SuperMarioGame/src/Utils/LevelLoader.cpp) · [SerializationUtils.cpp:48](../../SuperMarioGame/src/Utils/SerializationUtils.cpp) |
| A-2 | **C** | **The player cannot take damage or power down.** `powerDown()` has no callers on `dev`; `takeDamage(1)` decrements a `health` field nothing reads for a Player. `invincibilityTimer` is only assigned inside the unreachable `powerDown()`, so i-frames are dead too. *Fixed on `B/feature-fix/enemies-behavior`.* | [Player.cpp:78](../../SuperMarioGame/src/Entities/Player.cpp) · [CollisionResolver.cpp:163](../../SuperMarioGame/src/Physics/CollisionResolver.cpp) |
| A-3 | **C** | **Use-after-free on save-slot load.** `m_entities[0] = std::move(loadedPlayer)` destroys the object `m_player` points at without reassigning `m_player`, `Game::setPlayer` or `InputManager::registerPlayer`. | [PlayingState.cpp:939](../../SuperMarioGame/src/Core/PlayingState.cpp) |
| A-4 | H | **Coyote time and jump buffering never affect a jump.** Both are counted; neither is read by `Character::jump()`, and `jumpBufferFramesLeft` is never even set. `jump()` also accepts `onWall`, so a held jump climbs any wall. | [Player.cpp:202](../../SuperMarioGame/src/Entities/Player.cpp) · [Character.cpp:23](../../SuperMarioGame/src/Entities/Character.cpp) |
| A-5 | H | **Rewind records twice a frame and restores against shifted indices.** Two near-identical snapshot blocks run per frame, halving the documented 5-second buffer. Restore pairs `m_entities[i]` to `snapshot.entityStates[i]` by index while the prune step and fireball spawns shift those indices. | [PlayingState.cpp:299, :420](../../SuperMarioGame/src/Core/PlayingState.cpp) |
| A-6 | H | **Jumping in a narrow shaft destroys the side walls.** The head-butt loop's `\|\| preVelY < 0.0f` clause matches horizontal collisions while rising. | [PhysicsEngine.cpp:234](../../SuperMarioGame/src/Physics/PhysicsEngine.cpp) |
| A-7 | H | **Star/Mega decorators destroy themselves mid-update.** `changeState` is called from inside the decorator's own `update()`, freeing `this` while the call is on the stack. Already causes the wrapped state's `exit()` to be skipped and `enter()` to fire twice. | [IPlayerState.cpp:82, :102](../../SuperMarioGame/src/Entities/IPlayerState.cpp) |
| A-9 | H | **`PlayingState::render()` mutates game state.** ~600 of 1202 lines are ImGui panels (5 `ImGui::Begin` blocks) calling `setupTestScene()`, `MapGenerator::generate()`, `loadLevelByPath()`, `changeState()`. Render runs per frame; update runs on the fixed timestep. | [PlayingState.cpp:424–1020](../../SuperMarioGame/src/Core/PlayingState.cpp) |
| A-8 | M | Fire Flower / Cape / Mini call `changeState` unconditionally, discarding an active Star or Mega decorator. | [Player.cpp:48](../../SuperMarioGame/src/Entities/Player.cpp) |
| A-10 | M | **Type-switch cascades where dispatch belongs.** 12 sequential `dynamic_cast`s per colliding pair per frame; ~6 per entity per frame in `PhysicsEngine`; 30 in a row in `getEntityTypeName`. `Serializer.cpp:16` re-derives by cast what the existing virtual `Player::getCharacterName()` returns. | [CollisionResolver.cpp:56](../../SuperMarioGame/src/Physics/CollisionResolver.cpp) · [SerializationUtils.cpp:66](../../SuperMarioGame/src/Utils/SerializationUtils.cpp) |
| A-11 | M | `Entity` and `Character` each declare 12 friends including `PlayingState` and `int main()`; `PlayingState` writes `m_player->lives` directly beside a public `gainLife()`/`loseLife()`. | [Entity.hpp:36](../../SuperMarioGame/include/Entities/Entity.hpp) |
| A-12 | M | The physics AABB is resized inside `render()` — in all four render overrides. | [Player.cpp:268](../../SuperMarioGame/src/Entities/Player.cpp) |
| A-13 | M | Asset paths guessed at 4 call sites (13 font candidates, 11 level candidates, two 3-path lists). Note `ResourceManager::resolvePath` now exists on `dev` — adopt it everywhere. | [Game.cpp:36](../../SuperMarioGame/src/Core/Game.cpp) · [LevelLoader.cpp:35](../../SuperMarioGame/src/Utils/LevelLoader.cpp) |
| A-14 | M | No camera culling in the tile loop: ~4,400 sprite draws per frame on a 200-wide level. `TileMap::render()` already implements the culled version and is dead code. | [PlayingState.cpp:435](../../SuperMarioGame/src/Core/PlayingState.cpp) · [TileMap.cpp:60](../../SuperMarioGame/src/Utils/TileMap.cpp) |

### Member B — enemies & interaction

| ID | Sev | Finding | Location |
| :-- | :-- | :--- | :--- |
| B-2 | **C** | **Question blocks never spawn an item.** `onHitFromBelow` publishes `PowerUpCollected` with a comment saying it notifies the factory; nothing subscribes. `PlayingState` declares an `m_powerUpSubId` the compiler flags as unused. The event is also overloaded — `Player::powerUp` publishes the same type on pickup, so a naive handler would recurse. | [QuestionBlock.cpp:30](../../SuperMarioGame/src/Entities/QuestionBlock.cpp) |
| B-3 | **C** | **Hidden blocks can never be revealed.** `getBoundingBox()` returns an empty AABB while `!m_isRevealed` → never collides → `onHitFromBelow` never fires → never revealed. Deadlocked by construction; task 3.13 checked. | [HiddenBlock.cpp:56](../../SuperMarioGame/src/Entities/HiddenBlock.cpp) |
| B-4 | **C** | **Flying and scripted enemies are dragged down by gravity.** On `dev`, `Block` is the only class overriding `getGravityMultiplier()`. Strategies set velocity in `update()`; `PhysicsEngine` then adds +30 px/s every frame. PiranhaPlant is told to emerge at −64 px/s and rises at −34. *Fixed on `B/feature-fix/enemies-behavior`.* | [PhysicsEngine.cpp:155](../../SuperMarioGame/src/Physics/PhysicsEngine.cpp) |
| B-5 | H | **Moving platforms never move.** `EntityFactory` constructs them with `travelRange = (0,0)`, so `m_rangeLen` is zero and update takes the stationary branch. `level_1.json` alone has 5. | [EntityFactory.cpp:121](../../SuperMarioGame/src/Entities/EntityFactory.cpp) |
| B-6 | H | **Hammer Bro never throws.** `setThrowCallback` has no callers, so the throw branch is dead, and no `Hammer` projectile class exists. The shuffle also uses `sin(m_jumpCooldownTimer * 2.0f)` — a countdown timer as oscillator phase. | [HammerThrowStrategy.cpp:25, :45](../../SuperMarioGame/src/Entities/HammerThrowStrategy.cpp) |
| B-7 | H | **Lakitu never spawns Spinies.** `m_eggTimer` and `m_spawnCount` advance and a sound is requested, but no entity is constructed. | [Lakitu.cpp:44](../../SuperMarioGame/src/Entities/Lakitu.cpp) |
| B-8 | H | **A kicked shell cannot defeat another enemy.** `resolveEntityVsEntity` has no enemy–enemy branch, so shell chains are impossible. *Carry/pick-up mechanics added on `B/feature-fix/enemies-behavior`; the enemy–enemy gap remains.* | [CollisionResolver.cpp:56](../../SuperMarioGame/src/Physics/CollisionResolver.cpp) |
| B-9 | H | **Five finished graphics subsystems are never connected.** `Minimap`, `ParticleSystem`, `ParticleEmitter`, `AnimationManager`, `SpriteColorFilter`, `SpriteTransformAnim` are referenced only by their own sources and the `verify_*_visual` harnesses. `EntityDeathEffect::render` is called but never fed; `ScreenTransitionManager` is drawn but never triggered. Tasks 5.2/5.4/5.6/5.8/5.9 all checked. | [SuperMarioGame/src/Graphics/](../../SuperMarioGame/src/Graphics) |
| B-10 | H | **`LShift` both runs and rewinds.** `InputManager` binds it to `RunCommand`; `PlayingState::update` polls it directly as the rewind trigger, so holding Shift to run rewinds instead. The poll also bypasses the Command layer. | [InputManager.cpp:66](../../SuperMarioGame/src/Core/InputManager.cpp) · [PlayingState.cpp:262](../../SuperMarioGame/src/Core/PlayingState.cpp) |
| B-11 | M | Rebound keys are persisted to `config.json` but never applied; `InputManager` exposes no rebinding API. Tracked as [#9](https://github.com/ndmhuy/SuperMarioGame/issues/9) / [member_b_input_sync.md](member_b_input_sync.md). | [InputManager.cpp:42](../../SuperMarioGame/src/Core/InputManager.cpp) |
| B-12 | M | `m_isRed` selects a strategy but never reaches `setupAnimations`, which hardcodes `koopa_green_*` / `goomba_brown_*`, so red variants render green. | [KoopaTroopa.cpp:45](../../SuperMarioGame/src/Entities/KoopaTroopa.cpp) |
| B-13 | M | Goomba and Spiny integrate their own motion in `update()` while `PhysicsEngine` integrates them again — flipped enemies fall at double speed. Despawn also compares a world Y against `WINDOW_HEIGHT`. | [Goomba.cpp:22](../../SuperMarioGame/src/Entities/Goomba.cpp) · [Spiny.cpp:31](../../SuperMarioGame/src/Entities/Spiny.cpp) |
| B-14 | M | Goomba / Spiny / HiddenBlock return a reference to a function-local `static` zero AABB to opt out of collision, inserting them into spatial-hash cell (0,0) every frame. An explicit `isCollidable()` would say what is meant. | [Goomba.cpp:76](../../SuperMarioGame/src/Entities/Goomba.cpp) |
| B-15 | M | Per-enemy `m_scoreValue` is published with `EnemyDefeated`, but the award is a hardcoded `100 * combo`, so every enemy is worth the same. | [CollisionResolver.cpp:156](../../SuperMarioGame/src/Physics/CollisionResolver.cpp) |

### Cross-cutting

| ID | Sev | Finding | Location |
| :-- | :-- | :--- | :--- |
| X-1 | **C** | **The task ledger does not describe the code.** Five sub-tasks (5.2, 5.4, 5.6, 5.8, 5.9) are checked while the code has no caller; 1.5, 2.5, 3.9, 3.13 and 4.3 are checked but partial — key rebinding, coyote time, the Lakitu spawner, Hammer Bro's hammers, hidden blocks, and the camera's lookahead and scroll modes are all absent. This is the finding that produces the others. Treat "checked" as *reachable from `main()` and observed working*. | [TASK_DIVISION.md](../../TASK_DIVISION.md) |
| X-2 | H | **2 of 10 game states exist.** No Pause, Game Over, Victory, Character Select, Statistics, Achievements, World Map or Options. No boss classes; `EntityFactory` returns `nullptr` for Bowser/BoomBoom while the HUD carries boss health-bar fields. Phase 10 (ObjectPool, Replay, DebugConsole, config-driven entities) absent; `assets/config/entities.json` read by nothing. `GameStateManager::render` draws only the stack top, so a transparent pause overlay needs that changed first. | [include/Core/](../../SuperMarioGame/include/Core) · [GameStateManager.cpp:47](../../SuperMarioGame/src/Core/GameStateManager.cpp) |
| X-3 | H | **4,445 LOC of tests that CI cannot run.** 18 `verify_*` targets, 8 using `assert()`, but no `enable_testing()`/`add_test()` in the 552-line CMakeLists and no `.github/`. This is why B-2 and B-3 survived — each is one assertion away from being caught. | [CMakeLists.txt](../../SuperMarioGame/CMakeLists.txt) |
| X-4 | M | No `-Wall -Wextra`. Enabling them is free: a pass over the 10 largest TUs found only 7 issues, and two (`m_powerUpSubId`, `m_levelCompleteSubId`) are direct evidence of B-2. | [CMakeLists.txt](../../SuperMarioGame/CMakeLists.txt) |
| X-5 | M | The update/render contract is broken on both sides; `MenuState::render` also calls `changeState`. A `const` render path would surface every instance at compile time. | [MenuState.cpp:43](../../SuperMarioGame/src/Core/MenuState.cpp) |
| X-6 | M | The sprite draw block is copied 4× across Player/Enemy/Block/Item, including the same two dead locals in every copy. One protected `Entity::drawSprite()` replaces all four. | [Enemy.cpp:24](../../SuperMarioGame/src/Entities/Enemy.cpp) · [Block.cpp:36](../../SuperMarioGame/src/Entities/Block.cpp) |
| X-7 | M | `EventBus::publish` deep-copies the subscriber vector per event. `Camera` defaults its move constructor while its handlers capture `this`. An RAII subscription token fixes both. Note the new `SoundManager` subscriptions are never unsubscribed. | [EventBus.cpp:27](../../SuperMarioGame/src/Core/EventBus.cpp) · [Camera.hpp:32](../../SuperMarioGame/include/Graphics/Camera.hpp) |
| X-8 | M | **205 clangd index files, 2 `.DS_Store` and `.member_profile.json` are tracked** despite being in `.gitignore` — committed before the rules landed, and ignoring does not untrack. `git rm -r --cached` clears it. Also: `.member_profile.json` is checked in with `"memberName": "B"`; if it drives tooling on Member A's machine it is pointed at the wrong half. | `.cache/clangd/index/` · `.gitignore` |
| X-9 | H | **`main` has never received a merge** (tip = "Initial commit", 153 behind), and the newest work in the repo sits unmerged on `B/feature-fix/enemies-behavior`. Eleven of fourteen branches are fully merged and stale. See §1. | git topology |

---

## 3. Suggested order

Sequenced by playable-minutes recovered per hour spent.

1. **Merge `B/feature-fix/enemies-behavior` into `dev`** — closes A-2 and B-4, both critical, at zero implementation cost.
2. **A-1** — one line; restores coins across all seven levels. Swap the loader's if-chain for `parseTileTypeName`.
3. **A-3, A-7** — the two memory bugs. Reassign `m_player` after a slot load; defer the decorator's state swap out of its own `update()`.
4. **B-2** — add a spawn subscriber and split the overloaded `PowerUpCollected` event into request and pickup.
5. **B-3, B-5, B-6, B-7, B-10** — the dead mechanics; each a small self-contained fix that turns an existing class back on.
6. **X-3** — register the tests with CTest so this class of regression stops recurring.
7. **A-9** — lift the ImGui panels out of `render()`.
8. **X-9** — fast-forward `main` to `dev`, tag a milestone, delete the eleven stale branches.
9. **X-1** — re-baseline the ledger, then keep "checked" meaning reachable and observed.

---

## 4. Method

`git fetch --all`, then every finding re-verified against `origin/dev` with `git show <ref>:<path>` rather than the working tree. Counts by direct inspection: LOC via `wc`; level-tile losses by parsing each level JSON against the loader's accepted type strings; call-site counts by repository-wide `git grep`; warnings via a clang `-Wall -Wextra` pass over the ten largest translation units. The project builds clean.

Working tree at time of review was dirty (uncommitted edits to `CMakeLists.txt`, four level JSONs, and several headers); all findings above were confirmed against committed refs, not those edits.
