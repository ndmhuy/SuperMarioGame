#pragma once

#include "Entities/IAIPolicy.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class NeuralPolicy;

// Trains a NeuralPolicy inside the running game, so learning can be watched
// rather than inferred from a log file afterwards
// (docs/mapgen_gan_rl_plan.md §2d).
//
// Algorithm: DAgger (Ross et al. 2011) — dataset aggregation, i.e. supervised
// imitation of the heuristic policy on the states the *learner* actually
// visits.
//
// Why this rather than REINFORCE or DQN as the first learner:
//
//  - The heuristic already plays reasonably (level_2 to 75%), so it is a usable
//    teacher, and matching it is a far easier optimisation problem than
//    discovering behaviour from a sparse reward. It converges in minutes rather
//    than hours, which matters when the point is to *see* it converge.
//  - It gives an honest, immediately meaningful progress metric — agreement
//    with the teacher — where a policy-gradient run's early reward curve is
//    mostly noise.
//  - The action space is seven independent buttons decoded by threshold, which
//    is a natural multi-label regression target. A policy gradient would first
//    need a distribution over that 2^7 space.
//
// Why DAgger specifically and not plain behavioural cloning: cloning trains on
// states the *teacher* visits, so the moment the learner drifts somewhere the
// teacher never went, it is out of distribution and has no idea what to do —
// the classic compounding-error failure. DAgger fixes it by letting the learner
// drive while the teacher labels whatever state the learner reaches. `beta`
// anneals control from teacher to learner over training.
//
// This is deliberately a stepping stone: the ceiling of imitation is the
// teacher, and the teacher plateaued (§2c). Exceeding it needs reinforcement
// on top, which is the next step and reuses every part of this — the
// observation, the network and the optimiser are identical.
class PolicyTrainer {
public:
    struct Config {
        float learningRate = 0.01f;
        // Fraction of decisions the TEACHER executes. Starts at 1 (pure
        // demonstration, so the learner sees competent play) and decays toward
        // 0 (learner drives, teacher only labels).
        float beta = 1.0f;
        float betaDecayPerEpisode = 0.05f;
        float minBeta = 0.0f;
    };

    // `policy` must outlive the trainer; it is the network being trained.
    //
    // Two overloads rather than a defaulted `const Config& = Config()`: a
    // default argument in the class body requires Config's member initialisers
    // to be complete at that point, and they are not.
    explicit PolicyTrainer(NeuralPolicy& policy);
    PolicyTrainer(NeuralPolicy& policy, const Config& config);
    ~PolicyTrainer();

    PolicyTrainer(const PolicyTrainer&) = delete;
    PolicyTrainer& operator=(const PolicyTrainer&) = delete;

    // One supervised step: the network predicts on `observation`, the loss is
    // measured against the teacher's action, and one optimiser step is taken.
    // Returns the loss for this sample.
    float learn(const AIObservation& observation, const AIAction& teacherAction);

    // Whether the teacher should drive this decision, per the current beta.
    bool teacherDrives();

    // Called at the end of an episode: anneals beta and rolls the metrics.
    // `outcome` is recorded in the CSV log so a flat stretch of the curve can
    // be traced back to what was actually happening in the episodes.
    void endEpisode(const char* outcome = "");

    // Append every episode to a CSV. The on-screen curves are transient and
    // bounded; a run left going for an hour deserves a record that survives the
    // window closing and can be plotted properly afterwards.
    void openLog(const std::string& path);

    // --- Metrics, for the live overlay --------------------------------------
    float lastLoss() const { return m_lastLoss; }
    // Mean loss over a trailing window, which is what actually reads as a
    // curve; per-sample loss is far too noisy to see a trend in.
    const std::vector<float>& lossHistory() const { return m_lossHistory; }
    const std::vector<float>& agreementHistory() const { return m_agreementHistory; }
    // Fraction of the seven buttons the network currently gets right, this
    // episode. The number that says whether it is actually learning.
    float episodeAgreement() const;
    int episodes() const { return m_episodes; }
    std::size_t samples() const { return m_samples; }
    float beta() const { return m_config.beta; }
    // The network's last raw outputs, for the per-button bar display.
    const std::vector<float>& lastPrediction() const { return m_lastPrediction; }

private:
    struct Impl;                       // holds nn:: types; see NeuralPolicy.hpp
    struct ImplDeleter { void operator()(Impl*) const; };
    std::unique_ptr<Impl, ImplDeleter> m_impl;

    NeuralPolicy* m_policy = nullptr;
    Config m_config;
    float m_lastLoss = 0.0f;
    int m_episodes = 0;
    std::size_t m_samples = 0;

    // Per-episode accumulators.
    double m_episodeLossSum = 0.0;
    std::size_t m_episodeSamples = 0;
    std::size_t m_episodeButtonsCorrect = 0;
    std::size_t m_episodeButtonsTotal = 0;

    std::vector<float> m_lossHistory;
    std::vector<float> m_agreementHistory;
    std::vector<float> m_lastPrediction;

    std::string m_logPath;
    bool m_logOpen = false;

    unsigned m_rngState = 0x5EEDu;
    float nextRandom();
};
