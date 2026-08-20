# Learning to Play a Platformer In-Engine: Specification Failures, Certified Curricula, and What They Cost

**Super Mario Game — CS202 side project**
**Branch:** `A/mapgen-gan-plan` (stacked on `A/rl-neural-policy`, off the main line)
**Framework:** `nn::` — a from-scratch C++20 deep-learning library, vendored, CPU/NEON backend
**Date:** 21 August 2026

---

## Abstract

We train a neural policy to play a 2D platformer inside the running game engine.
The policy is a 184,391-parameter multi-layer perceptron mapping a
2844-dimensional agent-centred observation to seven independent button
probabilities. Training runs either in-engine with live visualisation, or
headless for long runs.

We report five findings. Three are failures, and all three were failures of
**specification** — of the observation, the loss, or the reward — rather than of
the model, the optimiser, or the framework. In each case the learner correctly
maximised what it was given, every training-side signal looked healthy, and the
failure was visible only in behaviour measured against something the agent was
*not* trained on.

| # | Finding | Type |
| :- | :--- | :--- |
| 1 | An observation that made correct play impossible | representation |
| 2 | A loss under which never jumping was near-optimal | objective |
| 3 | An algorithm that was not the algorithm it was named after | implementation |
| 4 | Imitation reaching, and locally exceeding, its teacher | positive |
| 5 | Reinforcement learning converging on standing still | objective |
| 6 | The instruments were lying: three measurement defects | measurement |
| 7 | A certified route in the goal channel, and how plans fail | representation |
| 8 | Motion is not optional: observation v4 | representation |
| 9 | A FIFO buffer is not a dataset: forgetting returns at 10 levels | implementation |
| 10 | A generator with a certified fitness, and the first completion | positive |
| 11 | The argmax of a stochastic policy is a different policy | evaluation |
| 12 | A quality gate closes the generator loop: 8/8 certified levels completed, 0 deaths | positive |

Findings 1–5 were reported first (Part I); findings 6–10 (Part II) were made
while acting on Part I's future-work list. Finding 6 is an erratum on Part I's
absolute numbers: the *comparisons* in Part I survive (both sides of every A/B
ran under the same defective instruments), but the absolute progress
percentages there understate, and are superseded by §13's corrected table.

---

## 1. System

### 1.1 Architecture

```
observation 2844 ──[Linear]──▶ 64 ──[tanh]──▶ ──[Linear]──▶ 32 ──[tanh]──▶
                                                    ──[Linear]──▶ 7 ──[sigmoid]──▶ P(button)
```

| Layer | Shape | Parameters | Share |
| :--- | :--- | ---: | ---: |
| Linear 1 | 2844 × 64 | 182,080 | 98.7 % |
| Linear 2 | 64 × 32 | 2,080 | 1.1 % |
| Linear 3 | 32 × 7 | 231 | 0.1 % |
| | | **184,391** | |

`tanh` hidden units: the observation is normalised to [−1, 1] and a zero-centred
activation preserves the sign information distinguishing "wall to the left" from
"wall to the right". **Multi-label sigmoid** output rather than softmax: a real
action is a *set* of simultaneous buttons — running right while jumping is three
at once — which one-of-N cannot express.

### 1.2 Observation (contract v3; superseded by v4 — §11)

```
315 cells (21 × 15, agent-centred) × 9 one-hot symbols = 2835
+ 9 scalars                                            =    9
                                                       = 2844
```

Symbols: `Unknown, Empty, Solid, Hazard, Coin, PowerUp, EnemyStompable,
EnemyDangerous, FriendlyProjectile`. Scalars: `dxToGoal, dyToGoal,
dxToOpponent, dyToOpponent, vx, vy, onGround, canJump, isPoweredUp`.

The width is constant across difficulty tiers — an easier agent's unseen cells
encode as `Unknown` rather than shortening the vector — so one network serves
every tier. The layout is versioned; changing it invalidates every weight file.

### 1.3 Cost, and why training runs in-engine

| Operation | Cost |
| :--- | ---: |
| Inference, batch 1 | 0.052 ms |
| Training step, batch 1 | 0.281 ms |
| Game simulation, one frame | 0.433 ms |
| Replay minibatch, per sample | 0.088 ms (2.45× cheaper than batch 1) |

The network is **10.7 %** of an inference step and **33 %** of a training step;
the simulation dominates. Compiling the math library `-O3 -mcpu=native` rather
than at the project's Debug default is worth a measured **42×** (12.77 → 0.30 ms
per step); unoptimised, one step consumes 77 % of a 60 Hz frame budget and
in-engine training is impossible.

---

