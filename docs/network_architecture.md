# Policy Network: Architecture, Empirical Evaluation, and Design Recommendations

**Project:** SuperMarioGame — reinforcement-learning agent side project
**Branch:** `A/mapgen-gan-plan` (stacked on `A/rl-neural-policy`)
**Framework:** `nn::` (vendored from CS200-Cpp), CPU/NEON backend
**Date:** 20 August 2026
**Status:** Imitation learning implemented and measured; reinforcement not yet implemented

---

## 1. Network Specification

### 1.1 Topology

```
input  1899  ──[Linear]──▶  64  ──[tanh]──▶  64
                             │
        64  ──[Linear]──▶   32  ──[tanh]──▶  32
                             │
        32  ──[Linear]──▶    7  ──[sigmoid]──▶  7 outputs
```

| Layer | Shape | Weights | Biases | Parameters | Share |
| :--- | :--- | ---: | ---: | ---: | ---: |
| Linear 1 | 1899 × 64 | 121,536 | 64 | **121,600** | **98.1 %** |
| Linear 2 | 64 × 32 | 2,048 | 32 | 2,080 | 1.7 % |
| Linear 3 | 32 × 7 | 224 | 7 | 231 | 0.2 % |
| | | | | **123,911** | 100 % |

Weight initialisation is Xavier (`nn::Linear`); biases are zero-initialised.

### 1.2 Input encoding (1899 features)

Defined by `AIObservation::toFeatureVector()`; the layout is a versioned
contract (`kAIObservationVersion`, currently **2**).

```
315 grid cells (21 wide × 15 tall, agent-centred) × 6 one-hot states = 1890
+ 9 scalars                                                          =    9
                                                                     = 1899
```

Cell states, in fixed order: `Unknown, Empty, Solid, Hazard, Reward, Enemy`.

Scalars, in fixed order: `dxToGoal, dyToGoal, dxToOpponent, dyToOpponent,
vx, vy, onGround, canJump, isPoweredUp`. All values lie in [−1, 1].

Two properties are load-bearing:

- **Fixed width across difficulties.** An Easy agent sees a 5×5 window, but the
  vector is still 1899 long; unseen cells encode as `Unknown`. One trained
  network therefore serves every difficulty tier.
- **One-hot rather than ordinal.** The six cell states have no ordering.
  Encoding `Solid = 2, Hazard = 3` would assert that a hazard is "more" of
  whatever solid is, which is false and would be learned as though true.

### 1.3 Output encoding (7 features)

Seven independent buttons in `AIAction` declaration order:
`moveLeft, moveRight, jump, run, crouch, shoot, groundPound`.

**Multi-label sigmoid, not softmax.** A real action is a *set* of simultaneous
buttons — running right while jumping is three at once — which a one-of-N
softmax cannot express. Each output is thresholded independently at 0.5;
`moveLeft ∧ moveRight` is resolved to whichever logit is larger.

These are deliberately the same seven fields `PlayerFramePacket` records for
the Shadow Mario replay system, so a recorded *human* session is valid training
data with no translation layer.

### 1.4 Activation rationale

| Position | Function | Reason |
| :--- | :--- | :--- |
| Hidden | `tanh` | The input is normalised to [−1, 1]; a zero-centred activation keeps hidden representations in the same regime. ReLU would discard the sign information that distinguishes "wall to the left" from "wall to the right" in the scalar block. |
| Output | `sigmoid` | Each output is an independent Bernoulli parameter — the probability that a button is pressed. Bounded [0, 1] and directly thresholdable. |

---

## 2. Training Procedure (as implemented)

**Algorithm:** DAgger (Ross, Gordon & Bagnell, 2011) — supervised imitation of
the heuristic policy, evaluated on states the *learner* visits.

| Component | Choice |
| :--- | :--- |
| Loss | `nn::MSELoss` between the 7 sigmoid outputs and the teacher's 7 binary presses |
| Optimiser | `nn::SGD`, learning rate 0.01, no momentum |
| Batch size | 1 (online, one update per agent decision) |
| Teacher | `HeuristicPolicy` (Speedrunner archetype) |
| β schedule | 1.0 → 0.0, decaying 0.05 per episode (teacher-driven → learner-driven) |
| Episode end | death, 60 s timeout, 4 s without horizontal progress, or reaching the goal |

**DAgger rather than plain behavioural cloning**, because cloning trains only
on states the *teacher* visits; the instant the learner drifts elsewhere it is
out of distribution with no supervision there. This is the standard
compounding-error failure, and DAgger addresses it by having the teacher label
whatever state the learner actually reaches.

