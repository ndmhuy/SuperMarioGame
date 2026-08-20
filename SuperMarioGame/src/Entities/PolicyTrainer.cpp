#include "Entities/PolicyTrainer.hpp"
#include "Entities/NeuralPolicy.hpp"

#include "Entities/NeuralNet.hpp"
#include "Entities/PolicyLosses.hpp"

#include "nn/Loss/Loss.hpp"
#include "nn/Module/Sequential.hpp"
#include "nn/Optim/SGD.hpp"
#include "nn/Tensor/Tensor.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cmath>

// C++20 translation unit (CMakeLists.txt §16b), like NeuralPolicy.cpp, because
// it includes nn/. PolicyTrainer.hpp is PIMPL'd so C++17 game code — including
// TrainingState — can use it without ever seeing a concept.

struct PolicyTrainer::Impl {
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
    m_loss = std::make_unique<WeightedBernoulliLoss>();
    // Seeded at 0.5 rather than 0: an unseen button starts neutral and the
    // running estimate moves it, instead of starting at an extreme weight.
    m_pressRate.assign(NeuralPolicy::kActionBits, 0.5f);
    m_predWhenPressed.assign(static_cast<std::size_t>(NeuralPolicy::kActionBits), 0.7f);
    m_predWhenNot.assign(static_cast<std::size_t>(NeuralPolicy::kActionBits), 0.3f);
    m_buttonCorrect.assign(NeuralPolicy::kActionBits, 0);
    m_buttonTotal.assign(NeuralPolicy::kActionBits, 0);
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
    // Update the running press-rate estimate BEFORE weighting, so the weights
    // reflect the distribution including this sample.
    constexpr float kRateDecay = 0.999f;
    for (int i = 0; i < NeuralPolicy::kActionBits; ++i) {
        const float pressed = target[static_cast<std::size_t>(i)];
        m_pressRate[static_cast<std::size_t>(i)] =
            kRateDecay * m_pressRate[static_cast<std::size_t>(i)] + (1.0f - kRateDecay) * pressed;
    }

    // Class-balanced weights. A button pressed with probability r contributes
    // 0.5/r when pressed and 0.5/(1-r) when not, so both classes carry equal
    // total weight regardless of how rare one of them is.
    std::vector<float> weights(NeuralPolicy::kActionBits, 1.0f);
    if (m_config.balanceClasses) {
        for (int i = 0; i < NeuralPolicy::kActionBits; ++i) {
            const std::size_t k = static_cast<std::size_t>(i);
            const float rate = std::clamp(m_pressRate[k], m_config.minPressRate,
                                          1.0f - m_config.minPressRate);
            weights[k] = target[k] > 0.5f ? 0.5f / rate : 0.5f / (1.0f - rate);
        }
    }
    m_loss->setWeights(weights);

    // Aggregate first, so the new sample is eligible for its own update.
    // Reservoir (Algorithm R): the buffer stays a UNIFORM sample of every
    // transition ever seen, so no level and no training era crowds out the
    // others. Quantize on the way in.
    auto quantize = [](const std::vector<float>& values) {
        std::vector<std::int8_t> q(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            q[i] = static_cast<std::int8_t>(
                std::lround(std::clamp(values[i], -1.0f, 1.0f) * 127.0f));
        }
        return q;
    };
    ++m_samplesSeen;
    if (m_aggregate.size() < m_config.aggregateCapacity) {
        m_aggregate.push_back(Sample{quantize(features), target});
    } else if (!m_aggregate.empty()) {
        const std::size_t j = static_cast<std::size_t>(
            nextRandom() * static_cast<float>(m_samplesSeen));
        if (j < m_aggregate.size()) {
            m_aggregate[j] = Sample{quantize(features), target};
        }
    }

