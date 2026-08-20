#include "Entities/PolicyTrainer.hpp"
#include "Entities/NeuralPolicy.hpp"

#include "Entities/NeuralNet.hpp"

#include "nn/Loss/MSELoss.hpp"
#include "nn/Module/Sequential.hpp"
#include "nn/Optim/SGD.hpp"
#include "nn/Tensor/Tensor.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>

// C++20 translation unit (CMakeLists.txt §16b), like NeuralPolicy.cpp, because
// it includes nn/. PolicyTrainer.hpp is PIMPL'd so C++17 game code — including
// TrainingState — can use it without ever seeing a concept.

struct PolicyTrainer::Impl {
    nn::MSELoss loss;
    nn::SGD optimizer;
    explicit Impl(float lr) : optimizer(lr) {}
};

void PolicyTrainer::ImplDeleter::operator()(Impl* impl) const { delete impl; }

namespace {

// The seven buttons as a target vector, in AIAction declaration order — the
// same order NeuralPolicy::decide() decodes, and the same order
// PlayerFramePacket records. Keeping one order across all three is what lets a
// recorded human session be training data without any translation.
std::vector<float> actionToTarget(const AIAction& action) {
    return {
        action.moveLeft    ? 1.0f : 0.0f,
        action.moveRight   ? 1.0f : 0.0f,
        action.jump        ? 1.0f : 0.0f,
        action.run         ? 1.0f : 0.0f,
        action.crouch      ? 1.0f : 0.0f,
        action.shoot       ? 1.0f : 0.0f,
        action.groundPound ? 1.0f : 0.0f,
    };
}

} // namespace

PolicyTrainer::PolicyTrainer(NeuralPolicy& policy)
    : PolicyTrainer(policy, Config()) {}

PolicyTrainer::PolicyTrainer(NeuralPolicy& policy, const Config& config)
    : m_impl(new Impl(config.learningRate)), m_policy(&policy), m_config(config) {
    m_lastPrediction.assign(NeuralPolicy::kActionBits, 0.0f);
}

PolicyTrainer::~PolicyTrainer() = default;

float PolicyTrainer::nextRandom() {
    // xorshift32 — deterministic across runs so a training curve is
    // reproducible, which matters when comparing two hyperparameter choices.
    m_rngState ^= m_rngState << 13;
    m_rngState ^= m_rngState >> 17;
    m_rngState ^= m_rngState << 5;
    return static_cast<float>(m_rngState & 0xFFFFFFu) / static_cast<float>(0xFFFFFF);
}

bool PolicyTrainer::teacherDrives() {
    return nextRandom() < m_config.beta;
}

float PolicyTrainer::learn(const AIObservation& observation,
                           const AIAction& teacherAction) {
    auto* net = m_policy->network();
    if (!net) return 0.0f;

    const std::vector<float> features = observation.toFeatureVector();
    const std::vector<float> target = actionToTarget(teacherAction);

    // Fresh tensors every step. This is recommended practice rather than a
    // workaround for a known defect: an earlier comment here claimed that
    // reusing a leaf tensor grows its graph without bound, citing a 200-step
    // run that "took minutes". That was wrong — the minutes were the framework
    // recompiling, misread as run time. Re-measured, reuse holds a flat
    // 0.010 ms/step over 100 steps, and the nn:: maintainer could not reproduce
    // any growth either. Fresh tensors are still what this loop wants, because
    // each decision is genuinely a new sample.
    nn::Tensor input(features, {1, static_cast<int>(features.size())});
    nn::Tensor labels(target, {1, NeuralPolicy::kActionBits});

    // Forward WITH the graph this time — unlike decide(), which runs under
    // nn::NoGrad. This is the one place gradients are wanted.
    nn::Tensor prediction = net->model.forward(input);
    // forward(), NOT compute(). compute() returns a bare loss value with no
    // GradFn attached; only forward() wires the loss into the autograd graph.
    // Calling compute() here made backward() a no-op, so the network trained
    // for 79,000 samples with the loss frozen at 0.13 and agreement stuck at
    // 47% — it looked like it was running and was learning nothing.
    nn::Tensor lossTensor = m_impl->loss.forward(prediction, labels);

    m_impl->optimizer.zeroGrad(net->model.parameters());
    lossTensor.backward();
    m_impl->optimizer.step(net->model.parameters());

    m_lastLoss = lossTensor.size() > 0 ? lossTensor.flat(0) : 0.0f;

    // Metrics: record what the network predicted BEFORE this step, which is the
    // honest measure of its current skill.
    m_lastPrediction.assign(NeuralPolicy::kActionBits, 0.0f);
    for (int i = 0; i < NeuralPolicy::kActionBits && i < prediction.size(); ++i) {
        const float p = prediction.flat(i);
        m_lastPrediction[static_cast<std::size_t>(i)] = p;
        // Agreement is measured after thresholding, because that is what the
        // game actually executes — a 0.49 that should be 1.0 is a wrong button,
        // however small its contribution to the loss.
        const bool predicted = p > 0.5f;
        const bool expected = target[static_cast<std::size_t>(i)] > 0.5f;
        if (predicted == expected) ++m_episodeButtonsCorrect;
        ++m_episodeButtonsTotal;
    }

    m_episodeLossSum += m_lastLoss;
    ++m_episodeSamples;
    ++m_samples;
    return m_lastLoss;
}

float PolicyTrainer::episodeAgreement() const {
    if (m_episodeButtonsTotal == 0) return 0.0f;
    return static_cast<float>(m_episodeButtonsCorrect) /
           static_cast<float>(m_episodeButtonsTotal);
}

void PolicyTrainer::openLog(const std::string& path) {
    std::error_code ignored;
    const std::filesystem::path out(path);
    if (out.has_parent_path()) {
        std::filesystem::create_directories(out.parent_path(), ignored);
    }
    // Truncate and write a header: a run is one experiment, and appending a
    // second run's rows onto the first produces a curve that means nothing.
    std::ofstream file(path, std::ios::trunc);
    if (!file) return;
    file << "episode,samples,mean_loss,agreement,beta,outcome\n";
    m_logPath = path;
    m_logOpen = true;
}

void PolicyTrainer::endEpisode(const char* outcome) {
    if (m_episodeSamples > 0) {
        m_lossHistory.push_back(
            static_cast<float>(m_episodeLossSum / static_cast<double>(m_episodeSamples)));
        m_agreementHistory.push_back(episodeAgreement());
        // Bounded: the overlay plots a window, and an unbounded vector in a
        // process meant to run for hours is a leak with extra steps.
        constexpr std::size_t kMaxHistory = 512;
        if (m_lossHistory.size() > kMaxHistory) {
            m_lossHistory.erase(m_lossHistory.begin());
            m_agreementHistory.erase(m_agreementHistory.begin());
        }
    }

    m_episodeLossSum = 0.0;
    m_episodeSamples = 0;
    m_episodeButtonsCorrect = 0;
    m_episodeButtonsTotal = 0;
    ++m_episodes;

    m_config.beta = std::max(m_config.minBeta,
                             m_config.beta - m_config.betaDecayPerEpisode);

    if (m_logOpen && !m_lossHistory.empty()) {
        std::ofstream file(m_logPath, std::ios::app);
        if (file) {
            file << m_episodes << ',' << m_samples << ','
                 << m_lossHistory.back() << ',' << m_agreementHistory.back() << ','
                 << m_config.beta << ',' << (outcome ? outcome : "") << '\n';
        }
    }
}
