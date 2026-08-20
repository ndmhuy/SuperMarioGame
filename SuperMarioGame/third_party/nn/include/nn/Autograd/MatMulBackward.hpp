#pragma once
#include "nn/Autograd/GradFn.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

class MatMulBackward : public GradFn {
   public:
    MatMulBackward(const Tensor& lhs, const Tensor& rhs);
    std::string name() const override { return "MatMulBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {lhs_, rhs_}; }

   private:
    Tensor lhs_;
    Tensor rhs_;
};

}  // namespace nn