---

## 3. Empirical Results

### 3.1 Optimisation converges

219 episodes, 88,968 samples:

| Metric | First quarter | Last quarter |
| :--- | ---: | ---: |
| Teacher agreement | 96.6 % | **99.9 %** |
| Mean MSE loss | 0.0155 | **0.0010** |

Button outputs move from undifferentiated (~0.5 everywhere) to decisive.
**The optimisation is healthy and the network fits the teacher.**

### 3.2 …and the resulting agent is markedly worse than its teacher

Evaluated on `level_1`, 120 s budget, identical conditions:

| Policy | Progress | Deaths | Outcome | Jumps in 1800 frames |
| :--- | ---: | ---: | :--- | ---: |
| Heuristic (teacher) | **23.9 %** | 7 | timeout | **62** |
| Neural (99.9 % agreement) | **8.2 %** | 0 | **stuck** | **0** |

### 3.3 Diagnosis: the metric is not the objective

The jump button is pressed on 62 of 1800 frames — **3.44 %** of the time.
Therefore:

```
P(jump pressed)                                    =  3.44 %
agreement of a policy that NEVER jumps, on jump    = 96.56 %
aggregate agreement if the other six are perfect   = 99.51 %
```

The measured 99.9 % aggregate agreement is fully consistent with a network that
has learned the six common buttons well and the jump button not at all — and
the direct measurement confirms exactly that: **zero jumps in 1800 frames.**

This is severe class imbalance combined with an unfortunate property of the
domain: the rare class is not noise, it is the decisive action. Platforming
consists almost entirely of the 3 % of frames the loss is least sensitive to.
MSE assigns identical weight to a missed jump and a spurious crouch.

**Conclusion: the network is not under-trained or under-sized. It is optimising
a quantity that is not the goal.**

---

## 4. Recommendation 1 — Should the network be larger?

### **No.** Capacity is not the binding constraint.

The evidence is decisive: the network already reproduces the teacher's function
to 99.9 % per-button agreement with 0.0010 training loss. A model that fits its
training signal nearly perfectly is not capacity-limited; adding parameters
addresses underfitting, and there is no underfitting here.

Measured cost of scaling, for reference (training step, batch 1, −O3):

| Architecture | Parameters | ms / step | vs current |
| :--- | ---: | ---: | ---: |
| 1899‑64‑32‑7 (current) | 123,911 | 0.216 | 1.0× |
| 1899‑128‑64‑7 | 251,335 | 0.366 | 1.7× |
| 1899‑256‑128‑7 | 519,559 | 0.660 | 3.1× |
| 1899‑512‑256‑7 | 1,104,391 | 1.362 | 6.3× |

Scaling would buy a 6× slowdown to fit a teacher we already fit. Worse, the
teacher itself plateaus (§2c of the plan), so the ceiling of *perfect*
imitation is still a mediocre player.

**Note on where capacity sits:** 98.1 % of parameters are in the first layer,
because the input is 1899-wide and highly sparse (each of the 315 cells has
exactly one active one-hot). If capacity ever *does* need attention, the
principled move is not a wider hidden layer but a better input representation
— a learned embedding over the 6 cell states, or a convolution over the 21×15
grid to exploit its spatial structure, which the current fully-connected layer
throws away entirely.

---

## 5. Recommendation 2 — Should the project move to PyTorch?

### **No**, on four grounds, three of them measured.

**5.1 The framework is not the bottleneck.**

| Component | Cost | Share |
| :--- | ---: | ---: |
| Game simulation (physics, entities) | 0.433 ms / frame | — |
| Network inference (batch 1) | 0.052 ms | **10.7 %** of an inference step |
| Network training (batch 1) | 0.216 ms | **33.3 %** of a training step |

Even eliminating the network's cost entirely would leave two thirds of the
training step untouched. A faster tensor library optimises the smaller half.

**5.2 The operating regime is the one PyTorch is weakest at.** PyTorch's
advantages are large batches and GPU throughput. This workload is batch size 1,
124K parameters, on CPU, called from inside a 60 Hz C++ game loop. Per-call
Python dispatch overhead alone is comparable to our entire 0.216 ms step.

**5.3 It would eliminate the feature that motivated this work.** The user's
requirement is that training be *visualised on screen as it happens*. That
requires training inside the render loop. PyTorch requires either embedding a
Python interpreter in the game or inter-process communication, and either
choice destroys the self-contained CMake build the submission depends on.

**5.4 Current headroom is ample.** At 0.216 ms/step, training consumes 1.3 % of
a 60 Hz frame budget. The measured in-game training runs at 64× real time.

