# Submission Sweep Plan — 2026-09-02

**What this is**: the phased work plan for the final submission sweep of the
CS202 Super Mario project — archive, defects, evidence capture, document
refresh, report rewrite, audit, package — with the dependency graph, what can
run in parallel, and the Claude model + effort level for every lane, plus the
OOP / SOLID / design-pattern findings that the report rewrite must answer.
Written so each lane can be handed to a subagent as a ready-to-paste prompt.

**Who it is for**: the user (who decides gates, merges and everything that
touches GitHub or `main`), and the subagents that run the lanes.

**Supersedes**: [`spec_feature_audit_2026-08-31.md`](spec_feature_audit_2026-08-31.md) §6
(its plan R1–R21 is complete; its findings stay valid as history) and the
open-items list in [`release_batch_r21_2026-09-02.md`](release_batch_r21_2026-09-02.md) §5,
which is folded into Phase 2 below.

**Repo state when written** (`git fetch --all` run first):
`dev` @ `6af4f8e` == `origin/dev` (`0 0`); `origin/main` is **144 commits
behind** `dev` (promoting `main` is the user's decision, reserved by them);
branches ahead of `origin/dev`: `A/mapgen-gan-plan` (+59), `A/rl-neural-policy`
(+1), `A/spritesheet-studio` (+1) — the first two are the AI side projects and
stay unmerged by standing decision. CI (`.github/workflows/ci.yml`,
ubuntu-latest, ctest under xvfb) is green on the last three `dev` pushes.
Working tree: `saves/config.json` is modified (`debugMode: true`, music 70) —
local settings, **not to be committed**. Last package build: 2026-08-31 11:32
(58-page PDF). Since 2026-08-29: **175 commits, 626 files, +43,156 / −7,293**;
74 log sessions since 2026-08-16.

---

## 0. The user's five report findings, and where each is answered

| # | Finding | Answered in |
| :-- | :--- | :--- |
| 1 | Detailed reasoning for the design — every pattern, and the OOP/SOLID choices | Phase 5 lanes 5A/5B (§7 rewritten one subsection per pattern: problem → naive alternative → why this pattern → what it cost; §6 gains a SOLID walk-through) fed by §2–§3 below — **and lane 5E writes the two learning records the rules require** (g-rule-21), which the report then links instead of restating |
| 2 | Screenshots of everything (entities, blocks, levels, bosses, characters, menus, modes) — in §8 or §11? | **Decision: both, with different jobs.** §11 *Features demonstration* becomes the systematic catalogue (contact-sheet figures per family). §8 *Implementation details* keeps at most one figure per subsection, only where a picture explains the mechanism (lighting shader, pipe entry modes, editor, endless chunk seam, versus HUD). Rationale in §5.2. Phase 3 captures, Phase 5 places |
| 3 | Future work = far goals (trained AI agents, AI map generation), not un-built SPEC items | Phase 5 lane 5C rewrites §14 around the two side branches that already exist (`A/rl-neural-policy`: neural policy, reward, experience log; `A/mapgen-gan-plan`: GAN/RL generation over the TheVGLC corpus) — described as direction, not as delivered work; un-built SPEC items move to §13 *known gaps* only |
| 4 | Clean GitHub issues; the report should describe the audit *procedure* (checkpoint audits, repeated) and main points, not the findings | Phase 7 (issues, user-gated) and Phase 5 lane 5C (§12 rewritten as procedure) |
| 5 | Overlapping text in the PDF | Root cause pinned in §4.1; fixed in Phase 5 lane 5D; a build guard stops it recurring; Phase 6 renders every page and looks |

---

## 1. Rules that bind every lane (from AGENTS.md — read it first)

1. `git fetch --all` before reading or reporting repository state; record `Fetched Remotes`.
2. Branch off `dev` as `A/<lane-name>`; **no auto-merge, no push** — the user integrates. One worktree per concurrent lane, **and one `FETCHCONTENT_BASE_DIR` per lane**: the R21 batch corrupted a shared `build/_deps` across concurrent reconfigures (96-byte `libImGui-SFML.a`).
3. Never discard uncommitted work to unblock git; commit it. Only `imgui.ini` is discardable.
4. "Complete" = reachable from `main()` **and observed running** (ctest or a scripted/manual run). Harness-only work is logged as `Reachable From Main: no — harness only`.
5. Append a full `logs/agent_history.log` entry per session: `Git Fingerprint`, `Fetched Remotes`, `Prompt`, `Files Modified`, `Reachable From Main`, `Verified By`, `Summary`. `build only` is an honest answer. Record mistakes made.
6. Every new guard is **mutation-tested** (revert the fix, watch it fail) before it counts.
7. Documents restate code only through generation (`build_report.py`, `gen_class_diagram.py`); counts are computed, not typed. One current doc per topic; superseded docs go to `docs/archive/` with a date prefix, never deleted.
8. SPEC.md is the behavioural source of truth; do not "fix" a value that disagrees with it without asking.
9. Nothing touches `origin`, GitHub issues, or `main` without the user's explicit go in this session.
10. **Every substantial piece of work gets a learning record** (g-rule-21, user
    instruction 2026-09-02): the OOP/SOLID fixes and the design-pattern reasoning
    are not done when the report says them — they are done when
    `docs/learning/<topic>.html` exists with the twelve mandated sections
    (prerequisites; what & why with a rejected-alternatives table; tech stack;
    CS foundations; text-authored diagrams; **how it works with real code
    excerpts extracted by `SuperMarioGame/tools/extract_learning_excerpts.py`
    and checked with `--check` so a shifted line range fails loudly**; a worked
    trace with real values; build & run; outcome & evidence; pitfalls; revision
    aids; references; changelog + "verified against commit <sha>"). Skeleton:
    `node ~/Documents/AgentHub/scripts/new_learning_doc.js super-mario-game "<Topic>"`,
    which also rebuilds `docs/learning/index.html`. The existing
    `docs/learning/mid-frame-entity-spawn-crash.html` is the house example.

Model tiers used below: **Haiku 4.5** for mechanical/search work, **Sonnet 5** for scoped implementation and document drafting from a given source, **Opus 5** for design-sensitive code and accuracy-critical audits, **Fable 5.1** for the two lanes where reasoning quality is the deliverable (report rewrite, final audit). Effort is the reasoning-effort setting to run the lane at.

---

## 2. OOP and SOLID — findings to fix or to argue in the report

Method: direct greps over `SuperMarioGame/include` + `src` at `dev` @ `6af4f8e`
(the two Opus auditors assigned to this section were killed by the org's spend
limit mid-run; their partial scratch output was re-verified and extended here).
Numbers are counts of textual hits unless marked *real*.

### 2.1 Strengths — the evidence the report should lead with

- **Encapsulation is real.** A precise scan of every `class` body finds **0 public
  mutable data members**; the only public aggregates are `struct`s used as data
  carriers (`GameSnapshot`, `PlayerSnapshot`, `EntitySnapshot`, config entries).
  Exactly one getter returns a mutable reference to internals:
  `Camera::getView()` → `sf::View&` (`include/Graphics/Camera.hpp:69`).
- **Ownership is `unique_ptr` throughout.** Zero `delete` in `src`/`include`.
  Raw `new` appears 8 times: 7 in `DevPanel.cpp:610-628` (spawner lambdas that
  wrap the result in `unique_ptr` on the same line) and the deliberate, documented
  never-destroyed `ResourceManager` instance (`ResourceManager.cpp:33`).
- **Every hierarchy root has a virtual destructor**: `Entity`, `IGameState`,
  `ICommand`, `IMovementStrategy`, `IPlayerState`, `IAIPolicy`, `IEntityAdmitter`,
  `IEditorCommand`, `IConsoleCommand`, `IDifficultyStrategy`. Derived abstract
  classes (`Character`, `Player`, `Boss`, `Item`, `Block`, `Projectile`) inherit it.
- **Polymorphic dispatch replaced RTTI where it matters.** `CollisionResolver.cpp`
  has **0 real** `dynamic_cast` (4 textual hits are comments explaining the
  refactor); `PhysicsEngine.cpp` 0. Contact rules are virtual hooks
  (`onStomped`, `onHitFromBelow`, `onPlayerTouch`, …) plus an ordered
  `EntityCategory` pair switch (`CollisionResolver.cpp:88-119`) — the R5 refactor
  (28 casts → 0) is intact.
- **Template Method is textbook**: `IMovementStrategy::execute()` is non-virtual
  and fixes `calculateTarget → applyMovement → checkConstraints`
  (`IMovementStrategy.hpp:13-17`); `Boss::update()` is sealed and calls
  `updateBehaviour()`.
- **Open/closed for entity types is now mechanical**: adding an entity is one
  `EntityCatalogue` row (`EntityCatalogue.cpp:70+`), and
  `verify_r21_entity_registry` walks `0..EntityType::Count` so a missing row
  fails the build instead of becoming a dead palette button.
- **Observer lifetime is engineered**: `std::function` subscribers, tombstoned
  unsubscribe, re-entrancy depth counter, and RAII `ScopedSubscription`
  (`EventBus.hpp:79-121`); `PlayingState` holds 15 scoped subscriptions.

### 2.2 Findings, ranked

| Sev | Finding | Evidence | Disposition |
| :-- | :--- | :--- | :--- |
| High (SRP) | `PlayingState` is a god class: **3,983** lines `.cpp` + **796** `.hpp` — level load, spawning, physics orchestration, camera, HUD sync, boss arenas, 2P, rewind, pipes, cheats, editor bridge, endless chunks | `wc -l`; next largest files: `MenuState.cpp` 972, `MapEditor.cpp` 876, `Player.cpp` 823, `EditorState.cpp` 815, `DevPanel.cpp` 762 | **Document as trade-off** in §6, with the extraction trend as evidence (`DevPanel`, `EditorBridge`, `DebugCheats`, `LightingRenderer`, `PipeRenderer` were all split out); no refactor this week |
| High (DIP) | 12 Meyers/leaked singletons reached from everywhere: `Game` **139 call sites / 37 files**, `SoundManager` 85 / 30, `EventBus` 50 / 28, `InputManager` 30 / 12, `ResourceManager` 24 / 8, `AchievementManager` 17 / 8, `ScreenTransitionManager` 16 / 1, `StatisticsTracker` 13 / 7, `DebugConsole` 7 / 3, `ReplayRecorder` 7 / 3, `EntityDeathEffect` 4 / 1, `ParticleSystem` 4 / 3 | `grep -r '::getInstance()'` excluding each class's own files | **Document**: SPEC §2.2 says "4 singletons" — the report must own 12 and argue the construction-order rationale; no injection refactor this week |
| Medium (OCP) | Type dispatch by `dynamic_cast` chains outside the resolver: **140** textual hits (117 *real*); hot spots `PlayingState.cpp` 29, `Player.cpp` 28 (decorator-chain walking), `DevPanel.cpp` 11, `Serializer.cpp` 10, `EditorCommands.cpp` 7, `EntityArtBinder.cpp` 7, `LevelLoader.cpp` 5 (per-type serialization), `CollisionDetector.cpp` 3 (MiniState water-walk, deliberately kept) | scratch `dyncast_real.txt` | Cheap subset → lane 2E (below); the rest documented: the Decorator chain is only inspectable by cast (10 sites) — a known cost of the pattern |
| Medium (OCP) | `switch (EntityType)` outside factory/registry: `PlayingState::spawnProjectile` (`PlayingState.cpp:3881`) and the three-way cast in `recycleEntity` (`:3897-3908`) | grep | **Cheap fix** (lane 2E): route through `EntityFactory::create`; give `Entity` a virtual pool tag |
| Medium (Encaps.) | Friend sprawl unchanged: `Entity.hpp:245-256` **12 friends**, `Character.hpp:58-69` **12**, `PlayingState.hpp` 4 (`DevPanel` + 3 test hooks), `Player.hpp` 3, `Hud.hpp` 1 | `friends.txt` | SPEC §22 count still accurate → **documented trade-off**; add the test-hook friends to the §22 text |
| Medium (State) | 4 of 5 `IPlayerState` forms have empty bodies (`IPlayerState.cpp:7-25, 59-62`); only `CapeState::update` carries behaviour — the forms differ by `getSize()` alone | patterns audit | State it honestly in §6.4: the axis is paid for and under-used; the Decorator layer is where behaviour lives |
| Medium (Memento) | `GameSnapshot` is a fully public `struct` — no narrow interface, Originator internals readable anywhere | `GameSnapshot.hpp:44-60` | Document (a C++ aggregate memento is a common simplification); optional: make fields private with `PlayingState`/`Player` as friends — not this week |
| Low (Observer) | `SoundManager`'s 20 subscriptions never unsubscribe; safety rests on `g_eventBusAlive` (`EventBus.cpp:19-29`); `Hud` subscribes to nothing (polls) though SPEC lists it as an observer | `SoundManager.cpp:107-188` | Cheap consistency fix (lane 2E): hold them as `ScopedSubscription`; correct SPEC's subscriber list |
| Low (Encaps.) | `Camera::getView()` returns `sf::View&` | `Camera.hpp:69` | Cheap fix (lane 2E): const ref + a `setViewport`-style operation |
| Low (Lifetime) | Non-owning raw pointers `m_player`, `m_player2`, `m_shadow`, `m_activeBoss` into `m_entities` (`PlayingState.hpp:278-458`); `forgetEntity` now clears all four (R21) | header | Document; the R21 use-after-free fix is the evidence that the invariant is now guarded |
| Low (Hygiene) | 7 `new X(p)` in `DevPanel.cpp:610-628` instead of `std::make_unique` / the factory | `new.txt` | Cheap fix (lane 2E) |

### 2.3 What lane 2E fixes (each under ~1 h, all mutation-testable)

1. `DevPanel.cpp:610-628`: replace the seven `new X(p)` lambdas with
   `EntityFactory::create(EntityType::X, p)` (fixes OCP and the raw `new` at once).
2. `PlayingState::spawnProjectile` (`:3881`) switch → `EntityFactory::create`.
3. `recycleEntity` (`:3897-3908`): virtual `Entity::poolKind()` (or per-type
   `releaseToPool`) instead of three sequential casts.
4. `Camera::getView()` → `const sf::View&` plus the one mutating operation callers need.
5. `SoundManager`: keep its 20 subscriptions in a `std::vector<ScopedSubscription>`.
6. SPEC §2.2 / README / report participant lists corrected to the code (see §3.2).

Everything else in §2.2 is a **design decision to explain**, not a change to make
in submission week.

---

## 3. Design patterns — what the code actually has, and the reasoning the report needs

Verified read-only at `dev` @ `6af4f8e` by the pattern auditor (completed
before the spend limit hit); every line reference was spot-checked.

### 3.1 The ten claimed patterns

| # | Pattern (SPEC §2.2) | Verdict | Production-reachable | Key files |
| :-- | :--- | :--- | :--- | :--- |
| 1 | Factory | **FAITHFUL** — Simple Factory + Registry (not GoF Factory Method); method is `create`, not `createEntity`; Lakitu spawns via `EntitySpawnRequested` → `PlayingState::spawnProjectile` → factory, not directly | yes (`LevelLoader.cpp:221`, `MapGenerator.cpp` ×11, `EditorCommands.cpp:116`) | `EntityFactory.hpp:90`, `EntityFactory.cpp:31-49`, `EntityCatalogue.cpp:70+` |
| 2 | Singleton | **PARTIAL** — 12 singletons, not 4; `ResourceManager` is a deliberately leaked heap instance (27-line rationale) | yes | `Game.cpp:21`, `ResourceManager.cpp:33`, `SoundManager.cpp:15`, `AchievementManager.cpp:9` |
| 3 | State | **PARTIAL** — `IGameState` (9 concretes incl. `EditorState`; SPEC's `StatisticsState` does not exist) and `IPlayerState` faithful; `FallingPlatform`/`Thwomp` are enum state *machines*, not the pattern (`FallingPlatform.hpp:7-12`, `ProximityTriggerStrategy.hpp:13-18`) | yes | `IGameState.hpp:6-28`, `GameStateManager.hpp:7-52` (deferred push/pop/change), `IPlayerState.hpp:10-27` |
| 4 | Observer | **FAITHFUL** mechanism; **wrong participants** — 29 event types; real subscribers are `SoundManager` (20), `PlayingState` (15), `AchievementManager` (9), `StatisticsTracker`, `Camera` (7), `Minimap`; `Hud` subscribes to nothing, `ComboTracker`/`AchievementTracker` do not exist | yes | `EventBus.hpp:53-122`, `EventBus.cpp` |
| 5 | Strategy | **FAITHFUL** — 8 movement strategies (no `SwimStrategy` exists) + `IDifficultyStrategy` ×3; runtime swap exercised (`KoopaParatroopa.cpp:81`, `KoopaTroopa.cpp:52`) | yes | `IMovementStrategy.hpp`, `Enemy.hpp:111`, `DifficultyStrategy.hpp:15-57`, `Game.cpp:480-490` |
| 6 | Command | **PARTIAL** — `ICommand` has `execute(Character&)` only: no `undo`, no serialization; "console text→command" is a *second* hierarchy `IConsoleCommand` (11 concretes in `DebugConsole.cpp`); "replay serialization" is **false** — `ReplayRecorder` serializes snapshots, not commands (`ReplayRecorder.hpp:15-21`) | yes (all three) | `ICommand.hpp:5-9`, `InputManager.hpp:24,104-109`, `ConsoleCommand.hpp:17-28` |
| 7 | Decorator | **FAITHFUL** — forwarding verified (`IPlayerState.cpp:79-98`); `MegaDecorator::getSize` scales the wrapped size; `Player::setBaseState` walks to the innermost decorator so a Fire Flower survives a Star | `StarDecorator` yes (Star placed 4× in levels); **`MegaDecorator` only via console/editor** (no level places a Mega Mushroom) | `IPlayerState.hpp:94-144`, `Player.cpp:198-246` |
| 8 | Memento | **PARTIAL** — `GameSnapshot` is a public aggregate; capture is player + id/pos/vel/active per entity, not "full game state" | yes (hold `R`; F5 attract replay) | `GameSnapshot.hpp:44-60`, `TimeRewindManager.hpp:7-27`, `ReplayRecorder.hpp:25-88` |
| 9 | Object Pool | **PARTIAL** — real and used for `Fireball`, `Hammer`, `BossFireball`; **particles use their own flag-array recycling** and **no `ObjectPool<BulletBill>` exists** (SPEC §16.1 wrong on two of three) | yes (`PlayingState.cpp:288, 3883-3909`) | `ObjectPool.hpp:26-76`, `PlayingState.hpp:762-764` |
| 10 | Template Method | **FAITHFUL** — hooks exist with exactly the claimed names, `applyMovement` pure, others defaulted; `LinearStrategy` overrides one, `TetheredChaseStrategy` all three | yes | `IMovementStrategy.hpp:13-40` |

### 3.2 Claims to correct in SPEC §2.2, README and the report

12 singletons (not 4) · 9 game states with `EditorState` (no `StatisticsState`) ·
8 movement strategies, no Swim · Observer subscribers per the table above ·
Command has no undo/serialization (undo lives in `IEditorCommand`; replay is
Memento) · Object Pool covers three projectile types, not particles/Bullet Bills ·
Memento captures a partial snapshot · `Thwomp`/`FallingPlatform` are not State
participants · README's "10 movement types" still lists Swimming and Climbing,
both descoped in SPEC §21.

### 3.3 Patterns present but unclaimed — candidates for the report

| Candidate | Evidence | Report? |
| :--- | :--- | :--- |
| **Command with undo/redo** (`IEditorCommand`: `execute/undo/describe`, 7 concretes each storing its inverse; `MapEditor` paired stacks + History panel; `Ctrl+Z`/`Ctrl+Shift+Z`) | `EditorCommands.hpp:13-194`, `MapEditor.hpp:71-76,186-187`, `EditorState.cpp:365-372` | **Yes — headline**; stronger than the claimed Command |
| **Registry / Type Object** (`EntityCatalogue::Entry`: type, name, label, category, creator; collapsed four drifting lists) | `EntityCatalogue.hpp:14-91`, `verify_r21_entity_registry.cpp` | **Yes** — it is what makes the Factory open/closed |
| Data-driven Type Object (`EntityConfig` + `entities.json`, negative sentinels = "not specified") | `EntityConfig.hpp:11-46`, `EntityFactory.cpp:18-27` | one paragraph inside Factory |
| Strategy/Policy for AI (`IAIPolicy::decide(AIObservation)→AIAction`; `AIController` senses/actuates; one concrete `HeuristicPolicy`) | `IAIPolicy.hpp:102-117`, `AIController.hpp:19-48` | yes, honestly as a single-implementation seam (and the hook for §14 future work) |
| Composite (`CompositeCommand : ICommand`, jump + wall-jump on one key) | `InputManager.cpp:19-33, 56-71` | one paragraph; lives in a `.cpp`, invisible to the diagram tool |
| RAII scoped subscription (`EventBus::ScopedSubscription`) | `EventBus.hpp:79-98` | inside Observer, as the subscriber-lifetime fix |
| Adapter/port (`PlayingState::EditorBridge : IEntityAdmitter`) | `PlayingState.hpp:786-790`, `IEntityAdmitter.hpp:26-37` | one sentence |
| **Double dispatch deliberately rejected** — `EntityCategory` ordered-pair switch instead of Visitor/12 casts per pair per frame | `Entity.hpp:17-31`, `CollisionResolver.cpp:88-119` | **Yes, as a rejected alternative** — do not claim Visitor |
| Not Flyweight — `ResourceManager`/`SpriteSheet` are a shared-resource cache | `ResourceManager.hpp:64-66`, `SpriteSheet.hpp:29-37` | call it a cache |

Confirmed absent (zero hits): Visitor, Prototype, Builder, Null Object, Facade,
Chain of Responsibility, custom Iterator, Abstract Factory, Bridge.

### 3.4 Reasoning seeds for §7 (problem → naive alternative → why → cost)

- **Factory + Registry**: four hand-synced type lists had drifted to 40/40/16/16
  entries so the editor could place nothing for 24 buttons and nothing failed
  (`EntityCatalogue.hpp:19-27`). Naive: a `switch` per call site. Now: one table
  row per type, `applyConfig` from `entities.json` as a policy step no caller
  knows about. Cost: function-pointer indirection and loss of compile-time
  exhaustiveness — a forgotten enumerator yields `nullptr` (`EntityFactory.cpp:43-49`),
  hence the registry-walk test.
- **State**: pause must draw the level beneath it. Naive: `enum m_screen` +
  `switch` in three functions. Now: `IGameState` + stack, `isOverlay()`
  (`IGameState.hpp:20`), and deferred stack ops so a state can request its own
  removal (`GameStateManager.hpp:14-20, 37-44`). Cost: nine classes and a heap
  allocation per transition; four empty `IPlayerState` bodies.
- **Observer**: a coin touches HUD, sound, achievements, statistics, particles.
  Naive: `Coin` depends on five collaborators. Now: `publish(EventType, std::any)`
  (`EventBus.hpp:48-51`). Cost: control flow untraceable at the call site,
  type errors at runtime, re-entrancy engineered around.
- **Strategy**: `switch (m_enemyType)` in `Enemy::update` vs `unique_ptr<IMovementStrategy>`
  (`Enemy.hpp:111`); a stomped Paratroopa becomes a Koopa by swapping strategy.
  Second axis: `IDifficultyStrategy` turned a persisted string nothing read
  (`DifficultyStrategy.hpp:8-11`) into four consulted numbers. Cost: per-enemy
  state migrates into the strategy (`translateAnchor` had to be added).
- **Decorator**: Star/Mega are orthogonal overlays; a form enum is a
  cross-product, flags re-derive behaviour every frame. Now: wrap and forward.
  Cost: chain inspectable only by `dynamic_cast` (10 sites), `setBaseState`
  hand-walks the chain (`Player.cpp:210-223`).
- **Template Method**: eight strategies each wrote sense→move→clamp themselves
  and could clamp before moving. Now: `execute()` fixes the order. Cost: rigidity
  for a strategy needing a fourth phase.
- **Command (input)**: rebinding and a second player through a different binding
  table (`m_commandsByAction`, `InputManager.cpp:62-65`); **Command (editor)**:
  undo/redo for free because every edit stores its inverse.
- **Memento**: rewind and attract-mode replay from the same `GameSnapshot`;
  entities restored *by id* because pruning and spawning permute the list.
- **Object Pool**: projectiles created/destroyed several times a second; pooled
  and unpooled entities stored identically (`unique_ptr`) so the entity list
  never learns pooling exists. Cost: `recycleEntity` type-tests by cast.
- **Singleton**: one audio device, one texture cache, one bus; Meyers order is
  defined. Cost: 12 of them, and `Game` reached from 37 files — say so.

### 3.5 UML roots for `tools/gen_class_diagram.py`

Configured: `Entity`, `IGameState`, `ICommand`, `IMovementStrategy`,
`IPlayerState`, `IDifficultyStrategy`. **Add**: `IEditorCommand` (7 concretes),
`IAIPolicy` (`HeuristicPolicy`), `IEntityAdmitter`. `IConsoleCommand`'s 11
concretes and `CompositeCommand` live in `.cpp` files — the header scanner
cannot see them; either move the declarations to a header or draw those two
groups by hand. Association-only participants (no inheritance edge) must be
added as boxes by hand: `EntityFactory`, `EntityCatalogue::Entry`, `EventBus`,
`ScopedSubscription`, `GameStateManager`, `InputManager`, `MapEditor`,
`TimeRewindManager`, `ReplayRecorder`, `ObjectPool<T>`.

---

## 4. Documents — what is stale, wrong or missing today

### 4.1 The PDF's overlapping text (finding 5) — root cause

Two mechanisms, both in the generated LaTeX, both deterministic:

1. **Fixed table column fractions.** `reports/html_to_latex.py:352-364`
   (`_render_table`) assigns `p{}` widths by column *count*, not content: a
   3-column table gets `0.22 / 0.16 / 0.54 \linewidth`, a 5-column table
   `0.14 / 0.13 / 0.24 / 0.24 / 0.10`. The "Where" column of the §7 pattern
   table (PDF pp. 15–16) is therefore ~69 pt wide and the "Root class" column
   of §16.3 *Figures at a glance* (PDF p. 45) ~56 pt — while `IMovementStrategy`
   set in `\small\texttt` is ~89 pt. It cannot break, so it is typeset over the
   next column: that is the `IMovementStrate§6.5` collision in the user's
   screenshot; `TimeRewindManager,` over "not by index", `updateBehaviour()`
   over "new boss", `IMovementStrategy::execute` over "the rest" are the same
   defect on p. 16.
2. **`\codebreak` breaks only at punctuation.** `Report/SuperMarioGame/main.tex`
   defines `\codebreak` to insert `\allowbreak` after `/ _ . : -` only. A
   17-character identifier with none of those is one unbreakable box. The same
   macro explains most of the **20 `Overfull \hbox`** warnings in `main.log`
   (worst 68 pt) in prose paragraphs.

A third layout defect is not overlap but is on the same pages: the *full detail*
UML figures (PDF pp. 46–54; the Enemy/Boss one is a 596 × 838 pt SVG) are
scaled to fit one page and become unreadable (5–6 pt text). Page 45's
"What it shows" column (0.10 `\linewidth`) wraps one word per line for the
same fixed-fraction reason.

**Fix (Phase 5 lane 5D):** size columns from content — measure the longest
unbreakable token per column (in `\small\texttt` ≈ 5.25 pt/char) as the
column's *minimum*, distribute the remainder proportionally to prose length;
let `\codebreak` break between any two characters when a token is longer than
~10 chars (the `seqsplit` package, or emit `\allowbreak` per character inside
table cells); set tables with a code column in `\footnotesize`; put the
detailed UML figures on landscape pages (`pdflscape`) split per subtree rather
than one shrunken page, or drop the detailed variants for Enemy/IGameState and
keep compact + per-class member tables. **Guard:** `build.sh` greps
`main.log` and fails on any `Overfull \hbox` above 5 pt — a layout parity test
in the sense of g-rule-17, so this cannot come back silently.

### 4.2 Staleness list per document

Evidence: keyword greps over `reports/report_content.py` (the single prose
source) and `submission_documents/*.md`, plus the agent log for how each edition
was produced. `[STALE]` = true on 08-31, false now; `[MISSING]` = shipped,
never documented; `[WRONG]` = never true as written.

**`reports/report_content.py` → report (all editions)**
- [MISSING] Zero mentions of: attract mode (R10), the LOAD GAME picker (R8),
  surface footsteps / menu cues / combo SFX escalation (R7), console
  Tab-completion (R7), lighting shader + day/night (`LightingRenderer`,
  `radial_light.frag`), `EditorState` + CUSTOM LEVELS page + F5 playtest
  round-trip, `DebugCheats` (immortal-that-rescues, tainted runs), the
  `EntityCatalogue` registry, pipe `EntryMode` + shared `PipeRenderer::cellArt`,
  Bowser's fireball-stagger fix and lava death, the 2P survivor camera and
  eliminated badges, the save-slot picker and delete, bridge axes by
  difficulty, Lakitu's Fire Flower drop, `Entity::translate` for endless
  chunks, procedural boss arenas, the single-asset-tree guard.
- [STALE] Hand-typed numbers to recheck against `FACTS`: "504 checks" (584 at
  the last run), 23 harnesses (38 `verify_*`/guard sources; 28 ctest cases),
  "28 event types" (29), "8 screens" (9 with `EditorState`), "Eight entries" on
  the menu (LOAD GAME and CUSTOM LEVELS rows added), "13 managers".
- [STALE] §13 *What is not done* still lists items since fixed: 1-3 BGM (R1),
  inert `AnimationManager` (deleted R2), six never-placed enemy types and the
  missing hidden block (placed R9), the descoped lighting (built R21).
- [STALE] §16.2 controls: P2 fire is now `Period` (was `M`, colliding with the
  minimap toggle); P2 ground-pound `Slash`; verify every row against
  `InputManager.cpp`.
- [WRONG] §7 participant claims per §3.2 (singleton count, Observer subscribers,
  Object Pool inventory, Command serialization, Thwomp/FallingPlatform as State).
- [REWRITE] §12/§12.1 (procedure, not findings), §14 (far goals), §7 (per-pattern
  reasoning), §11 (catalogue figures) — user findings 1–4.
- [PROCESS] The `.md` edition in `submission_documents/` was produced by
  **running `pandoc` by hand** on the HTML (log 2026-08-31, "re-ran pandoc");
  nothing in `build_report.py`/`build.sh` writes it → script it so the three
  editions come from one command.

**`submission_documents/features_list.md` (97 items)**
- [MISSING] at least: attract mode; surface footsteps; per-row menu cues;
  console Tab-completion; lone-player camera clamp; star-kill/player-death
  floating overlays + the four particle types; dynamic lighting + day/night
  (the **preamble still says lighting was excluded** — now wrong); real map
  editor with custom-level save/find/play and F5 round-trip (#64 describes the
  old F1 overlay); debug cheats (immortal rescue, invincibility, noclip, time
  scale, freeze timer, hide HUD, free camera) and the tainted-run rule; pipe
  entry modes + entry animation; Bowser stagger/lava death (#43 update);
  Lakitu Fire Flower drop; 2P survivor camera + eliminated badges (#79 update);
  save-slot picker + delete (#75/#77 update); bridge axes by difficulty;
  endless boss arenas and no countdown (#96 update).
- [STALE] #9 console command count (11 → recount; `noclip` added), #68 minimap
  key (`Tab` vs the `M` fix in `1a7a6db`), #2 "8 screens".
- [DEPENDS] `testing_tasks.md` maps 97 items → extend rows for every new item.

**`Member_Contributions.md` / `.xlsx`**
- [STALE] 120 tasks / 316 commits; `dev` has **430** commits today and 39 log
  sessions (08-30 … 09-02) have no rows.
- [WRONG] The doc says both editions are "generated from one source so they
  cannot drift"; **no generator exists** in `scripts/`, `reports/` or `tools/`
  (the xlsx was written directly with openpyxl on 08-31). Phase 4C writes one.

**`AI_Usage_Declaration.md`** — [STALE] dated 08-31; Claude Code scope names
three defects + Endless Mode + documents only; extend to R1–R21, lighting,
editor, registry, attract mode, Load Game, this sweep. Antigravity attribution
verified against the log — keep.

**`demo_video_links.md`** — three `[INSERT … LINK HERE]` placeholders; the
requirements checklist (8 sections, 2026-09-01) is unchecked → Phase 3B maps
each planned video to its boxes.

**Repo-level docs**
- `README.md`: P2 fire key `M`; "9 Game States" list omits Editor; "10 Movement
  Types" still lists Swimming and Climbing (descoped, SPEC §21).
- `SPEC.md` §2.1 lists `StatisticsState` (absent) and omits `EditorState`;
  §2.2 per §3.2. SPEC is frozen: corrections need the user's sign-off (lane 2E
  item 6 proposes the text; the user applies it).
- `class_diagram.md` is generated — regenerate after Phase 2, add the new roots.
- Weekly reports: see §4.3.

### 4.3 Weekly reports — the gap

`docs/Group52_04 … _10` exist (W10 = 2026-08-09 … 08-15). Commits per day since
then: Aug 16 (1), 18 (11), 19 (48), 20 (22), 21 (1), 22 (17), 23 (1),
31 (142), Sep 1 (14), Sep 2 (19). Sunday–Saturday weeks per `REPORT_RULES.md`:

| Week | Date range | Commits | Headline content (from the log) |
| :-- | :--- | --: | :--- |
| W11 | 2026-08-16 … 08-22 | 100 | Code audit (37 findings) and its remediation packages; tiers 1–4 game completion (states, bosses, systems); Shadow Mario / 2P AI; Windows crash root-cause (deferred spawns); teammate fixes; CI made real |
| W12 | 2026-08-23 … 08-29 | 1 | Quiet week — one commit; report states so plainly rather than padding |
| W13 | 2026-08-30 … 09-05 | 175 so far | Submission package v1; level-completion/camera defects; Endless Mode + solvability oracle; the 2026-08-31 SPEC/feature audit and R1–R20 batches; R21 release batch (15 items) and wave 2 (10 items); lighting shader; real map editor; entity registry |

Each gets `52.md` + `52.pdf` (`scripts/generate_pdf.py`), five mandatory
sections, per-member/per-branch tasks with clickable paths, generated from the
log and `git log`, never from memory. Member B's share in W11–W13 is small
(5 commits by `FubuGold` since 08-16) and is reported as such.

---

## 5. Decisions taken in this plan

### 5.1 Archive, don't delete (Phase 1)

Root-level files that are scratch, superseded, or build outputs, with last
commit date. Destination `docs/archive/2026-09-02_<original-name>` plus a
`docs/archive/README.md` saying when and why (g-rule-15). Build outputs are
untracked instead (g-rule-19) — the source they derive from stays.

| File | Class | Action |
| :--- | :--- | :--- |
| `test-1.svg … test-6.svg`, `test_escaping.html` (06-20) | scratch from the class-diagram HTML experiments | archive |
| `scratch_all_frame_keys.txt` (08-11) | scratch | archive |
| `class_diagram.html` (06-19), `class_diagram_slides.pdf`, `SuperMarioGame_ClassDiagram.pdf`, `SuperMarioGame_ClassDiagram_Horizontal.pdf` (06-20) | hand-drawn diagrams superseded by `gen_class_diagram.py` (`class_diagram.md` is now generated and stays) | archive |
| `25125083.md` (07-11) original proposal/spec draft; `two_member_workflow.md` (06-09); `sprites_list.md` (08-03) | superseded planning docs | archive (FEATURE_PROPOSAL.md stays — cited by AGENTS.md and the features list) |
| `TASKS.pdf`, `TASK_DIVISION.pdf`, `SPEC.pdf`, `implementation_plan.pdf` (06/07) | stale PDF renders of tracked .md | `git rm --cached`, gitignore `*.pdf` at root; regenerate on demand |
| `submission_documents.zip` (08-31) | build output of `submission_documents/` | untrack + gitignore; rebuilt in Phase 7 |
| `docs/Group52_04.zip, _05.zip, _09.zip, _10.zip` | zips of tracked folders | untrack + gitignore |
| `docs/issues/code_audit_2026-08-18.md`, `member_a_fix_plan.md`, `member_b_input_sync.md`, `completion_plan.md`; `docs/two_player_ai_plan.md` | superseded plans/audits (all items closed) | archive; leave one-line pointers in `spec_feature_audit_2026-08-31.md` |
| `implementation_plan.md` | **repurposed** — its content is now "Standalone Enemy Behavior Test Suite" while AGENTS.md still cites it as "architecture diagrams and user answers" | keep, but fix the AGENTS.md pointer (or archive the file and point AGENTS.md at SPEC.md) — user to confirm |
| `Report/SuperMarioGame/main.{aux,log,out,toc}`, `reports/__pycache__/`, `SuperMarioGame/tools/__pycache__/` | untracked build noise | add to `.gitignore` |
| `SuperMarioGame/corpus/`, `saves/ai/`, `saves/eval/` | untracked side-project data (mapgen/RL) | leave untracked; **not** part of the submission tree |

### 5.2 Screenshots: catalogue in §11, mechanism figures in §8

A grader reading *Features demonstration* wants to see that the thing exists;
a grader reading *Implementation details* wants to see how it works. Mixing
them makes §8 a picture book and §11 a list. So §11 gets contact sheets — one
figure per family, labelled cells, captured in-game via `--script … shot NAME`
(saves to `SuperMarioGame/saves/shots/NAME.png`; `tests/scripts/report_shots.txt`
is the existing template, and the level editor / `spawn` console command
stage entities that a shipped level does not contain) — and §8 gets a single
explanatory figure only where prose cannot carry the mechanism. Harness renders
(`verify_all_entities_visual`) are acceptable *only* for the sprite-catalogue
cell of each entity and must be captioned as harness output.

Catalogue to capture (Phase 3): 4 characters × forms (Small, Super, Fire, Cape,
Mini, Star, Mega); 13 enemies + Boom Boom + Bowser (incl. lava death, stagger
HUD); 13 items; 10 blocks incl. pipe entry modes and bridge/axe; the 7 shipped
levels + 3 sub-vaults + Bonus + Endless + a custom level; every screen (Menu,
Load Game picker, Character Select, World Map, Options/rebinding, Records,
Statistics, Pause, Game Over, Victory, Editor, Custom Levels page, Procedural
page); modes (Versus Human, Versus CPU, Co-op, Shadow Chase, Endless, Daily
Challenge, Attract mode); systems (rewind banner, lighting cave + day/night,
minimap, debug console, cheats panel, achievement toast, P-Switch timer).

### 5.3 Future work (§14) scope

Only far horizons: (a) learned agents — the RL seam on `A/rl-neural-policy`
(policy interface `IAIPolicy` already in `dev`, `HeuristicPolicy` as the
baseline) and what a trained policy would need from the engine (deterministic
replay, headless stepping, reward log); (b) learned level generation — the
GAN/RL line on `A/mapgen-gan-plan` over the TheVGLC corpus, with
`LevelSolvability` as the oracle in the loop; (c) at most one engine horizon
(gamepad through the existing Command layer, or networked 2P over the replay
stream). No SPEC descope items in §14 — they belong in §13.

### 5.4 GitHub issues (Phase 7, user-gated)

`#11` (audit, 33/37 then 4/4 resolved — close with a final disposition
comment linking `spec_feature_audit_2026-08-31.md` and SPEC §22), `#16` and
`#17` (R21 merge notes — close as released, or convert to a single
"Release notes — dev @ <sha>" issue), leave none open except one new
"Submission checklist" if the user wants a public tracker. `#2` and `#9` are
already closed. The report describes the *procedure* (checkpoint audits on
2026-08-18, 08-31, 09-02; R-numbered batches; mutation-tested guards; the log
as audit trail) and its main outcomes, not the finding lists.

---

## 6. Phases, dependencies, parallelism, models

```
Phase 0  Baseline & freeze ─────────────┐
                                        ├─► Phase 2  Defect lanes (2A…2E in parallel worktrees)
Phase 1  Archive (parallel with 0/2) ───┤        │
                                        │        ▼   user merges 2A…2E into dev
                                        │   Phase 3  Evidence capture (3A shots, 3B demo scripts)
                                        │        │
                                        └──► Phase 4  Documents from truth (4A weeklies ×3, 4B features, 4C contributions, 4D AI decl)  ← needs 2 merged
                                                 │           (4A/4D may start after Phase 0; 4B/4C after Phase 2 merge)
                                                 ▼
                                            Phase 5  Report (5A patterns+OOP, 5B implementation+demo figures, 5C process/gaps/future, 5D LaTeX layout, 5E learning records ← 2E+5A) — 5D parallel from day 1
                                                 │
                                                 ▼
                                            Phase 6  Independent audit (read-only) → fix-ups → re-audit
                                                 │
                                                 ▼
                                            Phase 7  Package, tag, issues, main  (USER GATES)
```

Critical path: 0 → 2 → (3 ∥ 4B) → 5A/5B → 6 → 7. Everything else hangs off it.

### Phase 0 — Baseline & freeze — 1 agent, **Sonnet 5, effort medium**, ~1 h

Depends on: nothing. Blocks: 2, 3, 4.
> Read AGENTS.md. `git fetch --all`; confirm `dev == origin/dev`. Fresh
> configure+build of `dev` in a clean build dir; run `ctest` (expect 28/28) and
> `verify_regressions` (expect 584/584); launch the game with
> `--script tests/scripts/verify_r19_smoke.txt` and confirm it reaches the menu.
> Record every number and the HEAD hash in the log. (The
> `LevelSolvability` vacuous-truth item from `release_batch_r21_2026-09-02.md` §5
> is already closed: `081c4d6` makes `groundRowAt` skip the ceiling run and
> moves the BFS goal to `width-3` — `LevelSolvability.cpp:36-49`,
> `MapGenerator.cpp:658`; verify with `verify_map_generator`, do not re-fix.)
> Do not commit `saves/config.json`. Output: a "baseline" log entry that every later lane
> cites as its starting fingerprint.

### Phase 1 — Archive older artifacts — 1 agent, **Haiku 4.5, effort medium**, ~1 h — branch `A/chore/archive-2026-09-02`

Depends on: nothing (runs alongside 0 and 2; touches no source). Blocks: 7.
> Read AGENTS.md §g-rule-15/19. Apply §5.1 of this plan exactly: `git mv` the
> listed scratch/superseded files to `docs/archive/2026-09-02_<name>`; write
> `docs/archive/README.md` (what moved, from where, why, on what date);
> `git rm --cached` the listed build outputs and extend `.gitignore` (root
> `*.pdf` except `submission_documents/`, `*.zip`, `Report/SuperMarioGame/main.{aux,log,out,toc}`,
> `__pycache__/`). Grep the repo for every path you moved (`README.md`,
> `AGENTS.md`, `TASKS.md`, `docs/`, `reports/`) and update links. Do NOT touch
> `implementation_plan.md` until the user answers §5.1's question. Build once to
> prove nothing referenced a moved file. Commit per group; no push.

### Phase 2 — Defect lanes — 5 parallel worktrees, each its own deps dir

Depends on: Phase 0. Blocks: 3, 4B, 5B. Every lane: fix + regression case
(mutation-tested) + observed in a scripted run + log entry. Lanes touch
disjoint files except where noted; the user merges in the order 2A → 2E.

| Lane | Item | Files | Model / effort | Size |
| :-- | :--- | :--- | :--- | :-- |
| 2A | `MovingPlatform::update()` never calls `Block::update()` (animator frozen); `Spiny::isEgg` unreachable — Lakitu should drop an egg that hatches on landing (SPEC §6.1) | `MovingPlatform.cpp`, `Spiny.cpp`, `Lakitu.cpp` | Sonnet 5 / medium | S |
| 2B | Third HARD axe stands behind the arena enclosure post at (189,19) — unreachable by ordinary movement; `MapGenerator`'s generated vault pipe is still `Top` entry, so the up/down pipe story is inconsistent in generated levels | `assets/levels/level_3.json`, `MapGenerator.cpp` | Sonnet 5 / medium | S |
| 2C | A playtest ended by Game Over goes through `GameOverState` instead of popping back to the editor | `EditorState.cpp`, `GameOverState.cpp`, `PlayingState.cpp` (small) | Sonnet 5 / medium | S |
| 2D | Endless: no distance gate on entity updates (long runs update every chunk's entities); lighting tunables have no ImGui sliders; `UiRenderer::wrapText` has no production caller — wire (Load Game summary, editor chrome) or delete | `PlayingState.cpp` (update loop), `DevPanel.cpp`, `UiRenderer.cpp` | Opus 5 / medium | M |
| 2E | **Cheap OOP fixes** — exactly the six items in §2.3 (DevPanel spawners via the factory, `spawnProjectile` switch, `recycleEntity` casts, `Camera::getView`, `SoundManager` scoped subscriptions, participant-list corrections); everything else in §2.2 is a documented trade-off, not a refactor this week; keep before/after excerpts for lane 5E's learning record | `DevPanel.cpp`, `PlayingState.cpp` (two functions), `Camera.hpp/.cpp`, `SoundManager.cpp`, `Entity.hpp` | Opus 5 / high | M |
| ~~2F~~ | ~~`LevelSolvability` vacuous truth~~ — **closed before this plan**: fixed in `081c4d6` (ceiling run skipped, goal column `width-3`); Phase 0 only confirms | — | — | — |

Not fixable this week, stated as such in §13: D14 Windows platform claim
(no Windows environment/CI), D27 death-SFX (not demonstrable as worded),
`profile.json` counters inflated by agent playtests (user data; user's call).

### Phase 3 — Evidence capture — 2 lanes in parallel, after Phase 2 is merged

3A **Screenshot catalogue** — **Sonnet 5, effort medium**, ~3 h — branch `A/docs/report-screenshots`
> Read §5.2. Write `tests/scripts/report_shots_*.txt` (one per family) using
> `wait/press/hold/shot/quit`, staging editor-only entities through the editor
> or the `spawn` console command; run each with `./build/SuperMarioGame --script …`;
> assemble contact sheets with Python (Pillow) into `reports/assets/<family>.png`
> with labelled cells; register each in `report_content.py` via `img()` with a
> caption that says what is shown and how it was captured. Every cell must be
> from the running game except the sprite-catalogue cells, which are captioned
> as harness output. Verify by opening each PNG (Read tool) before committing.

3B **Demo video scripts + shot lists** — **Sonnet 5, effort medium**, ~2 h — same branch
> `demo_video_requirements.md` lists 8 sections. Produce, per planned video
> (full playthrough 1-1→1-3 incl. sub-vaults and both bosses; feature showcase:
> engine/editor/multiplayer/endless/lighting/rewind/cheats; mechanics: power-ups,
> all entities, blocks, items), a deterministic `--script` that drives the run
> and a minute-by-minute shot list mapping to the checklist boxes. Recording is
> the user's: run the script while capturing with QuickTime/OBS (or
> `screencapture -V <seconds>` on macOS). Leave the three link slots in
> `demo_video_links.md` for the user; add the checklist mapping under each.

### Phase 4 — Documents from today's truth — 4 lanes, parallel, disjoint files

4A **Weekly reports W11, W12, W13** — three agents in parallel, **Sonnet 5, effort medium**, ~1.5 h each — branch `A/docs/weeklies-w11-w13`
> Read `docs/REPORT_RULES.md` and `docs/Group52_10/52.md` as the template.
> Run the four pre-flight commands. Build the week from
> `git log --since=<Sun> --until=<Sat 23:59> --format='%h %ad %an %s'` and the
> `logs/agent_history.log` entries in that range; group by member and branch;
> clickable paths; Issues & Resolutions written as problem/root cause/exact
> fix. W12 has one commit — say so. Render `52.pdf` with `scripts/generate_pdf.py`.
> Can start after Phase 0 (history does not change).

4B **Features list, exhaustive** — **Opus 5, effort high**, ~3 h — branch `A/docs/features-list-v3` — after Phase 2 merge
> Read §4.2's list of shipped-but-unlisted features and the R7–R21 log entries.
> For each candidate: find the production call path from `main()` (not a
> harness), then add it with the same voice as the existing 97 items; correct
> any existing item the code now contradicts (e.g. #64 editor, #68 minimap key,
> #75 save slots, #76 auto-save, lighting exclusion in the preamble). Number
> continuously; keep the "scope note for graders". Then update
> `testing_tasks.md` so every new item has a manual test row. The rubric is
> 0.25 pt per feature and the group carries a double allocation: completeness
> is the grade — but an unreachable claim costs more than a missing one.

4C **Member contributions** — **Sonnet 5, effort medium**, ~1.5 h — same branch as 4B or its own
> Extend the task table from #121 with the 2026-08-31 … 09-02 sessions (one row
> per log entry's headline, hours estimated the way rows 106–120 were); refresh
> commit counts from `git shortlog -sn` (430 on `dev` today vs 316 in the doc);
> regenerate `Member_Contributions.xlsx` and `.md` from one source — **there is no
> generator today** (verified: none in `scripts/`, `reports/`, `tools/`; the
> xlsx was hand-written with openpyxl on 08-31), so write
> `scripts/build_contributions.py` (openpyxl 3.1.5 is installed) and make the
> doc's "generated from one source" sentence true.
> Keep the 0.5/0.5 percent convention and the honesty note on the Git % gap.

4D **AI Usage Declaration** — **Haiku 4.5, effort low**, ~20 min
> Extend the Claude Code scope: R1–R21 defect batches, lighting shader, editor,
> registry, attract mode, Load Game, this document set; date it; keep the
> Antigravity attribution as is (verified against the log). Regenerate the PDF.

### Phase 5 — The report — 5 lanes; 5D from day 1, 5A–5C after 3A and 4B, 5E after 2E + 5A

All prose lives in `reports/report_content.py`; the PDF/HTML/MD are generated
(`reports/build_report.py`, `Report/SuperMarioGame/build.sh`). Lanes 5A–5C
edit disjoint sections of one file → run them **sequentially in one worktree**
or as three agents on three branches with a trivial merge; 5D edits
`html_to_latex.py`/`main.tex` only and runs in parallel.

5A **Design reasoning (§6 OOP, §7 patterns)** — **Fable 5.1, effort high**, ~4 h
> Rewrite §7 as one subsection per pattern (10 claimed + the unclaimed ones §3
> says deserve a place): the problem in this codebase, the naive alternative it
> replaced (as in the existing §7.1, extended to all), why this pattern, what it
> cost (the honest trade-off), participants named, and a pointer to the figure.
> Add a §6 SOLID walk-through: one paragraph per principle with the strongest
> evidence from §2's strengths and the documented trade-offs (friend classes,
> singletons, `PlayingState` size) stated as decisions with reasons. Every
> sentence grounded in a real identifier; no claim §2/§3 rated PARTIAL is stated
> as full.

5B **Implementation + demonstration (§8, §11)** — **Opus 5, effort high**, ~3 h
> Add the §8 subsections for what shipped since 08-31 (lighting renderer,
> EditorState + custom levels, entity registry, pipe EntryMode + shared
> `cellArt`, Bowser stagger and lava death, 2P survivor camera, save slots,
> DebugCheats/immortal rescue, endless chunk translation via `Entity::translate`);
> place Phase 3A's catalogue figures in §11 and the mechanism figures in §8 per
> §5.2; refresh every count through `FACTS`.

5C **Process, gaps, future (§12, §13, §14)** — **Opus 5, effort medium**, ~2 h
> §12: the procedure — three checkpoint audits (08-18, 08-31, 09-02), R-numbered
> batches, mutation-tested guards, the log as audit trail, the AGENTS.md rules
> that came out of failures — with main outcomes in one table, no finding lists.
> §13: today's honest gaps (Phase 2's not-fixable list; anything §2/§3 leaves as
> a trade-off). §14 per §5.3.

5D **LaTeX layout fix + guard** — **Sonnet 5, effort high**, ~2 h — branch `A/docs/report-layout`
> Implement §4.1's fix in `reports/html_to_latex.py` (content-measured column
> widths; per-character break in code tokens > 10 chars inside cells;
> `\footnotesize` for code-bearing tables) and `main.tex` (`seqsplit` or the
> per-char `\allowbreak`; `pdflscape` for detailed UML); split or drop the
> unreadable detailed UML pages; add the `Overfull \hbox` > 5 pt guard to
> `build.sh`. Verify by rendering pages 15, 16, 45–54 to PNG and looking at
> them, and by the guard passing. This is the one lane whose acceptance is
> visual.

5E **Learning records for OOP/SOLID and design patterns (g-rule-21)** — **Opus 5, effort high**, ~4 h — branch `A/docs/learning-oop-and-patterns` — after 2E is merged and 5A's reasoning is drafted (both feed it; 5E and 5A may run as one agent sequentially)
> Produce two records, each from the skeleton generator, each stamped against
> the merged `dev` commit: (1) `docs/learning/design-patterns-in-supermario.html`
> — one section per pattern in §3.1 plus the unclaimed ones in §3.3 the report
> adopts, with the rejected-alternative table per pattern (§3.4 seeds), the
> participant diagram as **Mermaid source** (not a pasted PNG; the SVGs from
> `gen_class_diagram.py` may be embedded inline as text), load-bearing excerpts
> extracted by `tools/extract_learning_excerpts.py` (e.g. `IMovementStrategy::execute`,
> `PlayerStateDecorator` forwarding, `EntityCatalogue::Entry`, `EventBus::ScopedSubscription`,
> `GameStateManager` deferred ops) with anchors asserted by `--check`, and a
> worked trace (a Fire Mario picks up a Star: which decorator wraps what, which
> events fire, who subscribes); (2) `docs/learning/oop-solid-design-decisions.html`
> — the four OOP attributes and five SOLID principles as they stand in this
> codebase: §2.1 strengths with excerpts, §2.2 trade-offs (friends, 12
> singletons, `PlayingState` size, decorator casts, public `GameSnapshot`) each
> with the alternative considered and why it lost, the lane 2E fixes as
> before/after excerpts, pitfalls (the R5 cast refactor, the R21 dangling
> `m_player` use-after-free), self-check questions and extension exercises
> ("add a new enemy", "add a new power-up overlay"). Rebuild `index.html` via
> `--index`. The report's §6/§7 link these records rather than duplicating them
> (g-rule-22); Phase 6 verifies both exist, `--check` passes, and the stamp
> equals the audited commit.

Then one build: `Report/SuperMarioGame/build.sh` → PDF; `build_report.py` →
HTML; the `.md` edition — which on 08-31 was made by **running `pandoc` on the
HTML by hand** (agent log, "re-ran pandoc") — gets a scripted step in
`build.sh` so all three editions come from one command and the two
`submission_documents/` copies are refreshed by it, not by hand.

### Phase 6 — Independent audit — 1 read-only agent, **Fable 5.1, effort high**, ~3 h, then fix-ups by the lane owners, then re-audit (**Opus 5, medium**)

Depends on: Phase 5 build. Blocks: 7.
> You did not write any of this. `git fetch --all`. For every factual claim in
> the generated report and the features list, find the code or commit that
> supports it; list every claim you cannot support. Render every PDF page to
> PNG and inspect for overlap, clipping, illegible figures, empty pages;
> confirm the `Overfull` guard ran. Check every link and path resolves; confirm
> no placeholder text remains except the three video links; confirm counts in
> the prose equal `FACTS`. Confirm the two g-rule-21 learning records exist,
> `tools/extract_learning_excerpts.py --check` passes on them, their
> "verified against commit" stamp is the audited `dev` commit, and
> `docs/learning/index.html` lists them. Write `docs/issues/submission_audit_<date>.md` (the
> one current audit doc) with PASS/FAIL per check and a fix list; log entry.

### Phase 7 — Package and hygiene — **Haiku 4.5, effort low** for the mechanics; **every outward action is the user's**

Depends on: 6 PASS.
- Rebuild `submission_documents/` (all `.md` + `.pdf`), rebuild the zip locally (untracked), verify the zip opens and lists the six deliverables: AI declaration, demo links, report, features list, member contributions, plus the source tree reference.
- **USER**: paste video links; approve issue closures (§5.4) and the comment text; decide `implementation_plan.md`; fast-forward `main` to `dev` and tag (e.g. `v1.1-submission`); push.
- Final log entry with fingerprints before/after.

### Effort budget (agent-hours, rough)

| Phase | Lanes | Parallel width | Wall-clock if parallel |
| :-- | :-- | :-- | :-- |
| 0 | 1 | 1 | 1 h |
| 1 | 1 | with 0/2 | 1 h |
| 2 | 5 | 5 | 2–3 h |
| 3 | 2 | 2 | 3 h |
| 4 | 4 (+2 for weeklies) | 6 | 3 h |
| 5 | 5 | 2 (5D ∥ 5A→5E→5B→5C) | 8–10 h |
| 6 | 1 (+ fix-ups) | 1 | 4 h |
| 7 | 1 + user | 1 | 1 h |

---

## 7. Open questions for the user (answer before the sweep starts)

1. `implementation_plan.md`: archive it and repoint AGENTS.md at SPEC.md, or keep?
2. GitHub issues: close #11/#16/#17 as proposed in §5.4, or replace with one release-notes issue?
3. `profile.json` lifetime counters inflated by agent playtests — reset from the earliest snapshot, or leave?
4. Demo videos: will you record while the Phase 3B scripts drive the game, or should an agent attempt `screencapture -V` recordings for you to review?
5. Detailed UML in the appendix: landscape split pages, or drop the detailed variants for the two largest trees?

---

## 8. Amendments — 2026-09-02 sweep execution (orchestrator)

### 8.1 The five open questions, answered by the user

| # | Question | User's answer | Where it lands |
| :-- | :--- | :--- | :--- |
| 1 | `implementation_plan.md` — archive or keep? | **Archive it, and repoint AGENTS.md at `SPEC.md`.** | Phase 1 lane: `git mv` → `docs/archive/2026-09-02_implementation_plan.md`; AGENTS.md Key Files pointer and g-rule-6's plan-file list repointed at `SPEC.md` / `TASKS.md`; rationale recorded in `docs/archive/README.md` (the file's content had been repurposed to "Standalone Enemy Behavior Test Suite" and no longer matched the role AGENTS.md cited) |
| 2 | GitHub issues — close #11/#16/#17, or one release-notes issue? | **Close them, as proposed in §5.4.** | Phase 7. Still user-gated at the posting step: the closure comment text is drafted for approval before anything is posted, because posting is outward-facing and public |
| 3 | `profile.json` inflated counters — reset or leave? | **Reset, and gitignore it — it is per-installation data, new for each install.** | Phase 1 lane: back up outside the repo first, reset the counters to the earliest committed snapshot, `git rm --cached`, add to `.gitignore`. Whether `config.json` / `progress.json` should follow is reported as a recommendation, not acted on |
| 4 | Demo videos — user records, or an agent attempts `screencapture -V`? | **Links added later.** | Phase 3B still produces the deterministic `--script` drivers and the minute-by-minute shot lists mapped to the checklist boxes; the three `[INSERT … LINK HERE]` slots in `demo_video_links.md` stay for the user. No agent attempts a recording |
| 5 | Detailed UML — landscape split pages, or drop the detailed variants? | **Try both and pick the best.** | Phase 5D implements both treatments for the two largest trees (Enemy/Boss, IGameState), renders both, measures the resulting font size in points, ships the legible one — preferring landscape split where it is legible, since it keeps the information — and reports the comparison so the user can overrule |

### 8.2 Defect found before the sweep started: the machine cannot host the planned parallelism

**Symptom.** §6's Phase 2 asks for five concurrent worktrees, "**one `FETCHCONTENT_BASE_DIR` per lane**", after the R21 batch corrupted a shared `build/_deps`. Measured on this machine at sweep start: **2.7–3.3 GiB free** (`df -h /`; 245.1 GB container, 2.9 GB free per `diskutil info /`), against **748 MB per full build** (`SuperMarioGame/build`, of which `_deps` is 405 MB: 327 MB of fetched sources, 75 MB of dependency build trees, plus 124 MB of project objects and ~219 MB of binaries). Five independent deps dirs plus five object trees is ~4.3 GB. **The plan as written does not fit.**

**Ruled out as the cause.** The user's hypothesis was leftover R21 lane worktrees still holding builds. Verified false: `git worktree list` showed a single tree, `.git/worktrees` did not exist, and a `find` over `$HOME` for `_deps` returned exactly one build directory for this project (the other is an unrelated CS163 project, 527 MB). The R21 worktrees were already cleaned. The disk is full for unrelated reasons — `~/Library` 35 GB, `~/Lecturing_Source` 3.7 GB, `~/Downloads` 1.2 GB — and the three APFS snapshots present are OS-update snapshots, not thinnable Time Machine locals.

**Fix adopted — APFS copy-on-write clones.** Measured on this volume: `cp -Rc` of a 200 MB tree cost **4 KB** of real disk. So each lane still gets a genuinely private `FETCHCONTENT_BASE_DIR` — satisfying the R21 lesson exactly, since nothing is shared and no concurrent reconfigure can corrupt a sibling — while the 327 MB of fetched dependency *sources* are physically shared until a lane writes to them:

1. Phase 0 builds once in the existing `SuperMarioGame/build` (net-zero disk: its stale project objects are removed, `_deps` is preserved so the fetch is not repeated).
2. Phase 0 then clones `build/_deps` → `smg-lanes/_deps_template` at ~0 cost.
3. Each Phase 2 lane clones the template into its own build dir and points `FETCHCONTENT_BASE_DIR` at that clone. Real per-lane cost falls from ~750 MB to ~350 MB (its own objects and binaries only).

**Consequent change to Phase 2's width.** Even at ~350 MB per lane, five concurrent lanes is ~1.75 GB against ~3 GB free, with a full disk risking exactly the kind of corrupt-artifact failure the R21 incident produced. Phase 2 therefore runs in **two waves — 2A + 2B + 2C (all size S), then 2D + 2E (both size M)** — with free space checked between waves and each wave's build directory released before the next starts. Lane isolation, per-lane deps, and the merge order 2A → 2E are unchanged; only the concurrency width is reduced. Wall-clock cost of the split is roughly one extra build cycle.

**Standing recommendation to the user (not acted on):** freeing ~5 GB would let Phase 2 run at the planned width 5 and remove the risk of a mid-build disk-full failure. The cheapest candidates are `~/Downloads` (1.2 GB) and the unrelated CS163 build directory (527 MB, regenerable). Nothing was deleted.

### 8.3 Wave 1 as dispatched

Six lanes launched concurrently, none of which needs a C++ build slot, so they run alongside Phase 0's exclusive use of the one build directory:

| Lane | Branch | Worktree | Model |
| :-- | :--- | :--- | :--- |
| Phase 0 baseline | `dev` (read-only; log entry left uncommitted) | main tree | Sonnet 5 |
| Phase 1 archive (+ Q1, Q3) | `A/chore/archive-2026-09-02` | `smg-lanes/archive` | Sonnet 5 |
| Phase 5D LaTeX layout (+ Q5) | `A/docs/report-layout` | `smg-lanes/layout` | Sonnet 5 |
| Phase 4A weekly W11 | `A/docs/weekly-w11` | `smg-lanes/w11` | Sonnet 5 |
| Phase 4A weeklies W12 + W13 | `A/docs/weekly-w12-w13` | `smg-lanes/w12w13` | Sonnet 5 |
| Phase 4D AI declaration | `A/docs/ai-declaration` | `smg-lanes/aidecl` | Haiku 4.5 |

Phase 1 was raised from the planned Haiku 4.5 to Sonnet 5 because the user's answers to Q1 and Q3 added judgement to what was a mechanical lane: repointing a rule's plan-file reference, and a reversible reset of user data.

Deviation from §6's model table, recorded per g-rule-6: the weeklies were split as W11 alone and W12+W13 together rather than three separate agents, because W12 is a single-commit week and three agents on one branch would have raced on one git index.

### 8.4 Checkpoints

Gates where the orchestrator stops, verifies, and reports before anything proceeds:

| # | Where | Why it is a gate |
| :-- | :--- | :--- |
| C0 | Before wave 1 | Repo state, disk, worktree audit — **passed**, with §8.2 as its finding |
| C1 | After wave 1, before Phase 2 | Six branches to verify; the archive lane's `git mv` / `git rm --cached` / `.gitignore` set and the `profile.json` reset are the first changes with any reach beyond one file. Merge into `dev` is the user's, per g-rule-7 |
| C2 | After Phase 2 wave A (2A/2B/2C), before wave B | Disk headroom re-checked; three source branches audited; mutation tests confirmed |
| C3 | After Phase 2 wave B, before Phase 3/4 | Phase 3 and 4B/4C read a merged `dev`, so the merge must happen here — user-gated |
| C4 | After Phase 5 build, before Phase 6 | The report is the deliverable; Phase 6 must audit a frozen artifact |
| C5 | Before every Phase 7 outward action | Irreversible or public: the GitHub issue comments and closures, the `main` fast-forward, the tag, the push. Each is the user's call individually — approval of one is not approval of the next |

### 8.5 Phase 0 baseline — measured, and independently re-verified by the orchestrator

`dev` @ `6af4f8e7e8cf5a362d26d5d05dc12cdbc3c6a962` == `origin/dev` (`0 0`), fetched 18:53.

| Check | Plan expected | Measured | Orchestrator re-ran |
| :--- | :--- | :--- | :--- |
| `ctest` | 28/28 | **29/29 passed** | yes — 29/29, 0 failed |
| `verify_regressions` | 584/584 | **584/584, ALL PASS** | yes — 584/584 |
| `verify_map_generator` | pass | 8/8, incl. "ceilinged maps are actually checked now" | no — trusted the lane |
| Scripted run `verify_r19_smoke.txt` | reaches menu | exit 0; `MenuState → CharacterSelectState → WorldMapState → PlayingState` (1-1); a real rendered frame inspected | no — trusted the lane |
| Build | fresh configure | exit 0, 54 targets; re-run idempotent (no recompilation) | no |

**The 28 → 29 discrepancy is explained, not adjusted.** HEAD `6af4f8e` ("a flagpole touched during a boss fight left the level unfinishable") itself adds `tests/verify_r21_flagpole_softlock.cpp` as ctest case 29, and it landed after the plan's 28/28 figure was written earlier the same day. **29/29 is the correct baseline for this HEAD**, and every later lane cites 29, not 28.

Deps template created at `smg-lanes/_deps_template`, 405 MB, confirmed a real APFS clone (distinct inodes, identical content) at ~0 disk cost. Save data was **not** polluted by the baseline playtest: `git status` shows only the pre-existing `saves/config.json` modification.

### 8.6 Defect: the Q3 `profile.json` instruction was built on a false premise

**The orchestrator's error, recorded per g-rule-5.** The Phase 1 lane was told to reset `saves/profile.json` to "the earliest committed snapshot", `git rm --cached` it, and add it to `.gitignore`. All three are impossible or redundant:

- `git log --all --follow -- saves/profile.json` returns **nothing** — the file has never been tracked, so there is no snapshot to restore and nothing to un-cache.
- `.gitignore:6` already contains `saves/`. **The gitignore half of the user's Q3 answer was already satisfied before the sweep began.**
- `git ls-files saves/` returns exactly one path, `saves/config.json` — tracked past the ignore rule, and the file holding the user's local `debugMode` change.

The lane was stopped and told to revert anything it had done on this account. Plan §5.1's premise that these are simply "inflated by agent playtests" is also wrong, which is the substantive finding:

| Field | Current `saves/profile.json` | Backup at 14:48 today (`saves_backup_lane_n/`) |
| :--- | :--- | :--- |
| achievements | 7 | **9** — additionally `100_coins`, `combo_king` |
| `highestCombo` | 2 | **8** |
| `totalCoinsCollected` | 498 | 420 |
| `totalDeaths` | 100 | 139 |
| `totalEnemiesDefeated` | 133 | 104 |
| `totalTimePlayed` | 4730.68 | 4412.41 |

The current file is **not a superset** of the backup: it has *lost* two achievements and a `highestCombo` of 8 while gaining coins, kills and playtime. So an agent playtest did not merely inflate the counters — something overwrote the file from a partial state and destroyed genuine records. Which state to keep is the user's call (§8.7), not a lane's.

**Recommendation carried to the user, not acted on:** `saves/config.json` is tracked while `saves/` is ignored. By the same reasoning the user applied to `profile.json` — per-installation data — it arguably belongs untracked too; it is also the reason `git status` has shown a permanent phantom modification all sweep. Left alone because it is the one save file with committed history.

### 8.7 Lane acceptance record (orchestrator verification)

"Re-verified" means the orchestrator independently re-ran the lane's own measurement rather than trusting its report.

| Lane | Branch | Status | Orchestrator's verification | Rework required |
| :-- | :--- | :--- | :--- | :--- |
| Phase 0 baseline | `dev` (no commits) | **accepted** | Re-ran `ctest` → 29/29; `verify_regressions` → 584/584; confirmed deps template 405 MB and that no save file was polluted | Resumed once — it stopped while a build was still pending, and was told to prove the build succeeded rather than infer it from the binary's existence |
| Phase 4A W11 | `A/docs/weekly-w11` | **accepted** | Re-ran the range count → 100 commits, 98 / 2 author split; confirmed 5 mandatory sections and a clean tree | none |
| Phase 4A W12+W13 | `A/docs/weekly-w12-w13` | **accepted** | Re-ran both range counts → 1 and 175; W13 author split 172 / 3; sections and clean tree confirmed | none |
| Phase 4D AI declaration | `A/docs/ai-declaration` | **accepted** | Read the final paragraph verbatim against the log's dates | **three rounds** — see §8.8 |
| Phase 1 archive | `A/chore/archive-2026-09-02` | running | — | scope corrected twice (§8.6, and the user's §8.9 decisions) |
| Phase 5D layout | `A/docs/report-layout` | running | — | — |
| Phase 2 2A/2B/2C/2D | four `A/fix/…` branches | running | — | — |
| Phase 2 2E | not yet branched | **held deliberately** | — | overlaps 2D in `PlayingState.cpp` and `DevPanel.cpp`; will branch off 2D's tip so the user is not handed the conflict |

Two lanes reported something the plan got wrong and were right to: W11's Member B share is **2** commits in that week, not the plan's "5 since 08-16" (which spans into W13); and W12's single commit is a *merge* whose 18 underlying commits are dated inside W11, so the week is genuinely empty of new work.

### 8.8 Lane 4D — three rounds of rework, recorded because two errors were factual

Lane 4D wrote the AI Usage Declaration, a document the students sign. It required three corrections:

1. **Invented counts.** It wrote "the `verify_*` regression test suite (38 cases covering 504+ checks)". `504` is listed `[STALE]` in §4.2 (584 measured); `38` is the count of harness *source files*, not registered ctest cases. The lane had no build slot and could measure neither. Per §1 rule 7 ("counts are computed, not typed") the number was **deleted**, not re-guessed.
2. **Self-contradicting dates.** "The final two weeks of development (from 2026-08-31 onward)" — three days, not two weeks. Corrected to 2026-08-18, the first Claude Code session in the log.
3. **Over-correction.** Told to fix the introduction date, it also shifted the *R-batch* span to Aug 18. The R-batches ran 2026-08-31 → 09-02 (first execution entry `[2026-08-31 08:45:00]`); the original "Aug 31 – Sep 2" was right. Both spans now appear correctly and distinctly in one sentence.

It also appended its log entry to the **main tree's** log instead of its worktree, leaving its own branch without one (g-rule-5). Remedy: add the entry to the branch, and **do not** revert the main-tree append — `agent_history.log` is append-only and union-merged, so a duplicate is harmless while a deletion destroys history.

Lesson for later phases, and the reason Phase 6's independent audit is not optional: a lane with no way to measure a number will still write one down. **Every count in a generated document must come from `FACTS`, never from a lane's prose.**

### 8.9 The user's two save-file decisions (2026-09-02)

| File | Decision | Execution |
| :--- | :--- | :--- |
| `saves/profile.json` | **Zero it to fresh-install state** — empty `achievements`, all `statistics` at 0 | Phase 1 lane, in the MAIN tree (it is untracked on-disk user data, not repo content). Pre-reset backup taken; `saves_backup_lane_n/profile.json` preserved untouched as the only copy of the lost `100_coins` / `combo_king` achievements and `highestCombo: 8` |
| `saves/config.json` | **Untrack it**; `.gitignore:6`'s `saves/` already covers it | `git rm --cached` on the Phase 1 branch — **gated**: the lane must first prove from the load path that the game supplies defaults when the file is ABSENT. If it cannot, it must NOT commit the untrack and must report a blocker instead. A submission that fails on a grader's fresh clone is worse than a phantom `git status` entry |

### 8.10 GitHub issue state, verified read-only for Phase 7

`gh issue list --state all` at 2026-09-02 confirms §5.4 exactly: **#17** open (R21 wave 2), **#16** open (R21 release batch), **#11** open (code audit, 37 findings), **#9** and **#2** already closed. Nothing has been posted, commented or closed — every outward action waits at checkpoint C5 for the user, per §1 rule 9. Note #16's title records "23/23 ctest" and #17's "28/28"; the count is now 29/29 (§8.5), so any closure comment should state the current figure rather than repeat a historical one.

### 8.11 Phase 1 archive — accepted, after one rules violation was caught

`A/chore/archive-2026-09-02`, tip `9d27c98`, 10 commits (one per group, g-rule-12), clean tree, not pushed.

**Verified by the orchestrator, not taken on report:**
- 20 files `git mv`'d to `docs/archive/2026-09-02_*` with `docs/archive/README.md` indexing what moved, from where and why. Nothing deleted (g-rule-15).
- `.gitignore` anchored as `/*.pdf`, not `*.pdf` — so the 6 tracked `submission_documents/*.pdf` and 7 tracked `docs/Group52_*/52.pdf` deliverables all survive. `git ls-files -i -c --exclude-standard` is empty: no tracked file contradicts an ignore rule.
- **The `config.json` untracking gate passed on real evidence.** `Serializer::loadSettings` (`SuperMarioGame/src/Utils/Serializer.cpp`) has an explicit `if (!std::filesystem::exists(path))` branch that sets every setting to a default and returns `true`. A grader's fresh clone gets working settings. Code read directly by the orchestrator.
- `saves/profile.json` zeroed in the main tree per §8.9; three backups exist, and `saves_backup_lane_n/profile.json` still holds the 9 achievements and `highestCombo: 8`.

**Violation caught in review: the lane edited a generated, AgentHub-synced block.** Its g-rule-6 fix landed at `AGENTS.md` lines 408 and 497, both inside `<!-- AGENTHUB:L3:BEGIN — generated, do not edit by hand -->` (lines 396–612). Two problems: g-rule-18 forbids forking an upstream Layer 3 rule in a project, and the next AgentHub sync would regenerate the block, silently reverting the fix so that `AGENTS.md` again cites a file that had been archived — **a fix that evaporates with nothing failing.** Remedy: both hunks reverted byte-for-byte (verified: `git diff dev -- AGENTS.md` now shows only two hunks, at lines 17 and 222, both outside the block); the project-owned Key Files pointer at line 20 kept, since that is what actually answers Q1; and the rule change recorded as an upstream suggestion in `.agenthub/suggestions.json` with `requestedDisposition: defer`, without touching the AgentHub repo.

**Residual, stated honestly rather than hidden:** this project's `AGENTS.md` still literally names `implementation_plan.md` inside g-rule-6's generated text. That is now an upstream request, not a local edit.

### 8.12 Plan defect found by a lane: `docs/two_player_ai_plan.md` must not be archived

§5.1 lists it as a "superseded plan (all items closed)". It is not superseded: **7 live engine sources cite it by section number** — `GameMode.hpp`, `DevPanel.hpp`, `HeuristicPolicy.hpp`, `AIController.hpp`, `IAIPolicy.hpp`, `ShadowMario.hpp`, `verify_multiplayer_ai.cpp`. The lane correctly refused the instruction and left the file in place. §5.1's row is **withdrawn**; the file stays current. Archiving it would have orphaned seven source comments.

### 8.13 Orchestrator mistake: the APFS deps clone bakes in absolute paths

§8.2's `cp -Rc` seeding gave each lane a private `build/_deps` at ~0 disk cost, which solved the space problem. What it did not anticipate: FetchContent's **subbuild `CMakeCache.txt` files store the absolute path they were generated at**, so a cloned tree points every dependency subbuild at the directory it was cloned *from* — the root repo's build dir. Lane 2A hit a CMake configure error on this and repaired it by substituting the old path prefix for the new one across the **453** affected files, with no deletion and no re-fetch. Correct response, and the finding is recorded in its log entry because three sibling lanes carry the same seeded clone.

Recorded as the orchestrator's error, not a lane's. The clone is still the right call — it is what made five concurrent private dependency trees fit — but a lane receiving one must expect to rewrite the baked paths. A cleaner seeding would clone only the `*-src` directories (327 MB, never written during a build) and let each lane generate its own subbuild trees (75 MB), which carry no stale absolute paths.

### 8.14 Lane 2A — accepted pending one verification gap

`A/fix/moving-platform-and-spiny-egg`, 4 commits. **ctest 30/30** (29 baseline + 1 new), `verify_regressions` 584/584 unchanged. Mutation-tested in both directions for both fixes, observed in a real scripted run.

- **`MovingPlatform::update()`** fully overrode `Block::update()` and never called back into it — the only place `m_animator` advances — so motion and player-carrying worked while the texture animation sat frozen on frame 0. Fixed by calling `Block::update(dt)` before the early-return paths. The lane honestly reported that this fix is **not photographable**: the shipped platform animation has a single frame, so no screenshot can distinguish "animator ran" from "animator frozen", and that half rests on the mutation-tested unit check alone. Exactly the kind of limit that should be stated rather than papered over.
- **`Spiny::isEgg`** was dead code: every production path (`EntityCatalogue::make<Spiny>`, the only route `EntityFactory::create()` offers, used by both `LevelLoader` and Lakitu's `EntitySpawnRequested` → `PlayingState::spawnProjectile`) calls the single-argument overload, so a Lakitu-dropped Spiny appeared already hatched and walking in mid-air. Fixed by flipping the constructor default to `isEgg = true`.

**Gap the orchestrator found:** the default flip changes **four shipped level placements** — 3 in `assets/levels/level_2.json`, 1 in `level_3.json` — and the lane verified none of them, having tested only World 1-1 and Lakitu. Reading `Spiny::update()` directly shows the hatch is `if (m_isEgg) { if (onGround) { m_isEgg = false; … } }`, so an egg resting on ground hatches on its first grounded update rather than sticking — but that is *reasoning*, and g-rule-11 is explicit that reasoning is not observation. The lane is adding a post-condition regression check (a level-placed Spiny must reach the walking state — asserting the post-condition, not the default, so the test cannot license the bug) and observing the four placements in Worlds 1-2 and 1-3.

**Deviation accepted:** the surgical fix would thread an egg flag through `EntitySpawnRequest` (`Core/GameSnapshot.hpp`), `PlayingState.cpp` and `EntityCatalogue.cpp` — none owned by 2A, and two sibling lanes are editing `PlayingState.cpp` concurrently. Respecting the boundary and flagging it was right. SPEC §6.1's "walks on ground after hatching from egg" describes hatching as how a Spiny comes into being at all, not a Lakitu-only case, so egg-by-default is spec-faithful rather than a workaround.

### 8.15 Lane 5D — accepted, verified by the orchestrator running the build

`A/docs/report-layout`, 2 commits (`0f3e5ea` fix, `93ca2c6` log). Orchestrator ran `Report/SuperMarioGame/build.sh` directly: **0 Overfull \hbox warnings** (baseline was 20, worst 68 pt), guard passes, 61-page PDF, `reports/report_content.py` untouched, no build outputs committed.

The fix is content-general, which was the condition set mid-lane. `_render_table` measures every cell — headers included — for its longest unbreakable code run and plain word, using separately measured pt/char constants for `\texttt` (≈5.4) and bold headers (≈7.1, found via `\settowidth`), plus exact `\tabcolsep` overhead, and works for any column count. `esc_code()` inserts `\allowbreak{}` between characters of any code run over 10 chars. Verified against a **synthetic worst case** (5–7 column tables, 40+ char punctuation-free identifiers) → 0 warnings, which is what proves it generalises to prose 5A–5C have not written yet. Two bugs the lane found and fixed while building it: a monospace-only pt/char constant underestimated bold header width (a real "Commits" column overflowed by 9.8 pt), and a per-column-count `\tabcolsep` fudge underestimated real overhead by 11 pt on a 6-column table.

**Guard is content-independent** — a regex over every `Overfull \hbox (Npt too wide)` in a fresh `main.log` with a 5 pt threshold, no page allowlist, no caption matching. Mutation-tested both directions: reverting the fix → exit 1 with 14 warnings (worst 31.94 pt); restoring → exit 0.

**Q5 answered by measurement.** Detailed UML trees over a measured 1200 pt height (Enemy/Boss 2212 pt, IGameState 1321 pt; Block at 1126 pt untouched) now split into balanced landscape groups. Rendered all 9 pages: 7 clearly legible at 5.4–8.6 pt effective, 2 smaller but readable at ~3.7 pt — against the original ~1.6–2.4 pt illegible shrink. Option B (compact + member-count table) was prototyped and rejected: fully legible but loses every member signature. **Shipped A.**

**Caveat carried forward honestly:** today's visual verification is against `dev @ 6af4f8e` content only. The guard, not the one-time inspection, is what protects the final report once 5A–5C rewrite the prose. Phase 6 must re-inspect.

### 8.16 Defect found in `FACTS`: the report undercounts its own ctest cases

`reports/build_report.py:74-85` `ctest_targets()` counts only `add_verify_test(...)` macro calls and returns **26** headless cases. But `SuperMarioGame/CMakeLists.txt` also registers three guards through explicit `add_test` — `guard_saves_hermeticity_setup` (:262), `guard_saves_hermeticity_check` (:268) and `guard_asset_single_source` (:288) — which the regex cannot see. 26 + 3 = **29**, which is what `ctest` actually runs (§8.5, re-verified twice).

So the generated report currently prints 26 and **understates its own verification**. This is the mirror image of §8.8's lesson: there, a lane typed a number it could not measure; here, the computed number is itself wrong because its extraction missed a second registration mechanism — precisely the "one fact, two places" failure g-rule-17 describes.

**Fix belongs to whichever lane next touches `build_report.py`** (5B refreshes counts through `FACTS`): have `ctest_targets()` also count explicit `add_test(NAME …)` calls, and add a parity assertion that `FACTS["ctests"]` equals the number of cases `ctest -N` reports, so the two cannot drift again. Phase 6 must not simply confirm "prose equals FACTS" — it must confirm FACTS equals reality.

### 8.17 Phase 2 results — 2A, 2B, 2C accepted (2D outstanding)

All re-verified by the orchestrator running `ctest` in each lane's own tree, not taken on report.

| Lane | Branch | ctest | Accepted |
| :-- | :--- | :-- | :-- |
| 2A | `A/fix/moving-platform-and-spiny-egg` | **30/30** | yes, 6 commits |
| 2B | `A/fix/hard-axe-and-vault-pipe` | **30/30** | yes, 1 commit |
| 2C | `A/fix/editor-playtest-gameover` | **29/29** + `verify_frontend_states` 80 → **86** | yes, 3 commits |

`verify_regressions` stayed 584/584 in all three. Merge order 2A → 2B → 2C → 2D → 2E. `PlayingState.cpp` is touched by 2C (**4 lines**, `buildRunSummary()` only) and by 2D; 2E follows 2D.

**2A** — `MovingPlatform::update()` overrode `Block::update()` and never called back into the only place `m_animator` advances. `Spiny::isEgg` was dead code because every production path calls the single-argument constructor. Its mutation test is the wave's best piece of reasoning: reverting the `isEgg` default would have passed **vacuously**, since a non-egg Spiny satisfies "ends up walking" from frame 0, so it mutated the actual hatch trigger in `Spiny::update()` instead. That is the vacuous-pass trap g-rule-17 names, spotted unprompted.

**2C** — the out-of-lives branch called `changeState(GameOverState)` with no `m_isPlaytest` check, unlike `leaveToCallingScreen()` and `advanceToNextLevel()` which both already branch on it; because a `Change` op swaps only the stack's top, the suspended `EditorState` stayed buried and unreachable through `GameOverState`'s `changeState()`-only choices. Fixed through the same `popState()` the existing exits use — no parallel path. Both hunks mutation-tested separately (85/86 and 83/86, exactly the expected checks failing).

### 8.18 Plan defect: the "third HARD axe is unreachable" defect does not reproduce

§6 lane 2B asserts the third HARD axe at tile (189,19) "stands behind the arena enclosure post … unreachable by ordinary movement", making the HARD Bowser fight unwinnable. **Investigated and not reproduced.** Lane 2B confirmed in code that `axeQuotaForDifficulty()` does require all three axes on Hard, then drove a real `Player` through real `PlayingState::update()` frames ("hold Right, mash Jump") and **reached the axe cleanly on the unmodified `level_3.json`**. It also tried the remedy the plan offered — removing the post's solid tile — and it changed nothing: the only obstacle to a jump-free crossing is Bowser's own patrol, which is by design.

The lane therefore **shipped no level-data edit** (`level_3.json` is untouched, verified) and left a mutation-tested reachability guard instead, which fails when the axe is moved past the arena's confinement boundary and passes on the real file. Correct on both counts: g-rule-2 forbids an unjustified data edit, and the earlier claim was never observation. This also corroborates a prior session's own note at 2026-09-02 17:40 that "a jump clearing the post also clears the axe" — the STILL OPEN list carried a defect that its own log had already contradicted.

**Withdrawn from the defect list.** The vault-pipe half of 2B was real and is fixed: `MapGenerator::generateSubLevel()`'s exit pipe never called `setEntryMode()`, so it stayed `Top` while every hand-authored sub-level authors that pipe as `side_left` (per `Pipe.hpp`: Top = descending into a vault, side-entry L-bend = ascending out). Fixed with `setEntryMode(SideLeft)` and reseating from `floorY-3` to `floorY-4`, because a side-entry pipe's mouth is measured from the collider's foot rather than its rim.

### 8.19 Environment trap worth keeping: `build/assets/levels/` is a stale copy

Lane 2B lost real time to false mutation-test results before finding that asset sync is a **`SuperMarioGame`-target-only `POST_BUILD` step**, so `build/assets/levels/` can hold a stale copy of level data while the source tree is correct. Related to the known duplicate-level-asset hazard: which `level_*.json` is actually loaded depends on the launch directory. Any lane editing level data must confirm which copy the running binary reads before concluding a change had or had not any effect.
