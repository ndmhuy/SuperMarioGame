#pragma once

#include "nn/Loss/Loss.hpp"

namespace nn {

// CrossEntropyLoss — standard classification cross-entropy loss.
//
// In our architecture, the model's last layer is Activation::Softmax(),
// which outputs probability distributions (predictions).
// Therefore, CrossEntropyLoss expects:
//   - input: Softmax output probabilities [batchSize x numClasses]
//   - target: One-hot target labels [batchSize x numClasses]
class CrossEntropyLoss : public Loss {
public:
    Tensor compute(const Tensor& input, const Tensor& target) override;
    Tensor gradient(const Tensor& input, const Tensor& target) override;
};

} // namespace nn
