# 🗺️ GAN Level Generation × RL Agent — Joint Setup Plan

> **Branch**: `A/mapgen-gan`, stacked on `A/rl-neural-policy` and kept **off the
> main line**, exactly as that branch is. This is the *second half of the same
> side project*, not a main-path feature.
>
> **Nothing on `dev` depends on any of it.** `dev` ships the heuristic AI and
> the hand-generated campaign levels and plays identically with this branch
> dropped. The submission can include or drop the pair independently, and the
> course deliverable never depends on a model training successfully.
>
> Companion to `docs/two_player_ai_plan.md` §6 (the RL policy seam) and
> `docs/rl_training.md` (the RL training design, on `A/rl-neural-policy`).

## 0. Why it is stacked on the RL branch, not on `dev`

The two side projects share one substrate (§4). If the map generator branched
off `dev` it would have to duplicate the evaluation runner, or the two would
have to be merged to be used together — which is precisely the pressure that
drags a side project onto the main line. Stacking keeps the whole experiment
one droppable unit:

```
main ── dev ──┬─ (campaign levels, heuristic AI: ships regardless)
              └─ A/rl-neural-policy ── A/mapgen-gan     ← side project, both halves
```

Rules this branch holds itself to, so the framing stays true and does not decay
into a de-facto dependency:

1. **No behaviour change on the default path.** Nothing added here runs unless
   a CLI flag or an explicitly-chosen menu entry asks for it.
2. **The C++ footprint stays at one flag plus one optional game mode.** All
   training, generation and orchestration is Python outside the game — the same
   split `rl_training.md` already committed to.
3. **Generated levels are additive files**, never replacements. `level_1..3`
   stay exactly as they are on `dev`.

---

## 1. The idea, and the honest version of it

Two learning side projects are on the table:

1. **RL agent** — a neural `IAIPolicy` trained to play (seam already shipped;
   training design in `docs/rl_training.md`).
2. **GAN level generator** — replace/augment `MapGenerator`, whose output is
   the reason the campaign levels play poorly.

The intuition "they may be better together" is correct, and it has a name in
the literature: agent-in-the-loop level generation (MarioGAN / Volz et al.
2018) and adversarial curriculum generation (POET, PAIRED). The generator
gives the agent unlimited, varied training levels so it does not overfit the
three campaign maps; the agent gives the generator the one thing a GAN cannot
provide by itself — a **playability and quality oracle**. A GAN knows what
levels *look like*; only an agent that plays them knows what they *play like*.

But "train both in one loop" on day one is how research projects stall. The
right first move is smaller: **both projects consume the exact same three
pieces of infrastructure, none of which exist yet. Build those first**, then
each project progresses independently at its own rate, and the loop between
them tightens later almost for free.

## 2. Why the current MapGenerator plays badly (so the GAN fixes the right thing)

`src/Utils/MapGenerator.cpp` is a single left-to-right pass of *independent*
probability rolls: a sine-wave elevation profile, a per-column pit dice roll,
uniform decoration/enemy rates. Structurally it cannot express what makes a
Mario level good:

- **No rhythm or phrasing** — real levels alternate tension and rest
  (setup → challenge → reward). Independent per-column rolls produce white
  noise, not phrases.
- **No intra-level difficulty curve** — column 20 and column 180 are sampled
  from the same distribution.
- **No composed challenges** — a pit *and* an enemy *and* a moving platform
  arranged as one obstacle is what makes gameplay; independent rolls almost
  never compose them deliberately.
- **No playtest in the loop** — "solvability guardrails" are static rules
  (max pit width, a platform coin-flip), not evidence anyone can finish it.

A learned generator attacks the first three directly, because it learns tile
*co-occurrence structure* from real levels. The fourth is attacked by the
agent-as-critic, never by the generator itself.

## 2b. The departure from the literature — measure the level, not the player

*Added after the first eval runs. This is the part of the project that is not a
reimplementation of something in `docs/rl_gan_pcg_literature.html`.*

