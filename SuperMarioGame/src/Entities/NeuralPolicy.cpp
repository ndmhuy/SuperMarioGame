#include "Entities/NeuralPolicy.hpp"
#include "Entities/NeuralNet.hpp"

#include "nn/Core/Device.hpp"
#include "nn/Module/Activation.hpp"
#include "nn/Module/Linear.hpp"
#include "nn/Module/Sequential.hpp"
#include "nn/Tensor/Tensor.hpp"
#include "nn/Training/Checkpoint.hpp"
#include "nn/Autograd/NoGrad.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

// This is the ONLY game translation unit that includes nn/, and it is compiled
// as C++20 (CMakeLists.txt §16b) because nn/Tensor/Tensor.hpp uses concepts.
// NeuralPolicy.hpp is PIMPL'd so nothing else in the C++17 game ever sees them.
//
// Why nn:: rather than the hand-rolled forward pass this file used to hold:
// training now runs inside the game so it can be visualised as it happens
// (docs/mapgen_gan_rl_plan.md §2d), and a training loop needs gradients. The
// old std::vector<Layer> math could do a forward pass and nothing else; the
// framework brings a real autograd engine, optimizers and checkpointing that
// already exist and are already tested.

void NeuralPolicy::NetDeleter::operator()(Net* net) const { delete net; }

namespace {

// Sidecar metadata written next to a checkpoint. The binary holds floats and
// nothing else, so on its own it cannot say what observation layout it was
// trained against — which is exactly the silent-misalignment failure
// kAIObservationVersion exists to prevent.
std::string sidecarPath(const std::string& weightsPath) {
    return weightsPath + ".meta.json";
}

} // namespace

NeuralPolicy::NeuralPolicy(const std::vector<int>& hiddenLayers, unsigned seed) {
    build(hiddenLayers, seed);
}

NeuralPolicy::~NeuralPolicy() = default;

void NeuralPolicy::build(const std::vector<int>& hiddenLayers, unsigned seed) {
    // nn::Linear seeds itself with Xavier initialisation; the seed argument is
    // kept in the signature because callers (and the old implementation)
    // reasonably expect reproducibility to be expressible here.
    (void)seed;

    auto net = std::unique_ptr<Net, NetDeleter>(new Net());
    net->hidden = hiddenLayers;

    int previous = static_cast<int>(AIObservation::featureCount());
    for (const int width : hiddenLayers) {
        net->model.add(std::make_unique<nn::Linear>(previous, width));
        // Tanh, matching what this policy has always used for hidden layers:
        // the observation is normalised to [-1, 1] and a zero-centred
        // activation keeps the hidden representation in that same world.
        net->model.add(nn::Activation::Tanh());
        previous = width;
    }
    net->model.add(std::make_unique<nn::Linear>(previous, kActionBits));
    // Sigmoid, NOT softmax: a real action is a *set* of buttons — running right
    // while jumping is three at once — and one-of-N cannot express that.
    net->model.add(nn::Activation::Sigmoid());

    m_net = std::move(net);
    m_hiddenLayers = hiddenLayers;
    m_trained = false;

    std::cout << "[NeuralPolicy] Built " << AIObservation::featureCount()
              << " inputs -> " << hiddenLayers.size() << " hidden layer(s) -> "
              << kActionBits << " buttons. Untrained until load() or training."
              << std::endl;
}

AIAction NeuralPolicy::decide(const AIObservation& observation) {
    AIAction action;
    if (!m_net) return action;

    const std::vector<float> features = observation.toFeatureVector();

    // Inference only: no graph, no gradients. Without this every decide() call
    // during play would build an autograd graph nobody backpropagates and leak
    // it for the lifetime of the episode.
    nn::NoGrad noGrad;

    nn::Tensor input(features, {1, static_cast<int>(features.size())});
    nn::Tensor output = m_net->model.forward(input);

    m_lastOutputs.assign(static_cast<std::size_t>(kActionBits), 0.0f);
    for (int i = 0; i < kActionBits && i < output.size(); ++i) {
        m_lastOutputs[static_cast<std::size_t>(i)] = output.flat(i);
    }

    // Multi-label: each button is its own independent decision — sampled from
    // its probability when stochastic acting is on (the policy as trained),
    // thresholded against its calibrated cut otherwise (see setStochastic and
    // setThresholds).
    auto press = [&](std::size_t i) {
        return m_stochastic ? m_unit(m_sampleRng) < m_lastOutputs[i]
                            : m_lastOutputs[i] > m_thresholds[i];
    };
    action.moveLeft    = press(0);
    action.moveRight   = press(1);
    action.jump        = press(2);
    action.run         = press(3);
    action.crouch      = press(4);
    action.shoot       = press(5);
    action.groundPound = press(6);

    // Left and right together is a real thing a human can press and it resolves
    // to standing still, but it also lets an undertrained network sit motionless
    // and never be corrected. The stronger output wins.
    if (action.moveLeft && action.moveRight) {
        if (m_lastOutputs[0] >= m_lastOutputs[1]) action.moveRight = false;
        else                                      action.moveLeft = false;
    }
    return action;
}