    nn::Tensor prediction = net->model.forward(input);
    // forward(), NOT compute(). compute() returns a bare loss value with no
    // GradFn attached; only forward() wires the loss into the autograd graph.
    // Calling compute() here made backward() a no-op, so the network trained
    // for 79,000 samples with the loss frozen at 0.13 and agreement stuck at
    // 47% — it looked like it was running and was learning nothing.
    nn::Loss* criterion = static_cast<nn::Loss*>(m_loss->handle());
    nn::Tensor lossTensor = criterion->forward(prediction, labels);

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
        // Threshold calibration data: the running mean prediction on each
        // side of the label. Slow EMA — thresholds should drift with the
        // policy, not chase one episode.
        constexpr float kCalibrationRate = 0.002f;
        float& classMean = expected ? m_predWhenPressed[static_cast<std::size_t>(i)]
                                    : m_predWhenNot[static_cast<std::size_t>(i)];
        classMean += kCalibrationRate * (p - classMean);
        if (predicted == expected) {
            ++m_episodeButtonsCorrect;
            ++m_buttonCorrect[static_cast<std::size_t>(i)];
        }
        ++m_episodeButtonsTotal;
        ++m_buttonTotal[static_cast<std::size_t>(i)];
    }

    // Replay from the aggregate, as ONE minibatch.
    //
    // Sequential single-sample steps were the first attempt and were wrong: 16
    // full-strength updates per decision is ~17x the intended learning rate,
    // and training-time progress fell 31% -> 1.4% accordingly. A real minibatch
    // averages the gradients (the loss divides by element count), so the update
    // magnitude is right, and it is also 2.45x cheaper per sample.
    if (!m_aggregate.empty() && m_config.replayBatch > 0) {
        const int batch = std::min<int>(m_config.replayBatch,
                                        static_cast<int>(m_aggregate.size()));
        const std::size_t width = features.size();
        std::vector<float> batchFeatures;
        std::vector<float> batchTargets;
        batchFeatures.reserve(width * static_cast<std::size_t>(batch));
        batchTargets.reserve(static_cast<std::size_t>(NeuralPolicy::kActionBits * batch));

        for (int r = 0; r < batch; ++r) {
            const std::size_t pick = std::min<std::size_t>(
                static_cast<std::size_t>(nextRandom() * static_cast<float>(m_aggregate.size())),
                m_aggregate.size() - 1);
            const Sample& sample = m_aggregate[pick];
            for (const std::int8_t q : sample.features) {
                batchFeatures.push_back(static_cast<float>(q) / 127.0f);
            }
            batchTargets.insert(batchTargets.end(), sample.target.begin(),
                                sample.target.end());
        }

        // Class weights stay per button; the loss indexes them by column.
        m_loss->setWeights(weights);

        nn::Tensor rIn(batchFeatures, {batch, static_cast<int>(width)});
        nn::Tensor rTarget(batchTargets, {batch, NeuralPolicy::kActionBits});
        nn::Tensor rPred = net->model.forward(rIn);
        nn::Tensor rLoss = criterion->forward(rPred, rTarget);
        m_impl->optimizer.zeroGrad(net->model.parameters());
        rLoss.backward();
        m_impl->optimizer.step(net->model.parameters());
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
    // jump_agreement is broken out because the aggregate hid a total failure:
    // 99.9% overall while the jump button was at 0%.
    file << "episode,phase,samples,mean_loss,agreement,jump_agreement,jump_rate,return,outcome\n";
    m_logPath = path;
    m_logOpen = true;
}