**One genuine efficiency finding that does apply:** minibatching is
substantially more efficient per sample even in this framework —

| Batch size | ms / step | ms / sample |
| ---: | ---: | ---: |
| 1 | 0.216 | 0.216 |
| 32 | 2.819 | **0.088** |

**2.45× better throughput per sample.** When reinforcement learning is added, a
replay buffer with minibatch updates is worth adopting — for statistical
reasons anyway (decorrelating consecutive, highly-correlated frames), with the
speed as a bonus. This requires no change of framework.

---

## 6. Recommendation 3 — The two-network (hierarchical) proposal

### **Yes — adopt it, but stage it, and take the free version first.**

The proposal is a high-level network selecting *where to go* and a low-level
network executing *how to move*. This is well-founded in the literature
(Options framework — Sutton, Precup & Singh 1999; FeUdal Networks — Vezhnevets
et al. 2017; HIRO — Nachum et al. 2018), but the reasons it fits *this* project
specifically are stronger than the generic case:

**6.1 The high-level signal is currently a hardcoded constant — and it is wrong.**

`AIController.cpp:211`:

```cpp
const float goalX = static_cast<float>(tileMap.getWidth()) * Constants::TILE_SIZE;
```

The goal is unconditionally the right edge of the map. `dyToGoal` is hardcoded
to `0.0f`. So features 7 and 8 of the observation — the entire "where am I
going" channel — encode "rightward, always". Any level requiring vertical
routing or backtracking is misinformed by construction. **The hierarchy does
not need to be invented; a degenerate one already exists and is defective.**

**6.2 The measured failure is precisely a low-level failure.** The network does
not jump. That is motor control, not goal selection. Separating the two means
the low-level policy can be trained and diagnosed against a *correct* goal
signal rather than a constant.

**6.3 Timescale separation matches the observed dynamics.** Waypoint choice
changes every few seconds; button presses change every frame. Training both
through one 60 Hz MSE conflates a slow decision with a fast one.

**6.4 The high-level planner already exists and is exact.**
`tools/solvability.py` computes the full reachability graph and the bottleneck
path to the goal. It *is* a high-level plan — an optimal waypoint sequence,
computed in ~0.25 s per level with no learning at all.

### Recommended staging

| Stage | Action | Cost | Tests |
| :-- | :--- | :--- | :--- |
| **1** | Replace the hardcoded `goalX`/`dyToGoal` with the **next waypoint from the oracle's reachability path**. No second network. | ~1 day | Does a correct goal signal alone improve the low-level policy? |
| **2** | Add reward-weighted learning (§7) against the corrected goal signal. | — | Does the agent exceed its teacher? |
| **3** | Train a high-level *network* to propose waypoints, imitating the oracle. | — | Can it generalise to levels where the oracle is unavailable at runtime? |

**Stage 1 is the scientifically correct first move**, because it tests the
hierarchy hypothesis with a *perfect* high-level policy for free. If a correct
goal signal does not improve behaviour, a *learned* high-level policy — which
can only be worse than the oracle — certainly will not, and the effort is saved.
Building two networks before running that test would risk attributing to
hierarchy an improvement that came from anywhere else.

---

## 7. Prerequisite: the objective must change first

None of the above matters more than this. The current gradient signal is MSE
against teacher buttons; `RewardTracker` exists and is **not connected**. No
reward, return, or advantage enters the weight update.

Two changes are required before any architectural work is worth evaluating:

1. **Class-imbalance correction.** Weight the jump button by inverse frequency,
   or report per-button agreement instead of one average. Either would have
   made this failure visible in a single episode rather than 219.
2. **Reward-weighted learning.** Wire `RewardTracker` in, so that a missed jump
   costs what it actually costs — falling in a pit — rather than 1/7th of one
   frame's MSE.

Until an action's weight in the loss reflects its consequence in the game,
network size, tensor framework and hierarchy are all optimisations of the wrong
quantity.

---

## 8. Summary of recommendations

| Question | Answer | Primary evidence |
| :--- | :--- | :--- |
| Make the network bigger? | **No** | 99.9 % agreement, 0.0010 loss — not capacity-limited |
| Move to PyTorch? | **No** | Network is 10.7 % of inference cost; batch-1 CPU regime; would destroy live visualisation |
| Two-network hierarchy? | **Yes, staged** | Goal signal is currently a hardcoded constant; oracle supplies an exact high-level plan for free |
| Highest-priority change? | **Reward + class weighting** | Network never jumps; jump is 3.44 % of frames and 100 % of the difficulty |
