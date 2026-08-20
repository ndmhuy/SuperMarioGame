#pragma once

#include "Entities/IAIPolicy.hpp"

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
    // Untrained: random small weights for the given hidden layer widths. Plays
    // badly, on purpose — an untrained network that looked competent would mean
    // the observation was not actually reaching it.
    explicit NeuralPolicy(const std::vector<int>& hiddenLayers = {64, 32},
                          unsigned seed = 1234u);

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

    // True once load() has succeeded. The HUD and the dev panel say so, because
    // "the network is playing" and "random weights are playing" look identical
    // from the outside for the first few seconds and completely different after.
    bool isTrained() const { return m_trained; }

    // Last forward pass's raw outputs, for the dev overlay. Seeing which button
    // sat at 0.49 is most of debugging a policy that will not jump.
    const std::vector<float>& lastOutputs() const { return m_lastOutputs; }

    // One output per button, in AIAction declaration order.
    static constexpr int kActionBits = 7;

private:
    struct Layer {
        // Row-major [out][in].
        std::vector<std::vector<float>> weights;
        std::vector<float> biases;
    };

    std::vector<float> forward(const std::vector<float>& input) const;

    std::vector<Layer> m_layers;
    std::vector<float> m_lastOutputs;
    bool m_trained = false;
};
