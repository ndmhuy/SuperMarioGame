#pragma once

#include "Entities/IAIPolicy.hpp"

#include <memory>
#include <random>
#include <string>
#include <vector>

// A feed-forward network in the IAIPolicy slot: inference only.
//
// This is the drop-in the seam was built for. It replaces HeuristicPolicy
// without AIController, PlayingState or anything else changing, because the
// controller only ever asked for an AIAction given an AIObservation.
//
// Scope, stated plainly
// ---------------------
// Training does NOT happen here, and deliberately so. A gradient implementation
// living inside a game loop would be slow, untestable and duplicated work: the
// tooling for that already exists in Python. What lives here is the half that
// has to be in the game — encode the observation, run a forward pass, emit
// buttons — plus the two things training needs from the game, which are the
// reward signal (RewardTracker) and the transition log (ExperienceLog).
//
// The intended loop is therefore:
//   1. play, with ExperienceLog recording (observation, action, reward) rows;
//   2. train offline against those rows, in whatever framework;
//   3. export weights as JSON in the layout load() expects;
//   4. load them here and play again.
//
// Architecture: an MLP of arbitrary depth, tanh on the hidden layers, sigmoid on
// the output. Seven outputs, one per button, thresholded at 0.5 — a multi-label
// head rather than a softmax over one-of-N, because a real action is a *set* of
// buttons (running right while jumping is three of them at once) and a softmax
// cannot express that.
class NeuralPolicy : public IAIPolicy {
public:
    // The nn::Sequential behind this policy, kept incomplete on purpose — see
    // the PIMPL note on m_net below.
    struct Net;

    // Untrained: random small weights for the given hidden layer widths. Plays
    // badly, on purpose — an untrained network that looked competent would mean
    // the observation was not actually reaching it.
    explicit NeuralPolicy(const std::vector<int>& hiddenLayers = {64, 32},
                          unsigned seed = 1234u);

    // Out-of-line: the PIMPL'd network is an incomplete type in this header.
    ~NeuralPolicy() override;

    AIAction decide(const AIObservation& observation) override;
    const char* name() const override { return "NEURAL"; }
    void reset() override {}

    // Read weights from `path`. The file must declare the observation version it
    // was trained against and it must match kAIObservationVersion; a mismatched
    // input layer produces a policy that acts confidently and arbitrarily.
    //
    // Expected JSON:
    //   {
    //     "observationVersion": 1,
    //     "layers": [
    //       { "weights": [[...], ...], "biases": [...] },   // [out][in]
    //       ...
    //     ]
    //   }
    // The first layer's input width must be AIObservation::featureCount() and
    // the last layer's output width must be kActionBits.
    bool load(const std::string& path);

    // Per-button decision thresholds. 0.5 is the right cut only for balanced
    // classes; jump is pressed on ~3% of frames, and its calibrated
    // probability can sit entirely below 0.5 — a policy that completed levels
    // when SAMPLED stalled at the first obstacle when THRESHOLDED, on every
    // level, at the same x. The trainer calibrates these (midpoint between
    // the running mean prediction on positive and on negative labels) and
    // they persist in the checkpoint sidecar.
    void setThresholds(const std::vector<float>& thresholds);
    const std::vector<float>& thresholds() const { return m_thresholds; }

    // Stochastic acting: sample each button ~ Bernoulli(p) instead of
    // thresholding. The trained object IS a distribution over button sets —
    // during training the learner acted by sampling, and the same checkpoint
    // that reached flags 33 times when sampled froze at the first staircase
    // when argmaxed (jump output 0.24, threshold forever unmet, zero noise to
    // break the loop). Deterministic projection of a stochastic policy is a
    // different policy; this evaluates the one that was trained. Seeded, so
    // evaluations reproduce.
    void setStochastic(bool enabled, unsigned seed = 1234u);

    // True once load() has succeeded. The HUD and the dev panel say so, because
    // "the network is playing" and "random weights are playing" look identical
    // from the outside for the first few seconds and completely different after.
    bool isTrained() const { return m_trained; }

    // Last forward pass's raw outputs, for the dev overlay. Seeing which button
    // sat at 0.49 is most of debugging a policy that will not jump.
    const std::vector<float>& lastOutputs() const { return m_lastOutputs; }

    // One output per button, in AIAction declaration order.
    static constexpr int kActionBits = 7;

    // --- Training access (see docs/mapgen_gan_rl_plan.md §2d) ----------------
    //
    // Training runs inside the game so it can be watched, which means the
    // trainer needs the network itself, not just its decisions. These are the
    // whole surface it needs, and they are deliberately opaque: `Net` is an
    // incomplete type out here, so a caller can hold and pass a network without
    // this header ever pulling in nn/.

    // The underlying nn::Sequential. Null until build() or load(). Only the
    // trainer (a C++20 translation unit) can do anything with it.
    Net* network() const { return m_net.get(); }

    // Construct a fresh randomly-initialised network of the given shape.
    // Called by the trainer before a run; decide() falls back to no-op until
    // this or load() has happened.
    void build(const std::vector<int>& hiddenLayers, unsigned seed = 1234u);

    // Persist / restore through nn::Training::Checkpoint (binary), alongside a
    // sidecar JSON recording the observation version and layer shape — the
    // binary alone cannot tell you what it was trained against.
    bool saveCheckpoint(const std::string& path) const;

private:
    // PIMPL, and the reason for it: nn/Tensor/Tensor.hpp uses C++20 concepts,
    // while this game is C++17 by AGENTS.md directive 5 and stays that way.
    // Hiding the network behind an incomplete type keeps every C++17 include
    // path in the game free of C++20 constructs; only NeuralPolicy.cpp and the
    // trainer are compiled as C++20 (see CMakeLists.txt §16b).
    struct NetDeleter { void operator()(Net*) const; };

    std::unique_ptr<Net, NetDeleter> m_net;
    std::vector<float> m_lastOutputs;
    std::vector<float> m_thresholds = std::vector<float>(7, 0.5f);
    bool m_stochastic = false;
    std::mt19937 m_sampleRng{1234u};
    std::uniform_real_distribution<float> m_unit{0.0f, 1.0f};
    std::vector<int> m_hiddenLayers;
    bool m_trained = false;
};
