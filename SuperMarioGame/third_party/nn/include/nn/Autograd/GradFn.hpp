#pragma once

#include <vector>
#include <string>

namespace nn {

class Tensor;

// Abstract base class for all nodes in the computational graph
class GradFn {
public:
    virtual ~GradFn() = default;

    // Returns a human-readable name of the operation (useful for debugging/printing graph)
    virtual std::string name() const = 0;

    // Given the gradient of the loss with respect to the output of this function,
    // computes the gradient of the loss with respect to each of the inputs.
    // Returns a vector of gradients, one for each input.
    virtual std::vector<Tensor> backward(const Tensor& gradOutput) = 0;

    // Returns the inputs to this operation, required for building the computational graph
    // during topological sort.
    virtual std::vector<Tensor> inputs() const = 0;
};

} // namespace nn
