#pragma once

#include "nn/Optim/Optimizer.hpp"

namespace nn {

class SGD : public Optimizer {
public:
    explicit SGD(float lr, float momentum = 0.0f);

    void step(const std::vector<Tensor*>& parameters) override;

private:
    float momentum_;
    std::vector<Tensor> velocities_;
};

} // namespace nn