## 2. Finding 1 — An observation that forbade correct play

Under the previous contract every enemy encoded as one `Enemy` symbol. But:

```cpp
Goomba::onStomped() { m_isSquished = true; }     // defeated, +2 reward
Spiny::onStomped()  { player->takeDamage(1); }   // the player is hurt
```

Two cells requiring opposite behaviour were **byte-identical in the network's
input**. For $x_1 = x_2$, any function gives $f(x_1) = f(x_2)$: no capacity,
data, or training time changes this. The best achievable policy must treat both
identically and therefore be wrong about one.

Two related collapses had the same character: every item encoded as `Reward`
(a Star indistinguishable from a coin), and every projectile as `Hazard` —
including the agent's **own** fireballs, so an agent that shot then fled from
its own shot.

**Intervention.** The vocabulary was widened to nine symbols. Stompability is
derived from behaviour, not taste: a virtual `Enemy::isStompSafe()` returns
false in exactly the five enemies whose `onStomped()` damages the player or
ignores the stomp. Projectiles are separated on `Projectile::damagesPlayer()`.

**Result, and an instructive regression.** Heuristic performance after the split
was *identical*. The change buys no immediate improvement; it buys that the
distinction exists, which is a precondition for any policy getting it right.
The first attempt at *using* the new symbols — treating `EnemyDangerous` as a
hazard to flee, as lava is — cost level_2 **73 percentage points** (75.3 % →
1.8 %): a Spiny on the route made goal-directed utility negative, so the agent
reversed and oscillated. Fleeing is right for lava, which cannot be jumped from
standing, and wrong for an obstacle one tile tall. Recast as a jump trigger,
performance returned to 75.3 %.

---

## 3. Finding 2 — A loss under which doing nothing was near-optimal

Initial objective: unweighted MSE between seven sigmoid outputs and the
teacher's seven binary presses.

| Metric | Value |
| :--- | ---: |
| Episodes / samples | 219 / 88,968 |
| Teacher agreement | 96.6 % → **99.9 %** |
| Mean loss | 0.0155 → **0.0010** |
| **Jumps in 1800 frames** | **0** |
| Progress, level_1 | **8.2 %** (teacher 23.9 %) |

The jump button is pressed on 1.2–3.4 % of decisions, so:

```
P(jump)                                        =  3.44 %
agreement of a never-jumping policy, on jump   = 96.56 %
aggregate, other six buttons perfect           = 99.51 %
```

**The optimiser was correct.** Never jumping *is* near-optimal for unweighted
per-button MSE. The domain makes this maximally damaging: platforming consists
almost entirely of the rare frames the loss is least sensitive to.

**Intervention.** Two changes, both to the objective, neither to the model.
*(a)* Cross-entropy in place of MSE — the proper scoring rule for Bernoulli
targets, and practically, MSE's gradient vanishes exactly where the network is
most wrong (a sigmoid saturated at 0.02 when the answer is 1 gives a negligible
MSE gradient and a large cross-entropy one). *(b)* Per-button class balancing:
a button pressed with probability $r$ gets weight $0.5/r$ when pressed and
$0.5/(1-r)$ when not.

$$\mathcal{L} = -\frac{1}{N}\sum_i w_i\left[a_i\log p_i + (1-a_i)\log(1-p_i)\right]$$

**Result.**

| | jumps / 1800 | progress | deaths |
| :--- | ---: | ---: | ---: |
| MSE, unweighted | **0** | 8.2 % | 0 (stuck) |
| Class-balanced CE | **82** | **22.2 %** | 5 |
| Heuristic teacher | 62 | 23.9 % | 7 |

A **2.7× behavioural improvement**, while the headline agreement metric *fell*
99.9 % → 95.1 %. It had been measuring conformity to a degenerate solution.

**Methodological consequence.** One aggregate number concealed the total failure
of one output for 219 episodes. Per-button agreement is now logged and displayed
per button.

---

## 4. Finding 3 — The algorithm was not the one it was named after

The method was called DAgger (Ross et al., 2011), but the implementation took
**one SGD step on the newest sample only**. DAgger is *defined* by dataset
aggregation — retaining every transition and retraining on the union — and that
aggregation is what yields its no-regret guarantee.

Without it, as the teacher-control coefficient $\beta \to 0$ the learner drives,
visits its own poor states, and trains exclusively on those, overwriting earlier
competence. Measured over 220 episodes **while training continued**:

| | first 20 episodes | last 20 |
| :--- | ---: | ---: |
| Teacher agreement | 0.939 | 0.928 |
| Jump agreement | 0.849 | **0.736** |
| Evaluated progress, level_1 | — | **1.2 %** |