AIAction PolicyTrainer::sampleAction(const AIObservation& observation) {
    AIAction action;
    auto* net = m_policy->network();
    if (!net) return action;

    // Still holding the previous action: reuse it, and let its reward keep
    // accruing to the transition that chose it. Re-sampling every frame gave
    // motion so incoherent that no return could be attributed to any decision.
    if (m_repeatLeft > 0) {
        --m_repeatLeft;
        return m_heldAction;
    }

    const std::vector<float> features = observation.toFeatureVector();

    std::vector<float> probabilities(NeuralPolicy::kActionBits, 0.0f);
    {
        // Inference only while acting; the gradient for this step is computed
        // later, in runReinforceUpdate(), once the return is known.
        nn::NoGrad noGrad;
        nn::Tensor input(features, {1, static_cast<int>(features.size())});
        nn::Tensor output = net->model.forward(input);
        for (int i = 0; i < NeuralPolicy::kActionBits && i < output.size(); ++i) {
            probabilities[static_cast<std::size_t>(i)] = output.flat(i);
        }
    }
    m_lastPrediction = probabilities;

    // Sample rather than threshold. A deterministic policy has no gradient
    // signal to learn FROM: REINFORCE estimates the gradient by comparing the
    // return of actions it actually took against the baseline, so the actions
    // have to vary.
    std::vector<float> taken(NeuralPolicy::kActionBits, 0.0f);
    for (int i = 0; i < NeuralPolicy::kActionBits; ++i) {
        const bool pressed = nextRandom() < probabilities[static_cast<std::size_t>(i)];
        taken[static_cast<std::size_t>(i)] = pressed ? 1.0f : 0.0f;
    }

    action.moveLeft    = taken[0] > 0.5f;
    action.moveRight   = taken[1] > 0.5f;
    action.jump        = taken[2] > 0.5f;
    action.run         = taken[3] > 0.5f;
    action.crouch      = taken[4] > 0.5f;
    action.shoot       = taken[5] > 0.5f;
    action.groundPound = taken[6] > 0.5f;
    if (action.moveLeft && action.moveRight) {
        if (probabilities[0] >= probabilities[1]) action.moveRight = false;
        else                                      action.moveLeft = false;
        taken[0] = action.moveLeft ? 1.0f : 0.0f;
        taken[1] = action.moveRight ? 1.0f : 0.0f;
    }

    m_episode.push_back(Transition{features, taken, 0.0f});
    ++m_samples;
    m_heldAction = action;
    m_repeatLeft = std::max(m_config.actionRepeat - 1, 0);
    return action;
}

void PolicyTrainer::recordReward(float reward) {
    if (!m_episode.empty()) m_episode.back().reward += reward;
}

void PolicyTrainer::runReinforceUpdate() {
    auto* net = m_policy->network();
    if (!net || m_episode.empty()) { m_episode.clear(); return; }

    // Discounted return following each action: G_t = r_t + gamma * G_{t+1},
    // accumulated backwards so each is computed once.
    const std::size_t n = m_episode.size();
    std::vector<float> returns(n, 0.0f);
    float running = 0.0f;
    for (std::size_t k = n; k-- > 0;) {
        running = m_episode[k].reward + m_config.discount * running;
        returns[k] = running;
    }
    m_lastEpisodeReturn = returns.empty() ? 0.0f : returns.front();

    // Standardise. Without this the update size tracks the raw magnitude of the
    // game's reward, so a level that happens to pay more would take larger
    // steps for no principled reason.
    double sum = 0.0;
    for (float g : returns) sum += g;
    const float mean = static_cast<float>(sum / static_cast<double>(n));
    double variance = 0.0;
    for (float g : returns) variance += (g - mean) * (g - mean);
    const float stdev = std::sqrt(static_cast<float>(variance / static_cast<double>(n)));

    if (!m_baselineSeeded) { m_returnBaseline = mean; m_baselineSeeded = true; }
    else                   { m_returnBaseline = 0.95f * m_returnBaseline + 0.05f * mean; }

    // No spread means no information about which actions were better, and
    // standardising by a near-zero spread manufactures advantages out of
    // rounding error. Skip rather than train on noise.
    if (stdev < m_config.minReturnSpread) {
        ++m_skippedFlatEpisodes;
        m_episodeLossSum = 0.0;
        m_episodeSamples = m_episode.size();
        m_episode.clear();
        m_repeatLeft = 0;
        return;
    }

    nn::Loss* criterion = static_cast<nn::Loss*>(m_loss->handle());
    double lossSum = 0.0;

    for (std::size_t k = 0; k < n; ++k) {
        const float advantage = (returns[k] - mean) / stdev;
        // Uniform across buttons: the return credits the whole action, and
        // REINFORCE has no way to attribute it to individual buttons. The SIGN
        // is the learning signal — a positive advantage raises the probability
        // of exactly the action that was taken, a negative one lowers it.
        m_loss->setWeights(std::vector<float>(NeuralPolicy::kActionBits, advantage));

        nn::Tensor input(m_episode[k].features,
                         {1, static_cast<int>(m_episode[k].features.size())});
        nn::Tensor taken(m_episode[k].action, {1, NeuralPolicy::kActionBits});

        // Aggregation belongs to the imitation path only: REINFORCE's target is
        // the action the policy itself sampled, which is not a supervision
        // label and must not be replayed as one.
        nn::Tensor prediction = net->model.forward(input);
        nn::Tensor lossTensor = criterion->forward(prediction, taken);

        m_impl->optimizer.zeroGrad(net->model.parameters());
        lossTensor.backward();
        m_impl->optimizer.step(net->model.parameters());

        lossSum += lossTensor.size() > 0 ? lossTensor.flat(0) : 0.0f;
    }

    m_lastLoss = static_cast<float>(lossSum / static_cast<double>(n));
    m_episodeLossSum = lossSum;
    m_episodeSamples = n;
    m_episode.clear();
    m_repeatLeft = 0;
}

