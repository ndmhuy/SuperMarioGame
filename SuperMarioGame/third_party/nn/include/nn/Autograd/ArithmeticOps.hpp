#pragma once
#include "nn/Autograd/GradFn.hpp"
#include "nn/Tensor/Tensor.hpp"  // Note: careful with circular dependencies if we need Tensor definitions here.
                                 // Usually passing by reference is okay with forward declaration,
                                 // but we might need to store Tensor copies if they are needed for backward pass.

namespace nn {

class AddBackward : public GradFn {
   public:
    AddBackward(const Tensor& lhs, const Tensor& rhs);
    std::string name() const override { return "AddBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {lhs_, rhs_}; }

   private:
    Tensor lhs_;
    Tensor rhs_;
};

class SubBackward : public GradFn {
   public:
    SubBackward(const Tensor& lhs, const Tensor& rhs);
    std::string name() const override { return "SubBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {lhs_, rhs_}; }

   private:
    Tensor lhs_;
    Tensor rhs_;
};

class MulBackward : public GradFn {
   public:
    MulBackward(const Tensor& lhs, const Tensor& rhs);
    std::string name() const override { return "MulBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {lhs_, rhs_}; }

   private:
    Tensor lhs_;
    Tensor rhs_;
};

class DivBackward : public GradFn {
   public:
    DivBackward(const Tensor& lhs, const Tensor& rhs);
    std::string name() const override { return "DivBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {lhs_, rhs_}; }

   private:
    Tensor lhs_;
    Tensor rhs_;
};

}  // namespace nn
