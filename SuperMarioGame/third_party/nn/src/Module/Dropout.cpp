#include "nn/Module/Dropout.hpp"
#include "nn/Autograd/ModuleBackward.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include <random>
#include <stdexcept>

namespace nn {

Dropout::Dropout(float p) : p_(p), mask_({1}) {
    if (p < 0.0f || p >= 1.0f) {
        throw std::invalid_argument("Dropout probability must be in range [0, 1)");
    }
}

Tensor Dropout::forward(const Tensor& input) {
    if (!training_) {
        // During evaluation, Dropout is an identity function.
        return input;
    }

    // Generate random mask of same shape as input
    mask_ = Tensor(input.shape());
    float* maskPtr = mask_.rawData();
    int totalSize = input.size();
    float scale = 1.0f / (1.0f - p_);

    // Use thread-local generator for speed and thread-safety
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < totalSize; ++i) {
        maskPtr[i] = (dist(gen) >= p_) ? scale : 0.0f;
    }

    Tensor output;
    {
        // Compute element-wise multiplication in NoGrad mode to avoid
        // building extraneous autograd nodes (e.g. MulBackward).
        NoGrad guard;
        output = input * mask_;
    }

    // Register ModuleBackward node to hook into autograd
    if (GradMode::is_enabled()) {
        output.setRequiresGrad(true);
        output.setGradFn(std::make_shared<ModuleBackward>(
            [this](const Tensor& g) { return this->backward(g); },
            input
        ));
    }

    return output;
}

Tensor Dropout::backward(const Tensor& gradOutput) {
    if (!training_) {
        return gradOutput;
    }

    Tensor gradInput;
    {
        // Compute element-wise multiplication in NoGrad mode to avoid
        // building extraneous autograd nodes.
        NoGrad guard;
        gradInput = gradOutput * mask_;
    }

    return gradInput;
}

std::vector<Tensor*> Dropout::parameters() {
    return {}; // Dropout has no learnable parameters
}

std::map<std::string, Tensor*> Dropout::namedParameters() {
    return {}; // Dropout has no learnable parameters
}

} // namespace nn
