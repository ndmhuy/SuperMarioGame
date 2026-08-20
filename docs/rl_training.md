# Reinforcement learning against the AI opponent

> **Branch**: `A/rl-neural-policy`, kept off the main line so it can be included
> in or dropped from the submission independently. Nothing on
> `A/shadow-mario-ai-multiplayer` depends on it — that branch ships the seam and
> the heuristic policy, and plays fine without any of this.

> **Correction, from `docs/mapgen_gan_rl_plan.md` §2d.** The "training belongs
> in Python" call below is superseded. Two things changed it: the user has a
> from-scratch C++ deep learning framework
> (`/Users/huynguyen/Documents/CS200-Cpp`, `nn::` — Tensor autograd,
> `Sequential`/`Linear`, `SGD`/`Adam`, existing RL precedent in that repo) to
> build on instead of a second hand-rolled forward pass; and the user wants
> training visualized on screen as it happens, which only a training loop
> running inside the render loop can do. At ~123K parameters (the observation
> and hidden-layer widths below fix that number), one optimizer step on that
> framework's CPU backend is microseconds, not milliseconds — the "duplicate
> of something better" argument below was sound for a large network and does
> not apply at this size. See `mapgen_gan_rl_plan.md` §2d and §6 for the
> integration shape (vendored static library, `NeuralPolicy` rebuilt on
> `nn::Sequential`, a new in-game training state) before touching any of this.
> Everything below about the observation, action space and reward is
> unaffected — it describes the contract, not where the maths runs.

## What is in the game, and what is not

The game owns the half of the loop that has to live in the game, and nothing
more — **as of the design below.** Training has since moved into the game too;
see the correction above.

| In the game | Not in the game |
| :--- | :--- |
| Encoding the observation (`AIObservation::toFeatureVector`) | Gradients, optimisers, replay buffers |
| Running a forward pass (`NeuralPolicy`) | Training |
| Turning outputs into buttons (`AIController::actuate`) | Hyperparameter search |
| Producing the reward (`RewardTracker`) | Anything that wants a GPU |
| Logging transitions (`ExperienceLog`) | |

*(Superseded, kept for the reasoning it still gets right about **why not** to
reinvent an autograd engine from zero: don't — which is exactly why this project
reuses an existing one rather than hand-rolling gradients inside
`HeuristicPolicy`-style code.)* Training belongs in Python, where the tooling
already exists. Reimplementing backpropagation inside a 60Hz game loop would be
slower, harder to test, and a duplicate of something better.

## The observation

`AIObservation::featureCount()` is **1899** floats, all in `[-1, 1]`:

```
315 grid cells (21 wide x 15 tall, agent-centred) x 6 one-hot cell states = 1890
+ dxToGoal, dyToGoal, dxToOpponent, dyToOpponent, vx, vy,
  onGround, canJump, isPoweredUp                                          =    9
```

Cell states are `Unknown, Empty, Solid, Hazard, Reward, Enemy` in that order.
One-hot rather than ordinal on purpose: the six states have no order, and
encoding `Solid=2, Hazard=3` would tell the network a hazard is "more" of
whatever solid is.

Two properties are load-bearing:

- **The width never changes.** An Easy agent sees a 5x5 window, but the vector is
  still 1899 long — the cells it cannot see encode as `Unknown`. One trained
  network therefore accepts every difficulty.
- **The layout is a contract.** Reordering the grid, the one-hot states or the
  scalars invalidates every trained weight file. If it has to change, bump
  `kAIObservationVersion` in `IAIPolicy.hpp`; `NeuralPolicy::load` refuses a file
  whose version does not match, because a silently misaligned input layer gives
  you a policy that acts confidently and arbitrarily — the hardest thing to
  diagnose from the outside.

## The action space

Seven independent buttons, in this order:

```
moveLeft, moveRight, jump, run, crouch, shoot, groundPound
```

A **multi-label sigmoid head**, not a softmax: a real action is a *set* of
buttons — running right while jumping is three at once — and a softmax over
one-of-N cannot express that. `NeuralPolicy` thresholds each output at 0.5, then
resolves left+right to whichever output was stronger.

These are deliberately the same seven buttons `PlayerFramePacket` records for
Shadow Mario. That means **a recorded human session is imitation-learning data**
for free: point `ExperienceLog` at a human match and the rows have the same
shape as the agent's.

## Collecting data

Turn recording on from the dev panel ("Match: Shadow & CPU" → Learning → *Start
recording experience*), or call `AIController::enableLearning(path)`. One row per
*decision*, not per frame — the agent acts at its difficulty's reaction cadence,
and logging idle frames would fill the file with duplicates.

`saves/experience/session.jsonl`, one JSON object per line:

```json
{"type":"header","observationVersion":1,"featureCount":1899,"visionWidth":21,...}
{"obs":[...1899 floats...],"act":[false,true,true,false,false,false,false],"rew":1.49,"done":false}
```

Rows append across runs, so several sessions make one dataset. `rew` is what
accumulated *since the previous decision*, which is what a transition needs;
`done` marks the last step of an episode and is what stops a learner
bootstrapping a value estimate across a death.

## The reward

`RewardTracker` subscribes to events the game already published — no gameplay
code knows a reward exists. Defaults, all in `RewardTracker::Weights`:

| Signal | Weight |
| :--- | ---: |
| Coin | +1 |
| Enemy defeated | +2 |
| Power-up | +5 |
| Star coin | +10 |
| Level complete | +100 |
| Took damage | −10 |
| Died | −50 |
| Rightward progress | +0.01 / px |
| Per decision | −0.001 |

Progress pays only for **new ground** — measured against the furthest point
reached, not the previous frame. Paying per rightward step and charging per
leftward one nets to zero over a round trip in theory, and in practice an agent
finds the asymmetry and paces on the spot.

## Getting weights back in

Export as JSON with `[out][in]` weight rows:

```json
{
  "observationVersion": 1,
  "layers": [
    { "weights": [[...1899 floats...], ...], "biases": [...] },
    { "weights": [[...], ...],               "biases": [...] }
  ]
}
```

The first layer's input width must be 1899 and the last layer's output width must
be 7; `load()` checks both and refuses rather than guessing. Activations are
`tanh` on hidden layers and `sigmoid` on the output — match that in training or
the exported weights will not mean the same thing.

From PyTorch:

```python
import json, torch

def export(model, path):
    layers = []
    for module in model:                      # nn.Sequential of nn.Linear
        if isinstance(module, torch.nn.Linear):
            layers.append({
                "weights": module.weight.detach().cpu().tolist(),   # already [out][in]
                "biases":  module.bias.detach().cpu().tolist(),
            })
    json.dump({"observationVersion": 1, "layers": layers}, open(path, "w"))
```

Drop the result at `SuperMarioGame/assets/config/ai_weights.json` and press *Swap
in neural policy* in the dev panel. The panel reports whether the network is
trained, because random weights and trained weights look identical for the first
few seconds and completely different after.

## What is still missing for serious training

Two things, both deliberately out of scope so far:

1. **A headless, accelerated mode.** Training needs thousands of episodes and the
   game currently runs at 60fps with a window open. This wants a loop that steps
   the simulation without rendering and without sleeping — worth its own
   proposal, since `PlayingState` assumes a render target exists in several
   places.
2. **Determinism.** `ReplayRecorder`'s header already documents that the
   simulation is not reproducible from inputs alone (float physics, a mutating
   entity list, strategies reading a singleton). That is survivable for
   model-free RL, which only needs transitions, but it rules out anything that
   needs to replay a trajectory exactly.
