#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

// BatchNorm — Batch Normalization layer for 2D inputs of shape [batchSize, features].
//
// Normalizes features across the batch dimension. Learnable parameters gamma
// and beta scale and shift the normalized inputs.
//
// Running statistics (runningMean and runningVar) are updated during training
// and used during evaluation.
class BatchNorm : public Module {
   public:
    // Constructor: numFeatures is the size of the feature dimension (columns).
    explicit BatchNorm(int numFeatures, float eps = 1e-5f, float momentum = 0.1f);
    ~BatchNorm() override = default;

    // Forward: normalizes input using batch statistics (train) or running statistics (eval)
    Tensor forward(const Tensor& input) override;

    // Backward: computes gradients for gamma, beta, and the input
    Tensor backward(const Tensor& gradOutput);

    // Returns gamma and beta parameters for optimization
    std::vector<Tensor*> parameters() override;

    // Returns named parameters
    std::map<std::string, Tensor*> namedParameters() override;

    // Getters for running statistics (primarily for testing and inspection)
    const Tensor& runningMean() const { return runningMean_; }
    const Tensor& runningVar() const { return runningVar_; }

   private:
    int numFeatures_;
    float eps_;
    float momentum_;

    // Learnable parameters
    Tensor gamma_;
    Tensor beta_;

    // Moving average statistics (non-trainable)
    Tensor runningMean_;
    Tensor runningVar_;

    // Cached states for backward pass (populated during forward)
    Tensor xHat_;
    Tensor stdInv_;
    Tensor mean_;
    Tensor var_;
};

}  // namespace nn