Every agent-in-the-loop generator in that bibliography — Volz et al.'s
latent-space Mario GAN, PAIRED, EA SEED's adversarial RL, MAP-Elites over a GAN
latent space — scores a candidate level by having something play it. That
conflates two unrelated facts:

```
"this level cannot be finished"   ← a property of the level
"this player cannot finish it"    ← a property of the player
```

A failed playthrough looks identical either way. This is not a hypothetical:

> **The heuristic agent finishes none of the four shipped campaign levels.**
> It stalls at 10.8% (`level_1`), 13.8% (`level_2`), 21.4% (`level_3`) and
> 46.4% (`bonus_1`), in all three archetypes, having never died — it simply
> stops making progress and stays stopped for the remaining ~145 seconds.

Used naively as a fitness function, that agent rejects hand-vetted shipped
levels as broken. In a curriculum loop it reports every level as "too hard"
forever, and the 50–70% success band never has anything in it.

The literature works around the confound rather than removing it: PAIRED cancels
it with regret (protagonist minus antagonist, so shared incompetence subtracts
out — at the cost of a second trained agent), Volz sidesteps it with A* (its own
kind of competence, and a search per candidate).

**`tools/solvability.py` removes it.** It floods the level with every jump the
engine's own constants permit — no policy, no learned search, no agent — and
answers reachability exactly, in ~0.25 s per level against ~5 s for one rollout.
The three-way decomposition it enables is the project's actual contribution:

| reachable? | agent clears it? | meaning |
| :-- | :-- | :--- |
| no | — | **broken.** Reject. No agent needed to know it. |
| yes | yes | too easy for this agent |
| yes | no | **the useful case** — real difficulty, or an agent gap |

Only the third row is worth training on, and it is exactly the row an
agent-only fitness cannot isolate.

### Difficulty as a bottleneck, not an average

Because reachability is computed as a graph, difficulty is available as a
**minimax** quantity rather than a heuristic score: the hardest single move on
the *kindest* route to the goal. That is what "how hard is this level" actually
means, and both naive alternatives get it wrong — averaging along a path hides
the one brutal jump that makes players quit behind a hundred trivial ones, and
scoring the hardest move anywhere on the map calls a level hard when an easy
detour exists.

Measured on the shipped levels, with 1.0 meaning "at the limit of what the
physics allows":

| level | required | hardest optional | reachable |
| :--- | ---: | ---: | ---: |
| `level_1` | 0.71 | 0.96 | 100% |
| `level_2` | **0.96** | 0.96 | 100% |
| `level_3` | **0.96** | 0.96 | 100% |
| `bonus_1` | 0.71 | 0.81 | 100% |
| `level_*_sub` | 0.14 | 0.42 | 100% |

Two findings drop straight out, neither of which any playtest had produced:

1. **`level_2` and `level_3` require a jump at 96% of the physical maximum just
   to finish.** There is no margin. That is not difficulty, it is a level you
   clear by pixel luck — and it is a direct consequence of `MapGenerator`
   rolling step heights independently of what a player can clear.
2. **There is no difficulty curve, there is a cliff**: 0.14 in the sub-levels,
   0.71–0.96 in the campaign. Nothing sits in between.

This also answers the three requirements directly:

- **Winnable maps** — guaranteed by construction. A generated level is checked
  before an agent ever sees it, and when it fails, the reachability frontier
  names the exact columns where progress stops, which is a repair instruction
  rather than a verdict.
- **Difficulty handling** — a target difficulty becomes a *filter and a search
  objective* over the generator's output, and the reported bottleneck tile is
  the knob to edit. Difficulty is set before generation, not sampled and hoped
  for.
- **An agent that can learn to win** — see §2c.

## 2c. Making the agent learnable, not just present

The stall finding is also a finding about the RL side, and it changes two things
in `rl_training.md`:

