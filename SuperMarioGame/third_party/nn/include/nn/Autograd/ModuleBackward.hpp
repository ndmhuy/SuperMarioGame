#pragma once

#include "nn/Autograd/GradFn.hpp"
#include "nn/Tensor/Tensor.hpp"
#include <functional>

namespace nn {

// ModuleBackward — bridges Module::backward() into the autograd graph.
//
// Instead of storing a Layer*, it stores a std::function callback.
// Each leaf module (Linear, Activation) captures its own backward() into
// this callback during forward(). This decouples the autograd graph from
// the Module hierarchy entirely — no Layer* needed.
//
// When Tensor::backward() traverses the computational graph, it calls
// ModuleBackward::backward() which invokes the captured callback.

class ModuleBackward : public GradFn {
   public:
    using BackwardFn = std::function<Tensor(const Tensor&)>;

    ModuleBackward(BackwardFn fn, Tensor input);

    std::string name() const override { return "ModuleBackward"; }
    std::vector<Tensor> backward(const Tensor& gradOutput) override;
    std::vector<Tensor> inputs() const override;

   private:
    BackwardFn backwardFn_;
    Tensor input_;
};

}  // namespace nn