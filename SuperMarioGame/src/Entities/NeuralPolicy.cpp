#include "Entities/NeuralPolicy.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <random>

namespace {

float tanhActivation(float x) { return std::tanh(x); }

float sigmoid(float x) {
    // Clamped before exp: a wide untrained layer can produce values large enough
    // to overflow, and inf/nan propagating into the thresholds turns "press
    // nothing" into "press everything".
    if (x < -30.0f) return 0.0f;
    if (x >  30.0f) return 1.0f;
    return 1.0f / (1.0f + std::exp(-x));
}

} // namespace

NeuralPolicy::NeuralPolicy(const std::vector<int>& hiddenLayers, unsigned seed) {
    std::mt19937 rng(seed);

    // Xavier-ish: scaled by the fan-in, so an untrained forward pass produces
    // outputs near 0.5 rather than saturated at 0 or 1.
    auto buildLayer = [&rng](int inputs, int outputs) {
        Layer layer;
        const float limit = 1.0f / std::sqrt(static_cast<float>(std::max(1, inputs)));
        std::uniform_real_distribution<float> dist(-limit, limit);
        layer.weights.resize(static_cast<std::size_t>(outputs));
        for (auto& row : layer.weights) {
            row.resize(static_cast<std::size_t>(inputs));
            for (float& w : row) w = dist(rng);
        }
        layer.biases.assign(static_cast<std::size_t>(outputs), 0.0f);
        return layer;
    };

    int previous = static_cast<int>(AIObservation::featureCount());
    for (const int width : hiddenLayers) {
        m_layers.push_back(buildLayer(previous, width));
        previous = width;
    }
    m_layers.push_back(buildLayer(previous, kActionBits));

    std::cout << "[NeuralPolicy] Untrained network: " << AIObservation::featureCount()
              << " inputs -> " << m_layers.size() << " layers -> " << kActionBits
              << " buttons. Load weights to make it play." << std::endl;
}

std::vector<float> NeuralPolicy::forward(const std::vector<float>& input) const {
    std::vector<float> activations = input;

    for (std::size_t index = 0; index < m_layers.size(); ++index) {
        const Layer& layer = m_layers[index];
        const bool isOutput = (index + 1 == m_layers.size());

        std::vector<float> next(layer.biases.size(), 0.0f);
        for (std::size_t out = 0; out < layer.weights.size(); ++out) {
            float sum = layer.biases[out];
            const std::vector<float>& row = layer.weights[out];
            // Guarded rather than assumed: load() validates shapes, but the
            // constructor and a loaded file are two different sources of truth
            // and a mismatch here would read off the end of the vector.
            const std::size_t width = std::min(row.size(), activations.size());
            for (std::size_t in = 0; in < width; ++in) {
                sum += row[in] * activations[in];
            }
            next[out] = isOutput ? sigmoid(sum) : tanhActivation(sum);
        }
        activations = std::move(next);
    }
    return activations;
}

AIAction NeuralPolicy::decide(const AIObservation& observation) {
    m_lastOutputs = forward(observation.toFeatureVector());

    AIAction action;
    if (m_lastOutputs.size() < static_cast<std::size_t>(kActionBits)) return action;

    // Multi-label: each button is its own independent decision.
    action.moveLeft    = m_lastOutputs[0] > 0.5f;
    action.moveRight   = m_lastOutputs[1] > 0.5f;
    action.jump        = m_lastOutputs[2] > 0.5f;
    action.run         = m_lastOutputs[3] > 0.5f;
    action.crouch      = m_lastOutputs[4] > 0.5f;
    action.shoot       = m_lastOutputs[5] > 0.5f;
    action.groundPound = m_lastOutputs[6] > 0.5f;

    // Left and right together is a real thing a human can press and it resolves
    // to standing still, but it also lets an undertrained network sit motionless
    // and never be corrected. The stronger output wins.
    if (action.moveLeft && action.moveRight) {
        if (m_lastOutputs[0] >= m_lastOutputs[1]) action.moveRight = false;
        else                                      action.moveLeft = false;
    }
    return action;
}

bool NeuralPolicy::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[NeuralPolicy] Cannot open weights '" << path << "'." << std::endl;
        return false;
    }

    try {
        nlohmann::json json;
        file >> json;

        const int version = json.value("observationVersion", -1);
        if (version != kAIObservationVersion) {
            std::cerr << "[NeuralPolicy] Weights were trained against observation "
                         "version " << version << ", this build is version "
                      << kAIObservationVersion << ". Refusing to load: the input "
                         "layer would be misaligned and the policy would act "
                         "confidently and arbitrarily." << std::endl;
            return false;
        }

        std::vector<Layer> layers;
        for (const auto& layerJson : json.at("layers")) {
            Layer layer;
            for (const auto& row : layerJson.at("weights")) {
                layer.weights.push_back(row.get<std::vector<float>>());
            }
            layer.biases = layerJson.at("biases").get<std::vector<float>>();

            if (layer.weights.empty() || layer.weights.size() != layer.biases.size()) {
                std::cerr << "[NeuralPolicy] Layer has " << layer.weights.size()
                          << " weight rows and " << layer.biases.size()
                          << " biases; they must match." << std::endl;
                return false;
            }
            layers.push_back(std::move(layer));
        }

        if (layers.empty()) {
            std::cerr << "[NeuralPolicy] Weight file declares no layers." << std::endl;
            return false;
        }
        if (layers.front().weights.front().size() != AIObservation::featureCount()) {
            std::cerr << "[NeuralPolicy] Input layer expects "
                      << layers.front().weights.front().size() << " features, the game "
                         "produces " << AIObservation::featureCount() << "." << std::endl;
            return false;
        }
        if (layers.back().weights.size() != static_cast<std::size_t>(kActionBits)) {
            std::cerr << "[NeuralPolicy] Output layer has " << layers.back().weights.size()
                      << " units, expected " << kActionBits << " (one per button)."
                      << std::endl;
            return false;
        }

        m_layers = std::move(layers);
        m_trained = true;
        std::cout << "[NeuralPolicy] Loaded " << m_layers.size() << " layers from "
                  << path << "." << std::endl;
        return true;
    } catch (const std::exception& error) {
        std::cerr << "[NeuralPolicy] Malformed weight file '" << path << "': "
                  << error.what() << std::endl;
        return false;
    }
}
