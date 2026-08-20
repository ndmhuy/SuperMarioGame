#include "nn/Loss/MSELoss.hpp"
#include "nn/Core/Device.hpp"

namespace nn {

Tensor MSELoss::compute(const Tensor& input, const Tensor& target) {
    int N = input.size();
    Tensor lossVal({1});

    float val = 0.0f;
    Device::activeBackend()->mseLoss(
        std::span<const float>(input.rawData(), N),
        std::span<const float>(target.rawData(), N),
        val, N
    );
    lossVal.flat(0) = val;

    return lossVal;
}

Tensor MSELoss::gradient(const Tensor& input, const Tensor& target) {
    int N = input.size();
    Tensor gradInput(input.shape());

    Device::activeBackend()->mseLossGrad(
        std::span<const float>(input.rawData(), N),
        std::span<const float>(target.rawData(), N),
        std::span<float>(gradInput.rawData(), N),
        N
    );

    return gradInput;
}

} // namespace nn
