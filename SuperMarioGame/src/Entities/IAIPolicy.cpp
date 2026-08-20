#include "Entities/IAIPolicy.hpp"

std::vector<float> AIObservation::toFeatureVector() const {
    std::vector<float> features;
    features.reserve(featureCount());

    // Grid, row-major, kAICellFeatures per cell: the one-hot class (one-hot
    // rather than an ordinal — the states have no meaningful order, and
    // feeding Solid=2, Hazard=3 would tell a network that a hazard is "more"
    // of whatever solid is), then the occupant's velocity. Interleaved per
    // cell so a first-layer unit reading a cell sees WHAT is there and WHERE
    // it is going as one local block.
    for (std::size_t i = 0; i < grid.size(); ++i) {
        for (int state = 0; state < kAICellStateCount; ++state) {
            features.push_back(static_cast<int>(grid[i]) == state ? 1.0f : 0.0f);
        }
        features.push_back(velX[i]);
        features.push_back(velY[i]);
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
    features.push_back(onWall ? 1.0f : 0.0f);
    features.push_back(powerTier);
    features.push_back(invincibility);

    return features;
}