void NeuralPolicy::setStochastic(bool enabled, unsigned seed) {
    m_stochastic = enabled;
    m_sampleRng.seed(seed);
}

void NeuralPolicy::setThresholds(const std::vector<float>& thresholds) {
    if (thresholds.size() == static_cast<std::size_t>(kActionBits)) {
        m_thresholds = thresholds;
    }
}

bool NeuralPolicy::saveCheckpoint(const std::string& path) const {
    if (!m_net) return false;

    if (!nn::Checkpoint::save(path, m_net->model.parameters())) {
        std::cerr << "[NeuralPolicy] Could not write checkpoint '" << path << "'."
                  << std::endl;
        return false;
    }

    nlohmann::json meta;
    meta["observationVersion"] = kAIObservationVersion;
    meta["featureCount"] = AIObservation::featureCount();
    meta["hiddenLayers"] = m_hiddenLayers;
    meta["actionBits"] = kActionBits;
    meta["thresholds"] = m_thresholds;

    std::ofstream file(sidecarPath(path));
    if (!file) {
        std::cerr << "[NeuralPolicy] Wrote weights but could not write "
                  << sidecarPath(path) << "; the checkpoint is unusable without "
                     "it, because nothing else records what it was trained against."
                  << std::endl;
        return false;
    }
    file << meta.dump(2) << std::endl;
    return true;
}

bool NeuralPolicy::load(const std::string& path) {
    std::ifstream metaFile(sidecarPath(path));
    if (!metaFile.is_open()) {
        std::cerr << "[NeuralPolicy] No metadata beside '" << path
                  << "'. Refusing to load: without it there is no way to know "
                     "what observation layout these weights expect." << std::endl;
        return false;
    }

    std::vector<int> hidden;
    try {
        nlohmann::json meta;
        metaFile >> meta;

        const int version = meta.value("observationVersion", -1);
        if (version != kAIObservationVersion) {
            std::cerr << "[NeuralPolicy] Weights were trained against observation "
                         "version " << version << ", this build is version "
                      << kAIObservationVersion << ". Refusing to load: the input "
                         "layer would be misaligned and the policy would act "
                         "confidently and arbitrarily." << std::endl;
            return false;
        }

        const std::size_t features = meta.value("featureCount", std::size_t{0});
        if (features != AIObservation::featureCount()) {
            std::cerr << "[NeuralPolicy] Weights expect " << features
                      << " features, the game produces "
                      << AIObservation::featureCount() << "." << std::endl;
            return false;
        }
        hidden = meta.value("hiddenLayers", std::vector<int>{64, 32});
        m_thresholds = meta.value("thresholds",
                                  std::vector<float>(static_cast<std::size_t>(kActionBits), 0.5f));
        if (m_thresholds.size() != static_cast<std::size_t>(kActionBits)) {
            m_thresholds.assign(static_cast<std::size_t>(kActionBits), 0.5f);
        }
    } catch (const std::exception& error) {
        std::cerr << "[NeuralPolicy] Malformed metadata for '" << path << "': "
                  << error.what() << std::endl;
        return false;
    }

    // Rebuild to the recorded shape before restoring, so the parameter list the
    // checkpoint is poured into has exactly the geometry it was saved from.
    build(hidden, 1234u);

    if (!nn::Checkpoint::load(path, m_net->model.parameters())) {
        std::cerr << "[NeuralPolicy] Could not read weights '" << path << "'."
                  << std::endl;
        return false;
    }

    m_trained = true;
    std::cout << "[NeuralPolicy] Loaded weights from " << path << " ("
              << hidden.size() << " hidden layer(s))." << std::endl;
    return true;
}
