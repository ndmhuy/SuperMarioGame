#pragma once

#include "Entities/IAIPolicy.hpp"

#include <cstddef>
#include <cstdint>
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
    // Which objective is being optimised.
    //
    // Imitation caps the agent at its teacher: the best possible outcome is
    // copying it perfectly, and the teacher plateaus. Reinforce removes that
    // ceiling by optimising the game's own reward, but it is far harder to
    // train from scratch — which is why the default is to imitate first and
    // reinforce afterwards, the standard pretrain-then-finetune pipeline.
    enum class Mode { Imitation, Reinforce };

    struct Config {
        float learningRate = 0.01f;
        // Switch from imitation to reinforcement after this many episodes.
        // Imitation supplies a competent starting policy; REINFORCE from random
        // weights on a sparse-ish reward would spend most of its time failing to
        // reach any reward at all.
        int imitationEpisodes = 40;
        // Discount for the return. 0.99 at 60 decisions/second means the horizon
        // is ~1.7 s of game time, which is about the length of one jump-and-land
        // manoeuvre — the timescale over which an action's consequence shows up.
        float discount = 0.99f;
        // Learning rate for the reinforcement phase. Lower than imitation's:
        // policy-gradient updates are much higher variance, and this phase is
        // refining an already-competent policy rather than finding one.
        float reinforceLearningRate = 0.002f;
        // Hold each sampled action for this many decisions.
        //
        // Independently sampling seven buttons at 60 Hz produces incoherent
        // motion — left and right alternating frame to frame — which no return
        // can be attributed to. Holding an action is standard practice (the
        // frame-skip in the Atari DQN work) and gives exploration that lasts
        // long enough to have a consequence. 4 decisions is ~67 ms, roughly the
        // shortest input a human makes.
        int actionRepeat = 4;
        // Minimum spread of returns within an episode before its updates are
        // applied at all.
        //
        // Standardising by a near-zero standard deviation turns floating-point
        // noise into full-magnitude advantages: a stuck episode has returns
        // identical to four decimal places, and dividing their differences by
        // their own tiny spread produced advantages around 1.0 built entirely
        // from noise. The agent then trained hard on nothing, which kept it
        // stuck. Episodes flatter than this carry no signal and are skipped.
        float minReturnSpread = 0.5f;
        // DATASET AGGREGATION — the defining feature of DAgger, and initially
        // missing from this implementation.
        //
        // DAgger keeps every transition seen so far and retrains on the union;
        // that aggregation is what gives it its no-regret guarantee. Training
        // only on the newest sample is ordinary online cloning, and it degrades
        // for a specific reason: as beta decays the learner drives, visits its
        // own poor states, and trains exclusively on those, overwriting the
        // competent behaviour learned earlier. Measured without aggregation,
        // jump-agreement fell 0.849 -> 0.736 over 220 episodes while training
        // continued — catastrophic forgetting, not underfitting.
        //
        // A bounded buffer rather than the unbounded union of the paper: 176k
        // samples at 2844 floats would be 2 GB. 12k samples is ~136 MB and
        // still spans dozens of episodes, so early competent states stay in the
        // training distribution.
        // Sized for TEN levels, not one. 12k was enough to hold ~13 episodes
        // of level_1 and reach teacher parity there; spread over a 10-level
        // rotation it held ~1.3 episodes per level, and the forgetting
        // signature returned (jump agreement 0.99 -> 0.82 over 400 episodes).
        // Quantized storage (below) makes 48k cost what 12k float did.
        std::size_t aggregateCapacity = 48000;
        // Samples drawn from the buffer per update, alongside the new one.
        // Minibatches are also 2.45x more efficient per sample than batch 1.
        int replayBatch = 16;
        // Balance each button's contribution by its inverse press frequency.
        // Off, the jump button — pressed on ~3% of frames — contributes ~3% of
        // the gradient it should, and the optimiser correctly concludes that
        // never jumping is near-optimal.
        bool balanceClasses = true;
        // Floor on an estimated press rate, so a button never pressed in the
        // first few episodes cannot produce an unbounded weight.
        float minPressRate = 0.01f;
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

    // Reinforcement phase. Samples an action from the policy's own Bernoulli
    // outputs — stochastic on purpose, because a deterministic threshold never
    // explores and REINFORCE needs the actions it evaluates to be its own — and
    // records the transition. The caller executes the returned action.
    AIAction sampleAction(const AIObservation& observation);
    // Credit whatever the game paid since the previous decision.
    void recordReward(float reward);

    Mode mode() const { return m_mode; }

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
    // Per-button agreement this episode. One aggregate number hid the entire
    // failure for 219 episodes: 99.9% overall while the jump button was 0%.
    std::vector<float> buttonAgreement() const;
    // Observed teacher press rate per button — the class balance itself.
    const std::vector<float>& pressRates() const { return m_pressRate; }

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
    std::vector<float> m_pressRate;             // running teacher press frequency
    std::vector<std::size_t> m_buttonCorrect;   // per-button, this episode
    std::vector<std::size_t> m_buttonTotal;
    std::unique_ptr<class WeightedBernoulliLoss> m_loss;

    Mode m_mode = Mode::Imitation;

    // One episode's transitions, replayed at episode end once returns are
    // known. REINFORCE is episodic by construction: the weight on an action is
    // the discounted return that FOLLOWED it, which does not exist until the
    // episode does.
    struct Transition {
        std::vector<float> features;
        std::vector<float> action;   // the action actually sampled and executed
        float reward = 0.0f;
    };
    std::vector<Transition> m_episode;

    // The aggregated dataset — a RESERVOIR, not a FIFO ring. DAgger's
    // guarantee comes from training on the union of everything ever
    // collected; a ring buffer keeps only the most recent window, which on a
    // 10-level rotation is a recency monoculture. Reservoir sampling
    // (Algorithm R) keeps a uniform sample of ALL history in fixed memory:
    // once full, the n-th sample replaces a uniformly random slot with
    // probability capacity/n, or is dropped.
    //
    // Features are stored quantized to int8 (value * 127): the one-hots are
    // exact, velocities and scalars get 1/127 steps — sensor noise, not
    // signal loss — and each sample costs 4.4KB instead of 17.7KB.
    struct Sample {
        std::vector<std::int8_t> features;
        std::vector<float> target;
    };
    std::vector<Sample> m_aggregate;
    std::size_t m_samplesSeen = 0;
    // Running mean return, used as the REINFORCE baseline. Subtracting a
    // baseline leaves the gradient unbiased while cutting its variance, which
    // is the difference between this converging and thrashing.
    float m_returnBaseline = 0.0f;
    bool m_baselineSeeded = false;
    float m_lastEpisodeReturn = 0.0f;
    int m_skippedFlatEpisodes = 0;

    AIAction m_heldAction{};
    int m_repeatLeft = 0;

    void runReinforceUpdate();

public:
    float lastEpisodeReturn() const { return m_lastEpisodeReturn; }
    float returnBaseline() const { return m_returnBaseline; }
    // Episodes whose returns were too flat to learn from. A rising count means
    // the agent is stuck rather than that training has stalled.
    int skippedFlatEpisodes() const { return m_skippedFlatEpisodes; }

private:

    std::string m_logPath;
    bool m_logOpen = false;

    unsigned m_rngState = 0x5EEDu;
    float nextRandom();
};
