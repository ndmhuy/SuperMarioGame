#include "nn/Optim/Adam.hpp"
#include "nn/Core/Device.hpp"
#include <cmath>

namespace nn {

Adam::Adam(float lr, float beta1, float beta2, float eps)
    : Optimizer(lr), beta1_(beta1), beta2_(beta2), eps_(eps) {}

void Adam::step(const std::vector<Tensor*>& parameters) {
    if (parameters.empty()) return;
    t_++;

    // Lazy-initialize moment buffers on first call
    if (m_.empty()) {
        for (auto* param : parameters) {
            Tensor m(param->shape());
            m.fill(0.0f);
            m_.push_back(std::move(m));

            Tensor v(param->shape());
            v.fill(0.0f);
            v_.push_back(std::move(v));
        }
    }

    // Compute bias corrections
    float biasCorrection1 = 1.0f - std::pow(beta1_, t_);
    float biasCorrection2 = 1.0f - std::pow(beta2_, t_);

    for (size_t i = 0; i < parameters.size(); ++i) {
        Tensor* param = parameters[i];
        if (!param->grad()) continue;
        Tensor* grad = param->grad().get();

        std::span<float> pSpan(param->rawData(), param->size());
        std::span<const float> gSpan(grad->rawData(), grad->size());
        std::span<float> mSpan(m_[i].rawData(), m_[i].size());
        std::span<float> vSpan(v_[i].rawData(), v_[i].size());

        Device::activeBackend()->adamStep(pSpan, gSpan, mSpan, vSpan,
                                         lr_, beta1_, beta2_, eps_,
                                         biasCorrection1, biasCorrection2, param->size());
    }
}

} // namespace nn
