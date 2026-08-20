#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

// Dropout — regularization layer that randomly zeros out elements during training.
//
// During training, elements are zeroed with probability p, and scaled by 1/(1-p).
// During evaluation, the layer is an identity function.
//
// Like Linear, it attaches a ModuleBackward node to the output tensor
// to enable autograd integration.
class Dropout : public Module {
public:
    // Constructor: p is the dropout probability (0.0f to 1.0f).
    explicit Dropout(float p = 0.5f);
    ~Dropout() override = default;

    // Forward: randomly drops activations during training
    Tensor forward(const Tensor& input) override;

    // Backward: computes dL/dX = dL/dY * mask
    Tensor backward(const Tensor& gradOutput);

    // Returns parameters (empty for Dropout)
    std::vector<Tensor*> parameters() override;

    // Returns named parameters (empty for Dropout)
    std::map<std::string, Tensor*> namedParameters() override;

private:
    float p_;            // Dropout probability
    Tensor mask_;        // Saved mask for backward pass (scaled by 1/(1-p))
};

} // namespace nn
