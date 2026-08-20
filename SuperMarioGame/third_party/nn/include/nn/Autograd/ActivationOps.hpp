#pragma once
#include "nn/Autograd/GradFn.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

class ReluBackward : public GradFn {
public:
    ReluBackward(const Tensor& input, const Tensor& output);
    std::string name() const override { return "ReluBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {input_}; }
   private:
    Tensor input_;
};

class SigmoidBackward : public GradFn {
public:
    SigmoidBackward(const Tensor& input, const Tensor& output);
    std::string name() const override { return "SigmoidBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {input_}; }
   private:
    Tensor input_;
};

class TanhBackward : public GradFn {
public:
    TanhBackward(const Tensor& input, const Tensor& output);
    std::string name() const override { return "TanhBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {input_}; }
   private:
    Tensor input_;
};

class SoftmaxBackward : public GradFn {
public:
    SoftmaxBackward(const Tensor& input, const Tensor& output);
    std::string name() const override { return "SoftmaxBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override { return {input_}; }
   private:
    Tensor input_;
    Tensor output_;
};

} // namespace nn