Catastrophic forgetting, not underfitting.

**Intervention and two further errors.** A bounded aggregation buffer (12,000
samples ≈ 136 MB; the unbounded union would be 2 GB) replayed alongside each new
sample. The first attempt replayed **16 sequential single-sample steps**, which
is ~17× the intended learning rate; training-time progress fell 31 % → 1.4 %. A
true minibatch averages the gradients and is also 2.45× cheaper per sample.
Separately, the weighted loss indexed its per-button weights by *flat* position,
so in a batched tensor every button in rows 2…N was weighted as if it were
`groundPound`; now indexed by column.

**Result.** Training-time progress rises and holds — 25.0 % → 37.8 %, flat over
the final four checkpoints — where previously it decayed.

---

## 5. Finding 4 — Imitation reaches, and locally exceeds, its teacher

Deterministic evaluation, 120 s per level, identical conditions:

| Level | Neural | Teacher | |
| :--- | ---: | ---: | :-- |
| `level_1` | 23.9 % | 23.9 % | **exact match** — same progress, 7 deaths each, 62 jumps each |
| `level_2` | 9.3 % | 75.3 % | much worse |
| `level_3` | **21.4 %** | 15.2 % | exceeds |
| `bonus_1` | **18.3 %** | 14.9 % | exceeds |

The level_1 result is the strongest evidence that imitation succeeded: the
policy reproduces the teacher's trajectory to the same progress, the same death
count, and the same number of jumps.

That it *exceeds* the teacher on two levels does not contradict imitation's
theoretical ceiling. The teacher is stochastic in effect — its behaviour depends
on enemy timing — and the student, trained across many trajectories, appears to
have learned a smoothed policy that avoids some of the teacher's specific
mistakes. This is a plausible reading, not a demonstrated mechanism, and it is
tested by a per-level trajectory comparison we have not run.

**Variance is a real limitation.** Across runs, evaluated level_1 progress
ranged from 1.2 % to 23.9 % under nominally similar configurations. Results in
this report are single runs; none should be read to three significant figures.

---

## 6. Finding 5 — Reinforcement learning converged on standing still

**Method.** Episodic REINFORCE fine-tuning from the imitation policy. Actions
sampled from the policy's own Bernoulli outputs (a deterministic threshold gives
REINFORCE nothing to learn from); action repeat of 4 decisions ≈ 67 ms, since
independently sampling seven buttons at 60 Hz gives motion too incoherent to
credit; returns $G_t = r_t + \gamma G_{t+1}$ with $\gamma = 0.99$, standardised
within episode, running-mean baseline.

**Two setup errors found first.** *(a)* Standardising by a within-episode
standard deviation floored at $10^{-8}$ turned floating-point noise into
full-magnitude advantages — a stuck episode has returns equal to four decimal
places — so the agent trained hard on rounding error. Fixed by skipping episodes
flatter than a minimum spread. *(b)* The 4 s stall cut-off, sensible during
imitation, killed every exploration episode before it could pay off: measured
median episode length was **61 decisions, exactly the timeout**, across 12,584
episodes that learned nothing. Relaxed during the RL phase, median rose to 901.

**The substantive result.** With both corrected, REINFORCE optimised its
objective successfully:

| | first third | last third |
| :--- | ---: | ---: |
| Episode return | −8.63 | **−1.54** |
| Deaths | 82 | **0** |

And the resulting policy scores **0.0 %** on level_1 *and* level_2.

**Analysis.** The shipped reward weights are:

```
progressPerPixel  +0.01/px  →  crossing level_1 (6400 px) = +64 total
died              −50
timeStep          −0.001/decision → 60 s at 60 Hz = −3.6
```

A single death costs almost as much as crossing the entire level earns.
Standing still costs −3.6 and risks nothing, so the globally optimal policy is
to not move — and REINFORCE found it. The improvement from −8.63 to −1.54 is
*entirely* the elimination of deaths.

