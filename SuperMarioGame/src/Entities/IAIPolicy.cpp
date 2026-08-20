#include "Entities/IAIPolicy.hpp"

std::vector<float> AIObservation::toFeatureVector() const {
    std::vector<float> features;
    features.reserve(featureCount());

    // Grid, one-hot per cell, row-major. One-hot rather than an ordinal: the six
    // cell states have no meaningful order, and feeding Solid=2, Hazard=3 would
    // tell a network that a hazard is "more" of whatever solid is.
    for (const AICellState cell : grid) {
        for (int state = 0; state < kAICellStateCount; ++state) {
            features.push_back(static_cast<int>(cell) == state ? 1.0f : 0.0f);
        }
    }

    // Scalars, in the order IAIPolicy.hpp documents. Already normalized to
    // [-1, 1] by AIController::scanEnvironment; the booleans join them as 0/1.
    features.push_back(dxToGoal);
    features.push_back(dyToGoal);
    features.push_back(dxToOpponent);
    features.push_back(dyToOpponent);
    features.push_back(vx);
    features.push_back(vy);
    features.push_back(onGround ? 1.0f : 0.0f);
    features.push_back(canJump ? 1.0f : 0.0f);
    features.push_back(isPoweredUp ? 1.0f : 0.0f);

    return features;
}
