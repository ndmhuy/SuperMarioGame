#pragma once

#include "nn/Loss/Loss.hpp"

namespace nn {

// MSELoss — Mean Squared Error loss:
// L = (1 / 2N) * sum((input - target)^2)
class MSELoss : public Loss {
public:
    Tensor compute(const Tensor& input, const Tensor& target) override;
    Tensor gradient(const Tensor& input, const Tensor& target) override;
};

} // namespace nn
