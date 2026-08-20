#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

// Linear — fully-connected layer (inherits Module directly).
//
// forward() computes Y = X * W + b and attaches a ModuleBackward node
// to the output tensor, so Tensor::backward() can traverse the graph
// without Sequential needing an explicit backward() loop.
//
// backward() is a public method but is NOT part of the Module interface.
// It is captured into a std::function by forward() and called only by
// the autograd engine via ModuleBackward.

class Linear : public Module {
public:
    // Constructor: creates weights [inputSize x outputSize] and bias [1 x outputSize]
    // Uses Xavier initialization for weights, zeros for bias
    Linear(int inputSize, int outputSize);

    // Forward: Y = X * W + b  where X is [batch x inputSize]
    // Attaches ModuleBackward to output for autograd graph construction
    Tensor forward(const Tensor& input) override;

    // Backward: computes dL/dW, dL/db, returns dL/dX
    // Called by ModuleBackward via captured lambda — NOT called directly
    Tensor backward(const Tensor& gradOutput);

    // Returns parameters for optimizer
    std::vector<Tensor*> parameters() override;

    // Returns named parameters for this layer
    std::map<std::string, Tensor*> namedParameters() override;

private:
    Tensor weights_;       // [inputSize x outputSize]
    Tensor bias_;          // [1 x outputSize]

    Tensor lastInput_;     // saved for backward pass
};

} // namespace nn
