#pragma once

#include "nn/Tensor/Tensor.hpp"
#include <vector>
#include <utility>

namespace nn {

// Abstract base class for all optimizers.
// In the C# Perceptron project, SGD was the only optimizer and it was a standalone class.
// Here we define an abstract Optimizer interface so we can add Adam in Phase 3.
class Optimizer {
public:
    explicit Optimizer(float lr) : lr_(lr) {}
    virtual ~Optimizer() = default;

    // Update parameters using their gradients.
    virtual void step(const std::vector<Tensor*>& parameters) = 0;
 
    // Zero out all gradient tensors. Called before each training step
    // to prevent gradient accumulation across iterations.
    virtual void zeroGrad(const std::vector<Tensor*>& parameters) {
        for (Tensor* param : parameters) {
            if (param->grad()) {
                param->grad()->fill(0.0f);
            }
        }
    }

protected:
    float lr_;
};

} // namespace nn
