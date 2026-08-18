# PLAN: Member A remediation — 12 findings, sequenced

## Status

* **Owner**: Member A (Engine, Physics, Player/Item Entities, Levels, Save/Load)
* **Baseline**: `dev` @ `1602516` — builds clean, game launches, no startup errors
* **Source audit**: [code_audit_2026-08-18.md](code_audit_2026-08-18.md) · issue [#11](https://github.com/ndmhuy/SuperMarioGame/issues/11)
* **Already closed by the `enemies-behavior` merge**: A-2 (damage/power-down)
* **Remaining**: 12 findings — 2 critical, 5 high, 5 medium
* **Estimated total**: ~11 hours, of which the first 65 minutes clears both criticals

---

## Sequencing principle

Packages 1–3 are the criticals and cost about an hour combined. Package 4 then
installs the regression guard *before* the larger refactors, so packages 5–12
land against a suite that can catch them. Do not reorder 4 later — the whole
reason A-1 through A-7 survived to now is that nothing was watching.

| # | Finding | Sev | Effort | Blocks |
| :-- | :--- | :--- | ---: | :--- |
| WP1 | A-1 coin tiles dropped at load | **C** | 15 m | — |
| WP2 | A-3 dangling `m_player` after slot load | **C** | 20 m | — |
| WP3 | A-7 decorator destroys itself mid-update | H | 30 m | — |
| WP4 | X-3 register tests with CTest | H | 60 m | WP1–3 done |
| WP5 | A-5 rewind double-record + index drift | H | 45 m | WP4 |
| WP6 | A-6 head-butt breaks side bricks | H | 15 m | WP4 |
| WP7 | A-4 coyote time + jump buffer inert | H | 45 m | WP4 |
| WP8 | A-8 power-ups cancel Star/Mega | M | 20 m | WP3 |
| WP9 | A-14 no tile culling | M | 45 m | WP4 |
| WP10 | A-9 `render()` mutates state | M | 3–4 h | WP4 |
| WP11 | A-12 AABB resized in render | M | 60 m | WP10 |
| WP12 | A-10/A-11 dispatch + friend sprawl | M | 2 h | WP10 |

---

## WP1 — Coin tiles dropped at load (A-1) · 15 min · CRITICAL

**Problem.** `LevelLoader::loadLevel` hand-rolls a string→`TileType` chain that
accepts `"coin"`. The writer emits `"coin_tile"`. Every coin tile in all seven
levels loads as `Empty`: 9 lost in `level_1`, 12 in `level_2`, 15 in `level_3`,
12 in `bonus_1`, 11 in `level_1_sub`.

**Fix.** Delete the if-chain in [LevelLoader.cpp:106](../../SuperMarioGame/src/Utils/LevelLoader.cpp)
and call the existing inverse, which already handles both spellings:

```cpp
TileType tileType = SerializationUtils::parseTileTypeName(typeStr);
```

Do not patch the chain to also accept `"coin_tile"` — that leaves two mappings
free to drift again. One function, both directions.

**Acceptance.** For each of the 7 level files: load it, count tiles of type
`Coin`, assert `> 0`. Also assert a save→load round-trip preserves the count.

---

## WP2 — Dangling `m_player` after slot load (A-3) · 20 min · CRITICAL

**Problem.** [PlayingState.cpp](../../SuperMarioGame/src/Core/PlayingState.cpp)
does `m_entities[0] = std::move(loadedPlayer)`, destroying the object `m_player`
points at, without updating `m_player`, `Game::setPlayer`, or
`InputManager::registerPlayer`. The next `update()` is a use-after-free.

**Fix.** Three call sites already need the same four-step "adopt this player"
sequence (`spawnSelectedPlayer`, `loadLevelByPath`, the slot-load button). Extract it:

```cpp
void PlayingState::adoptPlayer(std::unique_ptr<Player> player) {
    m_player = player.get();
    if (m_entities.empty()) m_entities.insert(m_entities.begin(), std::move(player));
    else                    m_entities[0] = std::move(player);
    InputManager::getInstance().registerPlayer(m_player, 0);
    Game::getInstance().setPlayer(m_player);
    wireEntityAnimations(m_player);
}
```

Then make all three sites call it. This also removes the standing assumption
that `m_entities[0]` *is* the player, which the checkpoint auto-save and the
save/load panel both rely on today.

**Acceptance.** Save to a slot, load it, then step one frame and read the
player's position. Under ASan this crashes before the fix and passes after.

---

## WP3 — Decorator destroys itself mid-update (A-7) · 30 min

**Problem.** `StarDecorator::update` / `MegaDecorator::update` call
`player.changeState(std::move(m_wrappedState))`, and `changeState` assigns over
`m_currentState` — freeing the decorator while its own `update()` is on the
stack. It does not crash today only because nothing touches `this` afterwards.
A visible symptom already exists: the wrapped state's `exit()` is skipped and
its `enter()` runs a second time.

**Fix.** Never mutate the state machine from inside a state. Have the decorator
report expiry and let the owner act on it after the call returns:

```cpp
// IPlayerState.hpp
virtual bool isExpired() const { return false; }
// StarDecorator
bool isExpired() const override { return m_timeLeft <= 0.0f; }
void update(Player&, float dt) override { PlayerStateDecorator::update(...); m_timeLeft -= dt; }

// Player::update, after m_currentState->update(*this, dt):
if (m_currentState && m_currentState->isExpired()) {
    if (auto* deco = dynamic_cast<PlayerStateDecorator*>(m_currentState.get()))
        changeState(deco->releaseWrappedState());
}
```

**Acceptance.** Grant Star, advance `STAR_DURATION + 0.1s` of fixed steps,
assert the base state survives and that `enter()` fired exactly once on it.
Run under ASan.

---

## WP4 — Register the tests with CTest (X-3) · 60 min

**Do this before the refactors, not after.** There are 4,445 lines of test code
and 8 files already using `assert()`, but no `enable_testing()`, no `add_test()`,
and no `.github/`. Nothing has ever run them automatically. That is the direct
cause of this backlog surviving.

**Fix.**

1. Add to [CMakeLists.txt](../../SuperMarioGame/CMakeLists.txt):
   ```cmake
   enable_testing()
   add_test(NAME map_generator COMMAND verify_map_generator)
   add_test(NAME save_load     COMMAND verify_save_load)
   # ...one per assert-bearing harness
   ```
   The 20 targets currently re-list their sources by hand; a
   `function(add_verify_target name)` wrapper collapses ~450 lines of CMake.
2. Split the *headless* assertions out of the windowed `verify_*_visual`
   harnesses so CI can run them without a display.
3. Add the WP1–WP3 acceptance tests above as new cases.
4. Add `-Wall -Wextra` while in this file. It is free: the last full pass found
   only 7 issues, two of which (`m_powerUpSubId`, `m_levelCompleteSubId`) are
   direct evidence of an unwired feature.
5. Minimal `.github/workflows/ci.yml`: configure, build, `ctest --output-on-failure`.

**Acceptance.** `ctest` runs green locally and on a pushed branch.

---

## WP5 — Rewind double-records and restores by index (A-5) · 45 min

**Problem.** Two near-identical snapshot blocks run per frame
([PlayingState.cpp:299 and :420](../../SuperMarioGame/src/Core/PlayingState.cpp)),
halving the documented 5-second buffer. Worse, restore pairs `m_entities[i]`
with `snapshot.entityStates[i]` by position, while the prune step removes
inactive entities and the fireball listener appends new ones — so after any
prune, rewind assigns positions to the wrong entities.

**Fix.** Delete the earlier block, keeping the end-of-frame one. Then give
`Entity` a monotonic `uint32_t m_id` assigned in its constructor, store it in
`EntitySnapshot`, and restore by id lookup rather than index. Entities absent
from the snapshot are skipped; entities in the snapshot but since destroyed are
ignored.

**Acceptance.** Record 60 frames, destroy an entity mid-sequence, rewind fully,
assert every surviving entity returns to its recorded position.

---

## WP6 — Head-butt breaks side bricks (A-6) · 15 min

**Problem.** [PhysicsEngine.cpp:234](../../SuperMarioGame/src/Physics/PhysicsEngine.cpp)
gates on `col.normal.y == 1.0f || preVelY < 0.0f`. While rising, the second
clause is true for *horizontal* collisions, so bricks beside the player break.

**Fix.** Drop the disjunct: `if (col.tileX != -1 && col.normal.y == 1.0f)`.
The `preVelY` check was compensating for the resolver zeroing `velocity.y`
before this loop reads it — capture the pre-resolution normal instead.

**Acceptance.** Place the player in a 1-tile shaft with brick walls, jump into
the ceiling, assert the two side bricks survive and the ceiling brick breaks.

---

## WP7 — Coyote time and jump buffering are inert (A-4) · 45 min

**Problem.** `coyoteFramesLeft` is set and decremented; `jumpBufferFramesLeft`
is decremented but never set. Neither is read by `Character::jump()`, which
checks `onGround || onWall`. Task 2.5 lists both complete. Accepting `onWall`
also lets a held jump climb any wall indefinitely.

**Fix.** Override `jump()` in `Player`:

```cpp
void Player::jump() {
    if (onGround || coyoteFramesLeft > 0) {
        velocity.y = -jumpForce;
        coyoteFramesLeft = 0;
        jumpBufferFramesLeft = 0;
    } else {
        jumpBufferFramesLeft = Constants::JUMP_BUFFER_FRAMES;  // retry on landing
    }
}
```
and in `Player::update`, on the frame `onGround` becomes true with
`jumpBufferFramesLeft > 0`, consume the buffer and jump. Remove `onWall` from
the base `Character::jump()`; wall jumps already have their own command.

**Acceptance.** Walk off a ledge and jump within `COYOTE_FRAMES` — asserts the
jump fires. Press jump `JUMP_BUFFER_FRAMES` before landing — asserts it fires
on touchdown. Hold jump against a wall — asserts height does not increase.

---

## WP8 — Power-ups cancel Star/Mega (A-8) · 20 min

**Problem.** Fire Flower, Cape and Mini call `changeState(make_unique<...>())`
unconditionally, discarding any active decorator.

**Fix.** Add `Player::setBaseState(std::unique_ptr<IPlayerState>)` that walks
the decorator chain, swaps the innermost state, and leaves the wrappers intact.
Route all base-form power-ups through it; only Star and Mega should touch the
wrapper chain. Depends on the `releaseWrappedState()` accessor added in WP3.

**Acceptance.** Grant Star, then Fire Flower; assert the state is
`StarDecorator` wrapping `FireState` and that the star timer still runs out.

---

## WP9 — Tile loop draws the whole map (A-14) · 45 min

**Problem.** [PlayingState.cpp:435](../../SuperMarioGame/src/Core/PlayingState.cpp)
iterates the full grid every frame — roughly 4,400 individual sprite draws on a
200-wide level. `TileMap::render()` already implements the culled version
correctly and is now dead code.

**Fix.** Clamp the loop to `m_camera.getVisibleBounds()` (one extra tile of
margin), then batch into a single `sf::VertexArray` per atlas rather than one
`sf::Sprite` per tile. Fold the result back into `TileMap::render()` so there is
one tile renderer, not two. Apply the same clamp to the AABB debug overlay.

**Acceptance.** Frame time on `level_3` before/after, recorded in the PR.

---

## WP10 — `render()` mutates game state (A-9) · 3–4 h

**Problem.** Six `ImGui::Begin` blocks inside `PlayingState::render()` call
`setupTestScene()`, `MapGenerator::generate()`, `spawnSelectedPlayer()`,
`loadLevelByPath()`, `m_entities.push_back()`, `swapBricksAndCoins()` and
`Game::changeState()`. Render runs once per frame; update runs on the fixed
timestep. Mutations land off-cadence from the physics they affect.

**Fix.** Extract a `DevPanel` class owning the panel state
(`m_selectedLevelIndex`, `m_selectedCharIndex`, `m_showAABB`, generator config).
Give it `void drawAndCollect(PlayingState&)` called from `update()`, which
records requested actions into a small command queue that `update()` drains at a
defined point. `render()` keeps only `target.draw(...)` calls.

Do this after WP4 so the suite catches behavioural drift, and after WP5–WP9 so
those land in the smaller file.

**Acceptance.** `PlayingState::render` contains no assignment to a member and
no call into `Game`, `MapGenerator` or `LevelLoader`. Enforce by review.

---

## WP11 — AABB resized inside render (A-12) · 60 min

All four `render()` overrides assign `boundingBox.width/height` from
`m_targetSize`, so the collision box depends on the draw call happening. Move it
into `Entity::setTargetSize` (which already sets both) and delete the four
copies. This also removes the duplicated aspect-fit block flagged as X-6 —
one protected `Entity::drawSprite()` replaces Player/Enemy/Block/Item versions,
including the dead `scaledW`/`scaledH` locals in each.

**Acceptance.** Construct a player headlessly, never call `render`, assert the
bounding box matches the state's `getSize()`.

---

## WP12 — Dispatch and friend sprawl (A-10, A-11) · 2 h

Lowest urgency, highest grading weight — the project is scored on OOP design.

* `resolveEntityVsEntity` runs up to 12 sequential `dynamic_cast`s per colliding
  pair per frame. Replace with an `EntityCategory` enum returned by a virtual,
  and dispatch on the ordered pair.
* `SerializationUtils::getEntityTypeName` is 30 casts in a row. Replace with a
  virtual `getTypeName()`. Note `Player::getCharacterName()` already exists and
  returns exactly the strings that `Serializer.cpp:16` re-derives by cast —
  delete that static helper.
* `Entity` and `Character` each declare 12 friends including `PlayingState` and
  `friend int main()`. Every one is reachable through an existing public or
  protected method; `PlayingState`'s `m_player->lives = savedLives` should be
  `gainLife()`/`loseLife()` or a new `restoreStats()`.

**Acceptance.** `grep -c dynamic_cast src/Physics/CollisionResolver.cpp` drops
below 4; no `friend` in `Entity.hpp` or `Character.hpp`.

---

## Companion: make the rules stick

The audit's root cause (X-1) is that "checked" in the ledger has meant "the file
exists" rather than "reachable from `main()` and observed working". Two cheap
process changes, both suggested by evidence in `logs/agent_history.log`:

**1. Rules with a template slot get followed; prose rules do not.** The Git
Fingerprint rule was added 2026-08-09 and appears in **64 of 64** entries since —
100% compliance — because it got a named field in the log format. The
"fetch all remote branches before writing the report" rule in
[REPORT_RULES.md](../REPORT_RULES.md) is prose with no artifact, and gets
skipped. Give every rule you actually care about a required field.

Add to the log template:
```
Reachable From Main: <yes | no — harness only | n/a>
Verified By: <ctest case | manual playtest | build only>
```

**2. `AGENTS.md` is probably not too big — it may not be getting read.**
At 355 lines / ~4.5k tokens it is well within any agent's budget. But this repo
has *only* `AGENTS.md`: no `CLAUDE.md`, no `.cursorrules`, no `.cursor/rules/`,
no `.github/copilot-instructions.md`. Tools that look for a different filename
load nothing and behave like stock agents. Before editing content, confirm which
filename your tool reads, and add a thin pointer file for each tool in use.

If a trim is still wanted, cut by *structure*, not words: 355 lines contain only
7 numbered rules and 6 `MUST`/`NEVER` directives, and the version-control safety
rule — the one born from real data loss — sits at #7, below C++ naming
conventions. Front-load a ~40-line hard-rules block; move the tech-stack tables,
folder tree and naming conventions to a referenced appendix.

---

## Out of scope for Member A

Tracked in [#11](https://github.com/ndmhuy/SuperMarioGame/issues/11), owned by Member B:
B-2 (question blocks spawn no item), B-3 (hidden blocks unreachable), B-5
(moving platforms static), B-6 (Hammer Bro never throws — `setThrowCallback`
still has zero callers even after the merge), B-7 (Lakitu spawns nothing),
B-8 (no enemy–enemy collision, so shell chains are impossible), B-10 (LShift
runs *and* rewinds), B-12, B-13, B-15.

Two defects found by running the game after wiring, not yet filed:
* `build/assets/` contains only `levels/` — no `bgm`, `sfx`, `spriteSheet` or
  `font` is copied by CMake. Audio works today only because
  `ResourceManager::resolvePath` walks up into the source tree. A packaged build
  would be silent and textureless.
* `docs/Group52_08/` has `52.md` but no `52.pdf`, the only week missing one —
  fallout from the `git clean` incident recorded at `AGENTS.md:181`.