std::vector<float> PolicyTrainer::buttonAgreement() const {
    std::vector<float> out(m_buttonTotal.size(), 0.0f);
    for (std::size_t i = 0; i < m_buttonTotal.size(); ++i) {
        if (m_buttonTotal[i] > 0) {
            out[i] = static_cast<float>(m_buttonCorrect[i]) /
                     static_cast<float>(m_buttonTotal[i]);
        }
    }
    return out;
}

void PolicyTrainer::endEpisode(const char* outcome) {
    if (m_mode == Mode::Reinforce) runReinforceUpdate();

    // Calibrated per-button thresholds: the midpoint between what the network
    // says on pressed frames and on unpressed frames. Clamped away from the
    // rails so a button can always fire and always rest.
    if (m_policy) {
        std::vector<float> thresholds(static_cast<std::size_t>(NeuralPolicy::kActionBits));
        for (std::size_t i = 0; i < thresholds.size(); ++i) {
            thresholds[i] = std::clamp(
                0.5f * (m_predWhenPressed[i] + m_predWhenNot[i]), 0.05f, 0.95f);
        }
        m_policy->setThresholds(thresholds);
    }

    // Index 2 is the jump button, per AIAction declaration order.
    const std::vector<float> perButton = buttonAgreement();
    const float jumpAgreement = perButton.size() > 2 ? perButton[2] : 0.0f;

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
    std::fill(m_buttonCorrect.begin(), m_buttonCorrect.end(), 0);
    std::fill(m_buttonTotal.begin(), m_buttonTotal.end(), 0);
    ++m_episodes;

    m_config.beta = std::max(m_config.minBeta,
                             m_config.beta - m_config.betaDecayPerEpisode);

    // Hand over from imitation to reinforcement once the policy is competent
    // enough that policy-gradient updates have something to improve on.
    if (m_mode == Mode::Imitation && m_episodes >= m_config.imitationEpisodes) {
        m_mode = Mode::Reinforce;
        m_impl->optimizer = nn::SGD(m_config.reinforceLearningRate);
        m_config.beta = 0.0f;   // the teacher no longer drives; it is done
        std::cout << "[PolicyTrainer] Imitation phase complete after "
                  << m_episodes << " episodes. Switching to REINFORCE (lr "
                  << m_config.reinforceLearningRate << ")." << std::endl;
    }

    if (m_logOpen && !m_lossHistory.empty()) {
        std::ofstream file(m_logPath, std::ios::app);
        if (file) {
            file << m_episodes << ','
                 << (m_mode == Mode::Reinforce ? "reinforce" : "imitation") << ','
                 << m_samples << ','
                 << m_lossHistory.back() << ',' << m_agreementHistory.back() << ','
                 << jumpAgreement << ',' << m_pressRate[2] << ','
                 << m_lastEpisodeReturn << ',' << (outcome ? outcome : "") << '\n';
        }
    }
}
