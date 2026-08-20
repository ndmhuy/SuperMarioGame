#include "nn/Module/Linear.hpp"
#include "nn/Tensor/Matrix.hpp"
#include "nn/Core/Device.hpp"
#include "nn/Autograd/ModuleBackward.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include <cmath>
#include <random>
#include <span>

namespace nn {

// ── Constructor with Xavier Initialization ──
// In C#, the Perceptron project used System.Random with a hardcoded seed.
// Here we use the modern C++ <random> library: std::mt19937 (Mersenne Twister)
// with std::normal_distribution for proper Gaussian sampling.
//
// Xavier init: stddev = sqrt(2.0 / (fan_in + fan_out))
// This keeps the signal variance stable across layers during both forward
// and backward passes, preventing vanishing/exploding gradients.

Linear::Linear(int inputSize, int outputSize)
    : weights_({inputSize, outputSize}),
      bias_({1, outputSize}),
      lastInput_({1}) {
    weights_.setRequiresGrad(true);
    bias_.setRequiresGrad(true);
    // Xavier initialization
    float stddev = std::sqrt(2.0f / static_cast<float>(inputSize + outputSize));
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::normal_distribution<float> dist(0.0f, stddev);

    float* w = weights_.rawData();
    for (int i = 0; i < weights_.size(); ++i) {
        w[i] = dist(gen);
    }
    // Bias initialized to zero (standard practice)
}

// ── Forward pass ──
// Y = X * W + b
// X is [batch x inputSize], W is [inputSize x outputSize], b is [1 x outputSize]
// Output is [batch x outputSize]
//
// In the C# project, this was done with the % operator for matmul.
// Here we use the free function matmul() which delegates to CPUBackend.
//
// AUTOGRAD: After computing the output, we attach a ModuleBackward node
// that captures this->backward() as a lambda. When Tensor::backward()
// traverses the graph, it invokes this callback automatically.

Tensor Linear::forward(const Tensor& input) {
    if (GradMode::is_enabled()) {
        // Save input for backward pass
        lastInput_ = input.clone();
    }

    // Matrix multiply: [batch x in] * [in x out] = [batch x out]
    Tensor output = matmul(input, weights_);

    // Add bias (broadcast across batch dimension)
    // bias_ is [1 x out], output is [batch x out]
    int batch = output.shape()[0];
    int outSize = output.shape()[1];
    float* outPtr = output.rawData();
    const float* biasPtr = bias_.rawData();
    for (int i = 0; i < batch; ++i) {
        Device::activeBackend()->add(
            std::span<const float>(outPtr + i * outSize, outSize),
            std::span<const float>(biasPtr, outSize),
            std::span<float>(outPtr + i * outSize, outSize));
    }

    // Attach autograd node: capture this->backward() into the graph
    if (GradMode::is_enabled()) {
        output.setRequiresGrad(true);
        output.setGradFn(std::make_shared<ModuleBackward>(
            [this](const Tensor& g) { return this->backward(g); },
            input
        ));
    }

    return output;
}

// ── Backward pass ──
// Given dL/dY (gradOutput), compute:
//   dL/dW = X^T * dL/dY
//   dL/db = sum over batch of dL/dY
//   dL/dX = dL/dY * W^T  (returned for previous layer)
//
// Optimization: uses Matrix::transpose() (zero-copy view with swapped strides)
// instead of the old manual transpose loops. The contiguous copy needed for
// matmul() is handled automatically by clone() inside matmul's contiguity check,
// or we can make the input contiguous explicitly before passing to matmul.

Tensor Linear::backward(const Tensor& gradOutput) {
    // TODO: Implement backward pass manually.
    // The logic should:
    //   1. Compute dL/dW = lastInput_^T * gradOutput
    //   2. Accumulate dL/dW into weights_.grad()
    //   3. Compute dL/db = sum of gradOutput along batch dim
    //   4. Accumulate dL/db into bias_.grad()
    //   5. Compute and return dL/dX = gradOutput * weights_^T
    //
    // Reference implementation (uncomment and adapt):
    //
    int batch = lastInput_.shape()[0];
    int inSize = lastInput_.shape()[1];
    int outSize = gradOutput.shape()[1];

    // dL/dW = X^T * dL/dY
    Tensor dW({inSize, outSize});
    Device::activeBackend()->matmulTransA(
        std::span<const float>(lastInput_.rawData(), lastInput_.size()),
        std::span<const float>(gradOutput.rawData(), gradOutput.size()),
        std::span<float>(dW.rawData(), dW.size()),
        inSize, batch, outSize
    );

    if (!weights_.grad()) {
        weights_.setGrad(std::make_shared<Tensor>(dW));
    } else {
        *weights_.grad() += dW;
    }

    // dL/db = sum of gradOutput along batch dimension
    if (!bias_.grad()) {
        bias_.setGrad(std::make_shared<Tensor>(bias_.shape()));
        bias_.grad()->fill(0.0f);
    }
    float* bgPtr = bias_.grad()->rawData();
    const float* goPtr = gradOutput.rawData();
    for (int i = 0; i < batch; ++i) {
        Device::activeBackend()->add(
            std::span<const float>(bgPtr, outSize),
            std::span<const float>(goPtr + i * outSize, outSize),
            std::span<float>(bgPtr, outSize));
    }

    // dL/dX = dL/dY * W^T  [batch x outSize] * [outSize x inSize] = [batch x inSize]
    Tensor gradInput({batch, inSize});
    Device::activeBackend()->matmulTransB(
        std::span<const float>(gradOutput.rawData(), gradOutput.size()),
        std::span<const float>(weights_.rawData(), weights_.size()),
        std::span<float>(gradInput.rawData(), gradInput.size()),
        batch, outSize, inSize
    );

    return gradInput;
}

// ── Parameters ──
// Returns parameters for the optimizer to update.
 
std::vector<Tensor*> Linear::parameters() {
    return {&weights_, &bias_};
}

// ── Named Parameters ──
std::map<std::string, Tensor*> Linear::namedParameters() {
    return {{"weight", &weights_}, {"bias", &bias_}};
}

} // namespace nn
