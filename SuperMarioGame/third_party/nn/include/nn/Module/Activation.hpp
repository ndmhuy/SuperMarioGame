#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"
#include <memory>
#include <functional>

namespace nn {

enum class ActivationType {
    ReLU,
    Sigmoid,
    Tanh,
    Softmax,
    LeakyReLU,
    GELU,
    SiLU
};

// Activation — non-linear activation function (inherits Module directly).
//
// forward() applies the activation and attaches a ModuleBackward node
// to the output tensor. backward() is public but NOT part of the Module
// interface — it's captured into a lambda by forward() and called only
// by the autograd engine via ModuleBackward.
//
// Factory Method pattern: use static methods ReLU(), Sigmoid(), Tanh(),
// Softmax() to create instances.

class Activation : public Module {
public:
    // Factory methods (Factory Method pattern)
    static std::unique_ptr<Activation> ReLU();
    static std::unique_ptr<Activation> Sigmoid();
    static std::unique_ptr<Activation> Tanh();
    static std::unique_ptr<Activation> Softmax();
    static std::unique_ptr<Activation> LeakyReLU(float alpha = 0.01f);
    static std::unique_ptr<Activation> GELU();
    static std::unique_ptr<Activation> SiLU();

    // Forward: applies activation, attaches ModuleBackward to output
    Tensor forward(const Tensor& input) override;

    // Backward: computes gradInput = gradOutput ⊙ activation'(input)
    // Called by ModuleBackward via captured lambda — NOT called directly
    Tensor backward(const Tensor& gradOutput);

private:
    ActivationType type_;
    Tensor lastInput_;
    Tensor lastOutput_;
    float alpha_;

    // Private constructor used by factories
    explicit Activation(ActivationType type, float alpha = 0.0f);
};

} // namespace nn
