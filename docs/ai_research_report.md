# Learning to Play a Platformer In-Engine: Three Specification Failures and What They Cost

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

### 1.2 Observation (contract v3)

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

## 7. Discussion

### 7.1 Every failure was a specification failure

| Failure | Located in | Visible in the optimised metric? |
| :--- | :--- | :--- |
| Goomba/Spiny indistinguishable | observation | No |
| Never jumping | loss weighting | No — it *improved* the metric |
| Forgetting | missing aggregation | Partly — agreement fell slightly |
| Standing still | reward weighting | No — return rose monotonically |

None was a bug in the model, optimiser, or framework. The diagnostic that worked
every time was **behavioural evaluation against a measure the agent was not
trained on** — jumps per 1800 frames, progress through the level — while
training curves were uninformative or actively misleading.

### 7.2 Capacity and framework are not the constraints

The network reaches 99.9 % agreement at 0.0010 loss, so it is not
capacity-limited; scaling to 2844‑512‑256‑7 costs 6.3× the compute to fit a
function already fit. The network is 10.7 % of an inference step, so replacing
the tensor library optimises the smaller part. The one framework-level change
that mattered was a build flag, worth 42×.

### 7.3 Limitations

- **Field of view.** The agent sees 21 × 15 tiles = 672 × 480 px against the
  human's 1280 × 720 — **52 % of the screen width**, 10.5 % of a level. It is
  imitating a teacher that sees more than it does.
- **Four levels.** Nothing here demonstrates generalisation, and four levels
  invite memorisation.
- **Run-to-run variance** of the same configuration is large (§5).
- **Scale.** Hundreds of episodes, not millions. Negative results bound these
  hyperparameters here; they do not bound the methods in general.

### 7.4 Future work, in priority order

1. **Correct the goal signal.** `AIController` hardcodes the goal to the map's
   right edge and `dyToGoal` to zero, so the "where am I going" channel reads
   *rightward, always*. A degenerate hierarchy already exists and is defective.
   The project's solvability oracle computes an exact optimal waypoint path;
   feeding those waypoints in tests the hierarchical hypothesis with a *perfect*
   high-level policy before any second network is trained.
2. **Directed exploration for RL.** Undirected Bernoulli noise does not escape
   the standing-still optimum under either reward scaling. Entropy
   regularisation, or an explicit progress-based curriculum, is the next thing
   to try — not another reward constant.
3. **Widen the field of view** to screen parity — information the agent lacks,
   in preference to hidden capacity, which restates what it has.
4. **Expand the training distribution** — more hand-authored levels, then
   generated ones, as domain randomisation.

---

## 8. Reproduction

```bash
mkdir build && cd build && cmake .. && make -j8
./train_policy --episodes 150 --imitation-only        # headless, no window
./eval_level assets/levels/level_1.json --policy saves/ai/policy_imitation.ckpt
./eval_level assets/levels/level_1.json --trace 0:1800   # per-frame agent state
```

In-engine with live visualisation: main menu → **TRAIN AI**. `SPACE` pauses,
`+`/`-` set simulation speed (1× renders every frame; above 1× the world draws
only on a new furthest point or a phase change), `ESC` checkpoints and exits.

Every episode appends to `saves/ai/training_log.csv`: `episode, phase, samples,
mean_loss, agreement, jump_agreement, jump_rate, return, outcome`. Imitation and
reinforcement checkpoints are written to separate files, because a collapsed RL
run destroyed a working 22.2 % imitation policy by sharing a filename.

---

## 9. Summary of measurements

| Configuration | level_1 | jumps/1800 | deaths |
| :--- | ---: | ---: | ---: |
| Heuristic teacher | 23.9 % | 62 | 7 |
| Imitation, unweighted MSE | 8.2 % | 0 | 0 (stuck) |
| Imitation, class-balanced CE | 22.2 % | 82 | 5 |
| Imitation, no aggregation, 220 ep | 1.2 % | — | 7 |
| **Imitation, aggregated (final)** | **23.9 %** | **62** | **7** |
| REINFORCE, default reward | 0.0 % | 80 | 0 (stuck) |
| REINFORCE, rebalanced reward | 0.0 % | — | 0 (stuck) |

### References

Ross, S., Gordon, G. J., & Bagnell, J. A. (2011). *A Reduction of Imitation
Learning and Structured Prediction to No-Regret Online Learning.* AISTATS.

Williams, R. J. (1992). *Simple statistical gradient-following algorithms for
connectionist reinforcement learning.* Machine Learning, 8(3–4).

Mnih, V. et al. (2015). *Human-level control through deep reinforcement
learning.* Nature, 518. (action repeat / frame skip)

Amodei, D. et al. (2016). *Concrete Problems in AI Safety.* arXiv:1606.06565.
(reward hacking)