1. **The reward's progress term is wrong.** `RewardTracker` pays
   `progressPerPixel` for rightward movement. In a level where the route goes up
   a staircase or briefly back left, rightward-x rewards the agent for pressing
   into the wall it is already stuck against. **Replace it with progress along
   the reachability graph**: the solvability oracle already computes a distance
   label for every foothold, so "how far along a known-good route am I" is
   available as a dense, correctly-shaped signal that does not reward pushing
   into geometry. This is a strictly better potential function and it costs one
   precomputed field per level.
2. **Training only on reachable levels is what makes winning learnable at all.**
   An agent trained on levels that cannot be finished is being given an
   unlearnable reward, and no amount of exploration fixes it. The oracle is the
   admission filter.

Open question, deliberately not answered yet: the heuristic policy's stall is
also a **bug worth fixing on its own terms** — it stops dead at a 3-tile step
that the oracle says is clearable. Whether to fix the heuristic or to let the
learned policy supersede it is a decision for when the RL run actually trains.
Either way, the finding is logged rather than papered over.

## 3. The data problem (decides whether a GAN is even trainable)

**The campaign levels are themselves `MapGenerator` output** — fixed seeds in
`tools/generate_game_levels.cpp` (10101, …). Training a GAN on them would
faithfully learn the distribution we are trying to escape. So:

- **Primary corpus**: the VGLC Super Mario Bros. corpus (public, ~30 levels
  as tile-character grids — the same data MarioGAN used), mapped into our
  tile vocabulary. Sliding-window slices (e.g. 14 tiles high × 28 wide)
  turn ~30 levels into thousands of training samples; this is the standard
  trick that makes a DCGAN viable on tiny data.
- **Secondary corpus**: levels authored/repaired in our own level editor.
  Every hand-fixed level is a training sample; this compounds over time.
- **Mapping decision to make once**: VGLC is 14 tiles tall, our levels are
  23. Proposal: generate the 14-row *playfield band* and pad
  ceiling/underground rows deterministically per theme, rather than asking
  the GAN to learn 9 rows of mostly-empty sky.

## 4. Phase 0 — the shared substrate (build this first, it serves both)

This is the direct answer to "set these two up so both progress at a good
rate." Three deliverables, all small, all C++-light:

### 4.1 Headless evaluation runner

Already flagged as future work in `two_player_ai_plan.md` §6 — this promotes
it to the *first* deliverable, because it is the bottleneck for **both**
projects: RL needs fast rollouts; the GAN needs solvability/fitness scores
for thousands of candidate levels.

**Correction to this document's first draft**: it said "the `--script` in-process
runner is 80% of it." That is wrong. `--script` goes through `Game::run()`,
which calls `initWindow()` and `initImGui()` — it is a *scripted* run, not a
headless one. And `PlayingState` cannot be reused headlessly as-is either:
`PlayingState.cpp:545` reads `ImGui::GetIO()` (a hard abort with no context),
`loadLevelByPath` is private so there is no arbitrary-level entry point, an
`AIController` can only ever be attached to a CPU *Player 2*, and its update
path writes to disk (`CampaignProgress`, autosave), pushes states on death and
victory, and takes a full-world Memento snapshot every frame.

So the runner is a **separate lightweight harness** — `tools/eval_level.cpp`,
built like `tools/generate_game_levels.cpp` — that reuses the real
`LevelLoader`, `EntityFactory`, `PhysicsEngine`, `Entity::update` and
`AIController`, and reimplements only the five self-contained gameplay rules it
needs (lava, void-fall, level timer, warp pipes, boss arena). Zero window, zero
GL, zero ImGui.

One non-obvious requirement, which would have produced silently wrong fitness
scores: **17 entity and strategy files call `Game::getInstance().getNearestPlayer()`
or `getTileMap()`**. Without `Game::setPlayer()` and `setTileMap()`, every enemy
in the level stands still and every generated level scores as easy.

The interface:

