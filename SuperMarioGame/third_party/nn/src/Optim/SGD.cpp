#include "nn/Optim/SGD.hpp"
#include "nn/Core/Device.hpp"

namespace nn {

SGD::SGD(float lr, float momentum) : Optimizer(lr), momentum_(momentum) {}

void SGD::step(const std::vector<Tensor*>& parameters) {
    if (parameters.empty()) return;

    // Lazy-initialize velocity buffers on first call
    if (momentum_ > 0.0f && velocities_.empty()) {
        for (auto* param : parameters) {
            Tensor vel(param->shape());
            vel.fill(0.0f);
            velocities_.push_back(std::move(vel));
        }
    }
 
    for (int i = 0; i < static_cast<int>(parameters.size()); ++i) {
        Tensor* param = parameters[i];
        if (!param->grad()) continue;
        Tensor* grad = param->grad().get();
 
        std::span<float> pSpan(param->rawData(), param->size());
        std::span<const float> gSpan(grad->rawData(), grad->size());
        std::span<float> vSpan;
        if (momentum_ > 0.0f) {
            vSpan = std::span<float>(velocities_[i].rawData(), velocities_[i].size());
        }

        Device::activeBackend()->sgdStep(pSpan, gSpan, vSpan, lr_, momentum_);
    }
}

}  // namespace nn
