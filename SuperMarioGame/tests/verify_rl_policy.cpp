// Regression harness for the reinforcement-learning seam.
//
// What matters here is not that a network can be constructed — it is that the
// contract between the game and a trainer holds: a fixed-width observation, an
// action space that matches the network's head, a reward that responds to the
// events it claims to, and a transition log a reader can actually parse.

#include "Core/EventBus.hpp"
#include "Entities/AIController.hpp"
#include "Entities/ExperienceLog.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/IAIPolicy.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/NeuralPolicy.hpp"
#include "Entities/RewardTracker.hpp"
#include "Utils/TileMap.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {

AIObservation makeObservation() {
    AIObservation obs;
    obs.grid.fill(AICellState::Empty);
    obs.dxToGoal = 0.5f;
    obs.dxToOpponent = -0.25f;
    obs.vx = 0.75f;
    obs.onGround = true;
    obs.canJump = true;
    return obs;
}

void testFeatureVectorIsFixedWidthAndBounded() {
    std::cout << "testFeatureVectorIsFixedWidthAndBounded..." << std::endl;

    // Every difficulty must produce the same width. This is the property that
    // lets one trained network accept an Easy agent's narrow view and a Hard
    // agent's full-screen view: the unseen cells encode as Unknown rather than
    // shortening the vector.
    AIObservation narrow;
    narrow.grid.fill(AICellState::Unknown);
    AIObservation wide = makeObservation();

    const auto narrowFeatures = narrow.toFeatureVector();
    const auto wideFeatures = wide.toFeatureVector();

    assert(narrowFeatures.size() == AIObservation::featureCount());
    assert(wideFeatures.size() == AIObservation::featureCount());
    assert(narrowFeatures.size() == wideFeatures.size() &&
           "Observation width must not depend on difficulty");

    // Everything normalized: a network trained on [-1, 1] inputs behaves
    // arbitrarily when handed raw pixel counts.
    for (const float value : wideFeatures) {
        assert(value >= -1.0f && value <= 1.0f && "Features must stay in [-1, 1]");
        assert(std::isfinite(value) && "Features must be finite");
    }

    // One-hot really is one-hot: exactly one bit set per cell. v4 interleaves
    // two motion floats after each cell's one-hot, so the stride is
    // kAICellFeatures and only the first kAICellStateCount entries are class
    // bits.
    for (int cell = 0; cell < kAIVisionCells; ++cell) {
        float sum = 0.0f;
        for (int state = 0; state < kAICellStateCount; ++state) {
            sum += wideFeatures[static_cast<std::size_t>(cell) * kAICellFeatures + state];
        }
        assert(std::abs(sum - 1.0f) < 1e-5f && "Each cell must be exactly one-hot");
    }

    std::cout << "  ok (" << AIObservation::featureCount() << " features)" << std::endl;
}

void testObservationLayoutIsStable() {
    std::cout << "testObservationLayoutIsStable..." << std::endl;

    // The scalars land immediately after the grid, in declaration order. A
    // trained weight file depends on this; if someone reorders the struct, this
    // is the test that should fail rather than the policy silently misbehaving.
    AIObservation obs;
    obs.grid.fill(AICellState::Empty);
    obs.dxToGoal = 0.11f;
    obs.dyToGoal = 0.22f;
    obs.dxToOpponent = 0.33f;
    obs.dyToOpponent = 0.44f;
    obs.vx = 0.55f;
    obs.vy = 0.66f;
    obs.onGround = true;
    obs.canJump = false;
    obs.isPoweredUp = true;

    const auto features = obs.toFeatureVector();
    const std::size_t base = static_cast<std::size_t>(kAIVisionCells) * kAICellFeatures;

    assert(std::abs(features[base + 0] - 0.11f) < 1e-6f);
    assert(std::abs(features[base + 1] - 0.22f) < 1e-6f);
    assert(std::abs(features[base + 2] - 0.33f) < 1e-6f);
    assert(std::abs(features[base + 3] - 0.44f) < 1e-6f);
    assert(std::abs(features[base + 4] - 0.55f) < 1e-6f);
    assert(std::abs(features[base + 5] - 0.66f) < 1e-6f);
    assert(features[base + 6] == 1.0f && "onGround");
    assert(features[base + 7] == 0.0f && "canJump");
    assert(features[base + 8] == 1.0f && "isPoweredUp");

    std::cout << "  ok" << std::endl;
}