```
./SuperMarioGame --eval assets/levels/gen_0042.json \
                 --policy heuristic          # or: neural <weights file>
                 --agents 1 --max-seconds 120 \
                 --report saves/eval/gen_0042.stats.json
```

- No window, no rendering, no audio; fixed 1/60 timestep stepped as fast as
  the CPU allows.
- Emits one JSON report: `{completed, timeToFlag, maxProgressX, deaths,
  damageTaken, coins, enemiesKilled, stuckFrames (longest interval with no
  progress), usedMovingPlatform, …}`.
- These fields are the **only currency** the two projects exchange. The RL
  trainer reads them as episode summaries; the GAN pipeline reads them as
  level fitness. Versioned like `kAIObservationVersion`.

### 4.2 Level tensor contract — **built**

`SuperMarioGame/tools/level_tensor.py`, with `docs/level_tensor_contract.md`
as the prose half. Roundtrips all 7 shipped levels cleanly
(`level_tensor.py check assets/levels/*.json`).

Three decisions it makes, all measured rather than assumed:

- **A semantic vocabulary, not the `TileType` enum.** 9 classes: `empty,
  solid, breakable, question, pipe, hazard, coin, enemy, platform`.
  Ground/Ice/Conveyor collapse to one `solid` class because they are the same
  thing to a player and to a generator — a surface you stand on. Which one is
  emitted is the *theme's* decision, made at decode time. Learning the skins
  would spend model capacity on what is already a config field.
- **A 14-row band, y=9..22.** Measured across `level_1..3` and `bonus_1`:
  every tile outside the two ceiling rows and every entity lives in rows
  12..22, and rows 2..8 are empty sky in all of them. 14 is also exactly the
  VGLC corpus height, so a VGLC level drops in with no rescaling. The ceiling
  is re-emitted deterministically per theme on decode.
- **Amendment to §5: enemies are IN the tensor, as one generic class.** The
  original sketch placed all entities by rule afterwards. That is backwards —
  *where* an enemy sits relative to a gap or a ledge is structural, and it is
  the part worth learning (VGLC encodes enemies in-grid for the same reason).
  The decoder still picks *which* species from the theme, which is the part a
  rule does perfectly well. Bosses, star coins, the flagpole and power-ups
  stay out: fixed count and authored intent, not sampled structure.

Dependency-free by design: the canonical form is a label grid, so
encode/decode/check need no numpy. numpy is imported lazily and only by the
`corpus` command that writes the training `.npz`.

### 4.3 Corpus assembly

VGLC download + mapping script + our levels directory scanned for
editor-authored maps. Output: one `data/corpus/*.npz` the GAN trains on.

**C++ footprint of all of Phase 0: the `--eval` flag.** Everything else is
Python/docs — the same "game does inference and evaluation, Python does
training" split `rl_training.md` already committed to.

## 5. Phase 1 — GAN track (independent of RL)

MarioGAN-shaped, offline, in Python:

1. DCGAN on corpus window slices → generator `G(z) → 14×28×C` chunks.
2. **Stitching**: full 200-wide level = sequence of chunks, either by
   latent-space interpolation or overlap-blending — start with plain
   concatenation + repair.
3. **Repair pass** (deterministic C++-rule logic, reimplemented in Python or
   run through the existing MapGenerator guardrail rules): cap pit widths,
   ensure a standable start/exit zone, drop floating half-tiles, insert the
   flagpole/exit.
4. **Decode pass**: `level_tensor.decode` turns the class grid into level
   JSON — theme skins for solid/breakable/hazard, a species per `enemy`
   cell, the ceiling, and the fixed furniture (spawn, flagpole, star coins)
   the tensor deliberately does not carry.
5. **Agent filter**: every candidate runs through `--eval --policy
   heuristic`. Discard unfinishable levels; score the rest on the report
   fields (completion time, death count, stuck time) → keep the top slice.
6. **Ship as a pool**: pre-generate N vetted levels offline, commit the JSON,
   and add an "Endless / Remix" mode that draws from the pool. **No GAN
   inference in C++** — unlike the policy net (a small MLP), a deconv
   generator is not worth reimplementing; pre-generation makes it a
   non-problem.