**The correction did not work.** Rebalancing 10× toward progress (a full
crossing worth +640 against a death's −10) produced the same outcome: 0.0 %
progress, return flat at −7.6 across 220 reinforcement episodes. So the result
is not a single badly-chosen constant. Standing still is a stable local optimum
reachable from the pretrained policy, and undirected Bernoulli exploration under
a discount horizon of ~1.7 s does not escape it.

**This is the report's clearest methodological point.** In both runs every
training-side signal was healthy — return rising monotonically, deaths falling
to zero, stable loss, episodes running to completion. Only evaluation against an
external measure revealed the agent had stopped playing.

---

## Part II

## 7. Finding 6 — The instruments were lying: three measurement defects

Every experiment in Part I ran through two instruments: the evaluation harness
(`eval_level`) and the level's own metadata. Acting on the future-work list
exposed a defect in each, plus one in the trainer's data path. None was
visible from inside training, which by now is this project's least surprising
sentence.

**(a) The harness reported the last life, not the run.** On respawn the
per-life progress marker resets (it must — stall detection is per-life), and
the report copied it straight in, so the reported number described whichever
episode happened to be running when time expired — usually a truncated one.
`bonus_1` reported 9.2 % while every one of its 89 full episodes reached
17.7 %. Every absolute progress number in Part I understates. Corrected
teacher baseline: level_1 37.8 %, level_2 75.3 %, level_3 21.4 %, bonus_1
17.8 %.

**(b) Every level lied about where its goal was.** `saveLevel` stamped the
flagpole metadata at `width − 2` regardless of where the flagpole entity
stood. level_1's real flag: x = 185; its metadata: x = 198 — *behind the
victory castle*. The solvability oracle trusted the metadata, so every
certified route ended by vaulting the castle roof: a constant 0.7071 demand (a
5-tile gap against the 7.07-tile maximum run-jump) that appeared in every
level, drowning every difficulty signal the generator emitted. Writer and
reader are fixed; the oracle now prefers the entity, which is what the game
actually completes on.

**(c) Teacher-driven episodes supervised "don't jump" on jump frames.** The
controller consumes one-shot buttons (jump, shoot) inside its action struct
the frame they fire, and the trainer read that struct *after* the frame ran.
The learner only ever saw honest jump labels in learner-driven DAgger
episodes; behavioural cloning episodes were systematically corrupted. Fixed by
returning the decision as chosen, not as consumed.

The pattern of Part I holds and sharpens: specification failures extend to the
**measurement layer**, and a defective instrument is worse than a missing one,
because it returns plausible numbers.

---

## 8. Finding 7 — A certified route in the goal channel, and how plans fail

Part I's first future-work item was the goal signal: `dxToGoal` pinned to the
map's right edge, `dyToGoal` a hardcoded zero — a degenerate high-level policy
reading *rightward, always*. The project's solvability oracle already computed
the kindest winnable route through every level as a list of footholds, so the
route now ships as a per-level sidecar (`<level>.waypoints.json`) and the
controller's goal channel follows it. This tests the two-network hierarchical
hypothesis with a **perfect** high-level policy before any second network is
trained: the oracle plans, the executor executes.

Three plan-following failure modes appeared immediately, each with a repair:

1. **Lookahead destroys the signal it smooths.** Aiming three nodes ahead read
   past jump edges (a jump is one edge spanning many tiles), saturating
   `dxToGoal` at 1.0 exactly where `dyToGoal` was supposed to say "climb
   here". The goal is now the first unconsumed node: small-but-correct beats
   smooth-but-wrong.
2. **Consumption needs height parity.** Walking under a hill "passed" every
   node on top of it by x-comparison, running the route index ten tiles ahead.
   A node may only be consumed by passing if the agent is at its height.
3. **A plan you have fallen off is worse than no plan.** Off-route, the next
   node hangs unreachable overhead and every goal-following behaviour becomes
   wall-bashing — on level_3, the escape behaviour then walked the agent off a
   ledge into lava it could not see (its hazard probe reads one row down; the
   trough was four), every one of that level's 15 deaths. After 8 s of zero
   route progress the route is abandoned for that life and the right-edge goal
   returns.

With guidance in place the teacher's honest numbers moved: level_1 37.8 → 38.0,
bonus_1 17.8 → **74.1 %** (deaths 89 → 11). level_3 *fell* 43.0 → 21.4 with
zero deaths — exposing that its old 43.0 % had been achieved *by* the lethal
escape flinging the agent across the lava trough. The wall-jump (Finding 8)
re-earned it honestly.

---

## 9. Finding 8 — Motion is not optional: observation v4

Finding 1 established that byte-identical encodings of behaviourally different
things are an information-theoretic wall. Observation v4 removes three more
walls of the same kind:

- **Motion planes.** Two floats per cell carry the occupying entity's velocity
  (signed — direction rides with speed). A *moving* platform was
  byte-identical to a parked one in a single-frame observation; no memoryless
  policy could time boarding one. Enemy approach and projectile direction come
  free.
- **Item identity.** A star (inverts every avoidance rule while active), a
  1-up (changes what a risk costs), and a trampoline (terrain that throws you
  ~6 tiles — encoded as generic Solid it was an invisible catapult) leave the
  `PowerUp` bucket. Vocabulary 9 → 12.
- **Self state.** `onWall`, power tier, invincibility seconds. The physics has
  had a wall-jump all along; no policy could see the wall it was pressed
  against, and a starred agent could not know it was starred.

Cell layout becomes `[one-hot ×12, vx, vy]`; feature count 2844 → 4422; the
version gate refuses v3 weights (observed doing so at the first v4 launch).
The jump button now means wall-jump when airborne and on a wall — no new
action dimension; an existing output gains a context meaning the physics
already supported.

Two behaviours previously *inexpressible* were then written into the teacher
in a few lines each: **scaling** a tall wall by ratcheting wall-jumps (tried
before conceding to backing off), and **waiting** for a moving platform seen
approaching in the motion planes — standing still was not an action the old
teacher could choose. The oracle gained the matching moves (ride: a boarded
platform's whole patrol for a 0.25 timing surcharge; bounce: 831.4 px/s
against its own envelope) once the level schema learned to carry a platform's
patrol at all — it had silently dropped it, flattening every placement to a
4-tile default.

Teacher, honest metric, cumulative through v4 + certified routes:

| Level | Part I baseline | corrected baseline | + routes & v4 | deaths |
| :--- | ---: | ---: | ---: | :--- |
| `level_1` | "23.9 %" | 37.8 % | **72.6 %** | 18 → 6 |
| `level_2` | "75.3 %" | 75.3 % | 75.3 % | 0 → 4 |
| `level_3` | "15.2 %" | 21.4 % | **43.0 %** | 15 lethal-escape → 15 at the new frontier |
| `bonus_1` | "14.9 %" | 17.8 % | **74.1 %** | 89 → 11 |

level_1's final jump (53.4 → 72.6) came from re-certifying routes with the
ride move: the route crosses on a moving platform, and the teacher waits for
and boards it.

---

## 10. Finding 9 — A FIFO buffer is not a dataset: forgetting returns at 10 levels

Part I's Finding 3 fixed catastrophic forgetting with a 12,000-sample
aggregation ring buffer, and on one level it held. Scaled to a 10-level
rotation, the ring held ~1.3 episodes per level and the forgetting signature
returned on schedule: jump agreement 0.99 → 0.82 across 442 episodes while
every loss curve looked healthy, and the student evaluated at a flat 8.2 %
with zero deaths — the hesitate-and-stall attractor again.

The defect is structural, not a size problem. DAgger's no-regret guarantee is
about training on the **union of everything ever collected**; a FIFO ring is a
recency window, which under a rotation is a monoculture of the last few
levels. The buffer is now a **reservoir** (Vitter's Algorithm R): once full,
the *n*-th sample replaces a uniformly random slot with probability
capacity/*n*, keeping a uniform sample of all history in fixed memory. Since
17.7 KB × more capacity does not fit the machine, features are stored
quantized to int8 — the one-hots are exact, velocities get 1/127 steps —
making 48,000 samples cost what 12,000 floats did.

Artifacts of the decayed FIFO run are preserved
(`saves/ai/*_fifo12k_drifted*`, `training_log_fifo12k.csv`).

---

## 11. Finding 10 — A generator with a certified fitness, and the first completion

The generator half of this project exists to answer Finding 5. Reinforcement
learning converged on standing still because completion is a sparse, distant
reward: an agent that has never reached a flag gets no gradient toward one.
The generator's job is to make levels on which winning is *reachable*, then
ratchet difficulty — a curriculum with a certificate.

**The loop.** `genome → render → oracle → fitness`, no agent in it. A genome
is the map generator's nine knobs (pit rate, pipe rate, enemy rate, coin rate,
roughness, theme, difficulty, hazard flags, seed); a thin tool renders it to a
level; the solvability oracle scores the result. Fitness: *winnable* is a hard
gate, closeness to a target required-difficulty band is the curriculum knob,
column-histogram distance from already-kept levels is diversity pressure.
Search is a small evolution: tournament selection, blend crossover, Gaussian
mutation, seed rerolls for phenotype diversity.

**Difficulty is quantized.** Required difficulty takes values n/7.07 (0.141,
0.283, 0.424, …): the demand of an n-tile gap against the maximum run-jump.
A band does not tune a continuum; it picks a quantum.

**The grammar has a floor.** No genome produced a level below ~0.685: some
standard chunk always demands a 3-up-4-across jump. Easy bands were therefore
*unreachable by search* — a 6 × 16 run returned zero levels under 0.25. The
oracle's bottleneck names both ends of the hardest edge, so a **repair
descent** follows: place one stepping stone at the edge's midpoint (in height
too — at takeoff height it splits distance but not climb), re-certify, and
continue *while the bottleneck moves*, because a level is limited by many
copies of its hardest chunk. Repair took the floor from 0.685 to 0.283 and
produced six certified easy levels where search alone produced none.

**External validation.** The oracle certifies **13 of 15** real Super Mario
Bros levels (VGLC corpus) winnable under this engine's physics — and both
failures are honest model gaps, not false alarms: mario-2-1's reachability
stops at 93.3 % exactly at its spring, mario-3-1's at 35.9 % where its
elevator platforms run. (The corpus's processed format does not encode either
device, so these two cannot certify from that data at all.)

**The milestone.** On the repair-descended easy band, the teacher
**completed a level** — the first completion recorded in this project by any
policy on any level. Four sibling levels certify at the same 0.2828 yet the
teacher sticks at ~40 %: the oracle says those levels are easy, so what fails
there is the agent. That is the decomposition doing its job — *reachable but
failed* is the one row worth training on, and it is now machine-generatable
at a chosen difficulty.

Training now rotates over 16 levels: 4 campaign, 6 mid-band (0.35–0.55),
6 easy-band (repair-descended to 0.283).

---

## 12. Finding 11 — The argmax of a stochastic policy is a different policy

The reservoir run (599 imitation episodes, 16-level rotation, honest labels,
mean agreement 0.949) produced a checkpoint that **reached flags 33 times**
in training rollouts — and evaluated deterministically at a flat 8.2 % on
every level, frozen at the first staircase, forever. The trace names the
freeze exactly: pressed against the wall, route saying *climb* (dy = −0.2),
the network's jump output sits at 0.24 — under the 0.5 cut, under even its
calibrated 0.264 cut — and with zero evaluation noise nothing ever breaks the
loop.

Per-button threshold calibration (the midpoint of class-conditional
prediction means, now maintained by the trainer and persisted in the
checkpoint sidecar) was necessary but not sufficient: 0.24 vs 0.264 is a
near-miss that a different level would turn into a hit, which is no way to
act. The categorical fix is to stop projecting: the trained object is a
*distribution* over button sets — during training the learner acted by
sampling from it — and evaluating its argmax evaluates a policy that was
never trained. Seeded Bernoulli sampling (`eval_level --stochastic`)
evaluates the policy that was, reproducibly:

| Level | teacher | student, argmax | **student, as trained** |
| :--- | ---: | ---: | :--- |
| `level_1` | 72.6 % | 8.2 % | 44.8 %, 12 deaths |
| `level_2` | 75.3 % | 8.2 % | 38.6 %, 9 deaths |
| `level_3` | 43.0 % | 8.2 % | 34.7 %, 14 deaths |
| `bonus_1` | 74.1 %, 11 deaths, never finished | 13.9 % | **86.6 % — COMPLETED, 0 deaths** |
| easy `evolved_02` | completed, 1 death | 8.2 % | **completed, 1 death** |

Two results stand out. **The student completes levels** — the project's first
learned-policy completions, on a campaign level and on a generated one. And on
`bonus_1` it **surpasses its teacher**: the teacher never finished that level
and spent 11 lives not finishing it; the student finished it without dying
once. Its path tells the more interesting story: **16 % route coverage**, mean
deviation 4.5 tiles — it barely touched the certified route and found its own
line. The oracle's route is a proof that a path exists, not a prescription;
the learner generalised past it.

---

## 13. Closing the loop — the gate passes

"Make the maps better" does not terminate; an acceptance test does. The gate
(`tools/map_quality_gate.py`) makes "the maps are best" an executable
proposition — a batch passes when every level clears four bars:

| Bar | Test | Basis for the number |
| :--- | :--- | :--- |
| CERTIFIED | winnable by the oracle, required difficulty in the declared band | non-negotiable |
| FORGIVING | forgiveness ≥ 0.85 | the completed level measured 0.94; the six-lives-one-pit level 0.84 — the bar sits just above the observed failure |
| FAIR | no tile takes more than half of an evaluated agent's deaths (6+) | a death monopoly is a trap, not a challenge |
| DISTINCT | pairwise column-histogram distance ≥ 1.0 | three "different" levels were signature-distance-0.00 duplicates |

The gate's first runs failed every existing batch and each failure named its
repair: hard dedup at the keep-pool door (the duplicates), iterated **safety
nets** under the oracle's punishing edges (a floor tile 4 below the edge turns
death into a survivable drop while the jump's demand stays exactly — measured
0.794 → 0.825 per pass), nets clamped into the map (a net below the bottom row
protected nothing), and wipe-before-write (stale levels kept failing fresh
batches). The FAIR bar then caught what static analysis cannot: three levels
whose *observed* deaths all landed on one tile (9/9, 8/8, 11/11) — netted at
the observed cluster, difficulty unchanged, and on re-evaluation all three
went from all-deaths-at-one-tile to **completed with zero deaths**.

Final state, seeded-stochastic student, single runs:

| Batch | Gate | Student behaviour |
| :--- | :--- | :--- |
| easy v2 (2 levels, 0.283) | **PASS**, all four bars | **2/2 completed, 0 deaths** |
| mid v3 (6 levels, 0.424–0.529) | **PASS**, all four bars | **6/6 completed, 0 deaths** |
| campaign (hand-made, for reference) | not gated | level_1 **75.1 %** — above its teacher's 72.6 % |

Every generated level that passes the gate is completed by the learned agent
without dying. The training rotation now consists of exactly the gate-passing
batches plus the campaign; earlier batches retire from rotation and stay in
the repository for the record. The loop — evolve → certify → repair → train →
evaluate paths → repair where the paths blame — is closed, and each stage's
verdict is a number a script checks rather than an impression.

---

## 14. Discussion

### 14.1 Every failure was a specification failure

| Failure | Located in | Visible in the optimised metric? |
| :--- | :--- | :--- |
| Goomba/Spiny indistinguishable | observation | No |
| Never jumping | loss weighting | No — it *improved* the metric |
| Forgetting | missing aggregation | Partly — agreement fell slightly |
| Standing still | reward weighting | No — return rose monotonically |
| Last-life metric | evaluation harness | It *was* the metric |
| Goal behind the castle | level metadata | No — every route "worked" |
| Consumed-action labels | trainer data path | No — loss fell on wrong labels |
| Moving platform = parked platform | observation | No |
| FIFO recency monoculture | buffer structure | Partly — jump agreement decayed |
| Generator difficulty floor | generator grammar | No — every level "winnable" |
| Argmax of a sampled policy | evaluation protocol | No — training rollouts completed levels |

None was a bug in the model, optimiser, or framework. The diagnostic that worked
every time was **behavioural evaluation against a measure the agent was not
trained on** — jumps per 1800 frames, progress through the level — while
training curves were uninformative or actively misleading.

### 14.2 Capacity and framework are not the constraints

The network reaches 99.9 % agreement at 0.0010 loss, so it is not
capacity-limited; scaling to 2844‑512‑256‑7 costs 6.3× the compute to fit a
function already fit. The network is 10.7 % of an inference step, so replacing
the tensor library optimises the smaller part. The one framework-level change
that mattered was a build flag, worth 42×.

### 14.3 Limitations

- **Field of view.** The agent sees 21 × 15 tiles = 672 × 480 px against the
  human's 1280 × 720 — **52 % of the screen width**, 10.5 % of a level. It is
  imitating a teacher that sees more than it does.
- **Four levels.** Nothing here demonstrates generalisation, and four levels
  invite memorisation.
- **Run-to-run variance** of the same configuration is large (§5).
- **Scale.** Hundreds of episodes, not millions. Negative results bound these
  hyperparameters here; they do not bound the methods in general.

### 14.4 Future work, in priority order

Items 1 and 4 of Part I's list are done (Findings 7 and 10); item 2's premise
changed — the curriculum now exists, so exploration pressure comes from the
certified band rather than from noise shaping. The list as it stands:

1. **Student completions on the easy band.** The reservoir run's success
   criterion is binary and behavioural: does the *student* complete
   repair-descended levels? Then ratchet the band.
2. **Widen the field of view** to screen parity — information the agent lacks,
   in preference to hidden capacity, which restates what it has.
3. **Repair beyond stepping stones.** The descent stalls at 0.283 because one
   stone cannot split a 2-tile hop; stone *removal* (widening a corridor,
   lowering a wall) extends the same oracle-named-cell mechanism downward.
4. **The GAN as the proposal distribution.** The corpus is imported and
   certified; once convolutions land in the vendored framework, the GAN
   replaces `genome → level` and the certified fitness stays exactly as is —
   which was the design's point.
5. **Evolution strategies over policy weights**, as the learning-from-
   consequence method that needs only episode returns — the thing Finding 5
   showed REINFORCE could not be trusted with here.

---

## 15. Reproduction

```bash
mkdir build && cd build && cmake .. && make -j8
./train_policy --episodes 150 --imitation-only        # headless, no window
./eval_level assets/levels/level_1.json --policy saves/ai/policy_imitation.ckpt
./eval_level assets/levels/level_1.json --trace 0:1800   # per-frame agent state
./SuperMarioGame --train                              # visual training, 16-level rotation

# The generator pipeline (Finding 10):
python3 tools/solvability.py --waypoints assets/levels/*.json   # certified routes
python3 tools/evolve.py --band 0.35:0.55 --generations 6 \
        --population 16 --keep 6 --out assets/levels/generated
python3 tools/solvability.py corpus/TheVGLC/"Super Mario Bros"/Processed/mario-*.txt
```

In-engine with live visualisation: main menu → **TRAIN AI**. `SPACE` pauses,
`+`/`-` set simulation speed (1× renders every frame; above 1× the world draws
only on a new furthest point or a phase change), `ESC` checkpoints and exits.

Every episode appends to `saves/ai/training_log.csv`: `episode, phase, samples,
mean_loss, agreement, jump_agreement, jump_rate, return, outcome`. Imitation and
reinforcement checkpoints are written to separate files, because a collapsed RL
run destroyed a working 22.2 % imitation policy by sharing a filename.

---

## 16. Summary of measurements

| Configuration | level_1 | jumps/1800 | deaths |
| :--- | ---: | ---: | ---: |
| Heuristic teacher | 23.9 % | 62 | 7 |
| Imitation, unweighted MSE | 8.2 % | 0 | 0 (stuck) |
| Imitation, class-balanced CE | 22.2 % | 82 | 5 |
| Imitation, no aggregation, 220 ep | 1.2 % | — | 7 |
| **Imitation, aggregated (final)** | **23.9 %** | **62** | **7** |
| REINFORCE, default reward | 0.0 % | 80 | 0 (stuck) |
| REINFORCE, rebalanced reward | 0.0 % | — | 0 (stuck) |

Part I numbers above were measured under the last-life metric (Finding 6a) and
are kept for internal comparison only. Part II, honest metric:

| Configuration | level_1 | level_2 | level_3 | bonus_1 |
| :--- | ---: | ---: | ---: | ---: |
| Teacher, corrected baseline | 37.8 % | 75.3 % | 21.4 % | 17.8 % |
| + certified routes | 38.0 % | 75.3 % | 21.4 % | **74.1 %** |
| + observation v4 (wall-jump, waiting) | 53.4 % | 75.3 % | **43.0 %** | 74.1 % |
| + ride/bounce-certified routes | **72.6 %** | 75.3 % | 43.0 % | 74.1 % |
| Student, FIFO-12k, 442 ep (drifted) | 8.2 % | 8.2 % | 8.2 % | 13.9 % |
| Student, reservoir-48k, argmax | 8.2 % | 8.2 % | 8.2 % | 13.9 % |
| **Student, reservoir-48k, as trained** | **44.8 %** | **38.6 %** | **34.7 %** | **86.6 % ✓ completed** |

Generator: 16/16 winnable by generation 1 in-band 0.35–0.55; easy band
0 → 6 levels via repair descent (floor 0.685 → 0.283); VGLC 13/15 certified;
forgiveness metric validated on first contact (completed level highest at
0.94, six-lives-one-pit level lowest at 0.84) and folded into fitness.
**Completions**: teacher on repair-descended `evolved_02`; student (as
trained) on `evolved_02` and on campaign `bonus_1` — the latter with zero
deaths, surpassing a teacher that never finished it.

Final cycle (episode-400 checkpoint, certified batches): **8/8 gate-passing
levels completed by the student, all with zero deaths**; campaign level_1 at
75.1 % — above its teacher. Both batches pass all four gate bars, the FAIR
bar measured on the student's own recorded paths. The three death monopolies
the FAIR bar caught (9/9, 8/8, 11/11 deaths on one tile) were each netted at
the observed cluster with required difficulty exactly unchanged, and each
level flipped to a zero-death completion.

### References

Ross, S., Gordon, G. J., & Bagnell, J. A. (2011). *A Reduction of Imitation
Learning and Structured Prediction to No-Regret Online Learning.* AISTATS.

Williams, R. J. (1992). *Simple statistical gradient-following algorithms for
connectionist reinforcement learning.* Machine Learning, 8(3–4).

Mnih, V. et al. (2015). *Human-level control through deep reinforcement
learning.* Nature, 518. (action repeat / frame skip)

Amodei, D. et al. (2016). *Concrete Problems in AI Safety.* arXiv:1606.06565.
(reward hacking)

Vitter, J. S. (1985). *Random Sampling with a Reservoir.* ACM TOMS 11(1).
(Algorithm R)

Summerville, A. et al. (2016). *The VGLC: The Video Game Level Corpus.*
arXiv:1606.07487.
