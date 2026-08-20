#pragma once

#include <cstddef>
#include <memory>
#include <vector>

// Loss functions for the policy network.
//
// Both the imitation objective and the policy-gradient objective are the same
// Bernoulli log-likelihood with different per-element weights, so they are one
// class. Kept out of third_party/nn because they are specific to this policy's
// seven-button action space, not general-purpose losses.
//
// PIMPL'd: the implementation derives from nn::Loss, and nn/ headers require
// C++20 while the game is C++17.
class WeightedBernoulliLoss {
public:
    WeightedBernoulliLoss();
    ~WeightedBernoulliLoss();

    WeightedBernoulliLoss(const WeightedBernoulliLoss&) = delete;
    WeightedBernoulliLoss& operator=(const WeightedBernoulliLoss&) = delete;

    // Per-button multipliers on the log-likelihood, applied before summing.
    //
    // For IMITATION these are class-balance weights, so that a button pressed
    // on 3% of frames contributes as much total gradient as one pressed on 97%.
    // Without this the optimiser correctly concludes that never jumping is
    // excellent, because it is — by the unweighted metric.
    //
    // For POLICY GRADIENT they are all the same value, the advantage, and the
    // SIGN carries the learning signal: a positive advantage increases the
    // probability of the action that was taken, a negative one decreases it.
    // This is what lets the agent exceed the teacher rather than converge to it.
    void setWeights(const std::vector<float>& weights);

    // Opaque handle to the nn::Loss for PolicyTrainer to pass to forward().
    // Declared void* rather than a forward-declared type because the concrete
    // type is nn::Loss, which cannot be named in a C++17 header.
    void* handle() const;

private:
    struct Impl;
    struct ImplDeleter { void operator()(Impl*) const; };
    std::unique_ptr<Impl, ImplDeleter> m_impl;
};