7. Later, latent-space search (CMA-ES over `z`, fitness = eval report) to
   *target* difficulty instead of filtering for it — this is the MarioGAN
   result, and it needs nothing new, just Phase 0's runner.

Note the heuristic policy is the critic here — **Phase 1 does not wait for a
trained RL agent.** This alone fixes "map gen is somewhat bad."

## 6. Phase 2 — RL track (independent of GAN)

Exactly as `docs/rl_training.md`, with one amendment: the training level set
is `campaign ∪ generated pool`, sampled per episode. Generated levels are
domain randomization — the single cheapest defense against the agent
overfitting three fixed maps. Until the pool exists, train on campaign maps;
swap the sampler when Phase 1 ships. That is the *entire* coupling at this
stage, by design.

## 7. Phase 3 — closing the loop (only after 1 & 2 both work standalone)

The actual co-training the original idea points at:

```
repeat:
  1. Sample/search z's whose levels put the CURRENT agent's success
     rate in a target band (~50–70% — hard enough to teach,
     easy enough to learn: the curriculum frontier)
  2. Train the agent on those levels
  3. (optional) fine-tune G toward the band, PAIRED-style regret
```

Both sides "progress at a good rate" precisely because the band tracks the
agent: as the agent improves, yesterday's 60% levels become 95% levels and
fall out of the curriculum automatically.

- With Phase 0 in place this loop is **one Python orchestrator script** —
  eval runner for measurement, tensor contract for levels, `ExperienceLog`
  for transitions. No new C++.
- Warning, stated now: step 3 (adversarial generator updates) is the finicky,
  research-grade part — unconstrained, the generator collapses to impossible
  levels. Steps 1–2 (curriculum by *search/filtering* over a frozen G) get
  ~80% of the benefit with none of the instability. Do 1–2; treat 3 as
  stretch.

## 8. Caveats and fallbacks

- **GAN is the ambitious choice, not the safe one.** On tiny corpora,
  Markov-chain / WFC (wave-function-collapse) generators are cheaper and
  often comparable. Keep a Markov baseline in the Phase 1 pipeline (same
  repair pass, same agent filter) — it de-risks the deliverable and makes
  the GAN's win measurable in the report.
- **Solvability is never the generator's job.** The agent filter is
  permanent, whichever generator wins.
- **Scope honesty**: this is two research-flavored side projects on a CS202
  C++ game. The design keeps the C++ surface to one CLI flag and one
  level-pool game mode precisely so the course deliverable never depends on
  training succeeding.

## 9. Order of work

| # | Deliverable | Track | Depends on | State |
| :- | :--- | :--- | :--- | :--- |
| 1 | `eval_level` headless runner + versioned stats report | shared | — | **done** |
| 2 | Level tensor contract + `level_tensor.py` | shared | — | **done** |
| 2b | Solvability + bottleneck-difficulty oracle | shared | 2 | **done** |
| 3 | Corpus: VGLC mapping + editor levels | shared | 2 | next |
| 4 | Baseline generator → repair → oracle filter → agent filter → pool | mapgen | 2b, 3 | |
| 5 | DCGAN generator (replaces baseline in same pipeline) | mapgen | 4 | |
| 6 | "Endless/Remix" mode loading the vetted pool | mapgen | 4 | |
| 7 | RL training, reachability-shaped reward, sampler = campaign ∪ pool | RL | 1, 2b | |
| 8 | Curriculum loop (difficulty-targeted level search) | joint | 5, 7 | |
| 9 | PAIRED-style generator fine-tuning | stretch | 8 | |

Note that 2b moved the oracle *ahead* of the agent in the pipeline at step 4:
levels are now filtered geometrically first and played second. That ordering is
what makes the agent filter affordable — it never wastes a 5-second rollout on a
level a 0.25-second reachability check could have rejected.