void testNeuralPolicyIsADropInReplacement() {
    std::cout << "testNeuralPolicyIsADropInReplacement..." << std::endl;

    // The seam's whole claim: anything holding an IAIPolicy can hold either one.
    std::unique_ptr<IAIPolicy> heuristic = std::make_unique<HeuristicPolicy>(AIArchetype::Speedrunner);
    std::unique_ptr<IAIPolicy> neural = std::make_unique<NeuralPolicy>(std::vector<int>{8});

    const AIObservation obs = makeObservation();
    const AIAction fromHeuristic = heuristic->decide(obs);
    const AIAction fromNeural = neural->decide(obs);

    // Both must answer. What they answer is their business — an untrained
    // network plays badly, which is correct and expected.
    (void)fromHeuristic;
    (void)fromNeural;

    // Never both directions at once: that is a contradictory action, and an
    // untrained sigmoid head will produce it unless it is resolved.
    assert(!(fromNeural.moveLeft && fromNeural.moveRight) &&
           "The policy must not press left and right together");

    auto* asNeural = dynamic_cast<NeuralPolicy*>(neural.get());
    assert(asNeural && asNeural->lastOutputs().size() == NeuralPolicy::kActionBits &&
           "One output per button");
    assert(!asNeural->isTrained() && "A random-init network must not claim to be trained");

    std::cout << "  ok" << std::endl;
}

void testNeuralPolicyRefusesMismatchedWeights() {
    std::cout << "testNeuralPolicyRefusesMismatchedWeights..." << std::endl;

    const std::string path = "saves/test_bad_weights.json";
    {
        // Right shape, wrong observation version. Loading this would misalign
        // every input; the failure has to be loud rather than a policy that acts
        // confidently and arbitrarily.
        nlohmann::json json;
        json["observationVersion"] = kAIObservationVersion + 99;
        json["layers"] = nlohmann::json::array();
        std::ofstream out(path);
        out << json.dump();
    }

    NeuralPolicy policy(std::vector<int>{4});
    assert(!policy.load(path) && "A version mismatch must be refused");
    assert(!policy.isTrained());

    {
        // Correct version, wrong input width.
        nlohmann::json layer;
        layer["weights"] = nlohmann::json::array({nlohmann::json::array({0.1f, 0.2f})});
        layer["biases"] = nlohmann::json::array({0.0f});
        nlohmann::json json;
        json["observationVersion"] = kAIObservationVersion;
        json["layers"] = nlohmann::json::array({layer});
        std::ofstream out(path);
        out << json.dump();
    }
    assert(!policy.load(path) && "A wrong input width must be refused");

    std::remove(path.c_str());
    std::cout << "  ok" << std::endl;
}

void testRewardRespondsToTheEventsItClaims() {
    std::cout << "testRewardRespondsToTheEventsItClaims..." << std::endl;

    RewardTracker reward;
    reward.reset({0.0f, 0.0f});

    EventBus& bus = EventBus::getInstance();
    bus.publish({EventType::CoinCollected, 1});
    const float afterCoin = reward.consume();
    assert(afterCoin > 0.0f && "A coin must be worth something");

    bus.publish({EventType::PlayerDamaged, 0});
    const float afterDamage = reward.consume();
    assert(afterDamage < 0.0f && "Damage must cost something");

    // consume() drains: a trainer must be told what happened since it last
    // acted, not a running total that credits every step with the whole episode.
    assert(std::abs(reward.consume()) < 1e-6f && "consume() must reset the pending value");

    // Rightward progress pays, but only for new ground — otherwise an agent
    // farms reward by pacing back and forth over the same stretch.
    reward.reset({100.0f, 0.0f});
    reward.observe({200.0f, 0.0f});
    const float forward = reward.consume();
    assert(forward > 0.0f && "New ground must pay");

    reward.observe({150.0f, 0.0f});
    reward.observe({200.0f, 0.0f});
    const float retread = reward.consume();
    assert(retread <= 0.0f && "Re-covering old ground must not pay again");

    // The episode total survives consume(), because it is what the overlay shows.
    assert(reward.episodeTotal() != 0.0f);

    std::cout << "  ok" << std::endl;
}

