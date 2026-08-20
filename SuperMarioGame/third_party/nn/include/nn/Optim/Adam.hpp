#pragma once

#include "nn/Optim/Optimizer.hpp"

namespace nn {

class Adam : public Optimizer {
public:
    explicit Adam(float lr, float beta1 = 0.9f, float beta2 = 0.999f, float eps = 1e-8f);

    void step(const std::vector<Tensor*>& parameters) override;

private:
    float beta1_;
    float beta2_;
    float eps_;
    int t_{0}; // time step count
    std::vector<Tensor> m_; // first moment vectors
    std::vector<Tensor> v_; // second moment vectors
};

} // namespace nn
