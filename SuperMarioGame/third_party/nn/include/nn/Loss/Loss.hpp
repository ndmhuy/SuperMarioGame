#pragma once

#include "nn/Tensor/Tensor.hpp"

namespace nn {

// Loss — abstract base class for loss functions (Loss layer pattern).
//
// Separates the core math calculations (compute() and gradient()) from the
// autograd graph routing (forward()).
class Loss {
public:
    virtual ~Loss() = default;

    // Pure mathematical operations (overridden by concrete losses)
    virtual Tensor compute(const Tensor& input, const Tensor& target) = 0;
    virtual Tensor gradient(const Tensor& input, const Tensor& target) = 0;

    // Forward pass: computes the loss value and automatically binds the
    // analytical gradient to the autograd graph if tracking is active.
    Tensor forward(const Tensor& input, const Tensor& target);

    // Callable interface for convenience: loss = criterion(input, target)
    Tensor operator()(const Tensor& input, const Tensor& target) {
        return forward(input, target);
    }
};

} // namespace nn