void testExperienceLogIsParseable() {
    std::cout << "testExperienceLogIsParseable..." << std::endl;

    const std::string path = "saves/test_experience.jsonl";
    std::remove(path.c_str());

    {
        ExperienceLog log;
        assert(log.open(path) && "Should be able to open the log");

        AIAction action;
        action.moveRight = true;
        action.jump = true;
        log.record(makeObservation(), action, 1.5f, false);
        log.record(makeObservation(), action, -50.0f, true);
        assert(log.rowsWritten() == 2);
    }

    // Read it back the way a trainer would: one JSON object per line.
    std::ifstream in(path);
    std::string line;
    assert(std::getline(in, line) && "Expected a header line");
    const auto header = nlohmann::json::parse(line);
    assert(header.at("type") == "header");
    assert(header.at("featureCount") == AIObservation::featureCount() &&
           "The header must state the width the rows actually have");
    assert(header.at("observationVersion") == kAIObservationVersion);

    int rows = 0;
    bool sawTerminal = false;
    while (std::getline(in, line)) {
        const auto row = nlohmann::json::parse(line);
        assert(row.at("obs").size() == AIObservation::featureCount());
        // Seven labels, matching the network's seven outputs — the property that
        // makes a recorded human session usable as imitation data.
        assert(row.at("act").size() == NeuralPolicy::kActionBits);
        assert(row.contains("rew"));
        if (row.at("done").get<bool>()) sawTerminal = true;
        ++rows;
    }
    assert(rows == 2 && "Both transitions must be readable");
    assert(sawTerminal && "The terminal flag must survive the round trip");

    std::remove(path.c_str());
    std::cout << "  ok" << std::endl;
}

void testControllerLearningPathWritesTransitions() {
    std::cout << "testControllerLearningPathWritesTransitions..." << std::endl;

    // This is the exact code the dev panel's "Start recording experience" button
    // runs, minus ImGui. Without this the button's three lines would be the only
    // untested part of the whole seam — and they are the part that connects it
    // to the game.
    TileMap tileMap;
    tileMap.initialize(60, 22);
    for (int x = 0; x < 60; ++x) {
        tileMap.setTile(x, 20, TileType::Ground);
        tileMap.setTile(x, 21, TileType::Ground);
    }

    const std::string path = "saves/test_controller_experience.jsonl";
    std::remove(path.c_str());

    Luigi bot({320.0f, 20.0f * 32.0f - 32.0f});
    bot.setGrounded(true);
    std::vector<std::unique_ptr<Entity>> entities;

    // Hard: zero reaction latency, so one update is one decision and one row.
    AIController controller(bot, AIDifficulty::Hard, AIArchetype::Speedrunner);
    assert(!controller.isLearning() && "Learning must be off until asked for");

    controller.enableLearning(path);
    assert(controller.isLearning());

    for (int frame = 0; frame < 5; ++frame) {
        controller.update(1.0f / 60.0f, nullptr, tileMap, entities);
    }

    assert(controller.transitionsLogged() >= 5 &&
           "Each decision must produce a transition");
    // Moving right over new ground, so progress reward should have accrued.
    assert(controller.episodeReward() != 0.0f && "Reward must be accumulating");

    std::cout << "  ok (" << controller.transitionsLogged() << " rows, reward "
              << controller.episodeReward() << ")" << std::endl;
    std::remove(path.c_str());
}

} // namespace

int main() {
    std::cout << "=== verify_rl_policy ===" << std::endl;

    testFeatureVectorIsFixedWidthAndBounded();
    testObservationLayoutIsStable();
    testNeuralPolicyIsADropInReplacement();
    testNeuralPolicyRefusesMismatchedWeights();
    testRewardRespondsToTheEventsItClaims();
    testExperienceLogIsParseable();
    testControllerLearningPathWritesTransitions();

    std::cout << "=== all RL seam checks passed ===" << std::endl;
    return 0;
}
