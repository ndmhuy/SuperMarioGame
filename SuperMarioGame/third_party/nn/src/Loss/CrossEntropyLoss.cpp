#include "nn/Loss/CrossEntropyLoss.hpp"
#include "nn/Core/Device.hpp"

namespace nn {

Tensor CrossEntropyLoss::compute(const Tensor& input, const Tensor& target) {
    int batchSize = input.shape()[0];
    Tensor lossVal({1});

    float val = 0.0f;
    Device::activeBackend()->crossEntropyLoss(
        std::span<const float>(input.rawData(), input.size()),
        std::span<const float>(target.rawData(), target.size()),
        val, batchSize
    );
    lossVal.flat(0) = val;

    return lossVal;
}

Tensor CrossEntropyLoss::gradient(const Tensor& input, const Tensor& target) {
    int batchSize = input.shape()[0];
    Tensor gradInput(input.shape());

    Device::activeBackend()->crossEntropyLossGrad(
        std::span<const float>(input.rawData(), input.size()),
        std::span<const float>(target.rawData(), target.size()),
        std::span<float>(gradInput.rawData(), gradInput.size()),
        batchSize
    );

    return gradInput;
}

} // namespace nn
