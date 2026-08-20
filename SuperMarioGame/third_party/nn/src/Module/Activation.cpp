#include "nn/Module/Activation.hpp"
#include "nn/Autograd/ModuleBackward.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include <cmath>

#include "nn/Core/Device.hpp"
#include <span>

namespace nn {

// ---------------------------------------------------------------------------
// VENDORED, WITH A SECOND PATCH. See third_party/nn/README.md.
//
// Upstream CS200-Cpp at the commit this was copied from does not compile this
// file: Activation.hpp declares LeakyReLU/GELU/SiLU factories, three matching
// ActivationType enumerators and an `alpha` constructor parameter, but
// Activation.cpp implements none of them — the header ran ahead of the source
// and upstream's build/ directory still holds pre-change objects, so the break
// is invisible there until a clean rebuild.
//
// The wiring below is filled in against the Backend interface, which already
// declares leakyRelu/gelu/silu and their backward passes — so this is
// completing an interface upstream had already provided for, not inventing
// behaviour. The same fix is worth applying upstream.
// ---------------------------------------------------------------------------

Activation::Activation(ActivationType type, float alpha)
    : type_(type), lastInput_({1}), lastOutput_({1}), alpha_(alpha) {}

// ── Factory Methods ──

std::unique_ptr<Activation> Activation::ReLU() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::ReLU));
}

std::unique_ptr<Activation> Activation::Sigmoid() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::Sigmoid));
}

std::unique_ptr<Activation> Activation::Tanh() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::Tanh));
}

std::unique_ptr<Activation> Activation::Softmax() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::Softmax));
}

std::unique_ptr<Activation> Activation::LeakyReLU(float alpha) {
    return std::unique_ptr<Activation>(new Activation(ActivationType::LeakyReLU, alpha));
}

std::unique_ptr<Activation> Activation::GELU() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::GELU));
}

std::unique_ptr<Activation> Activation::SiLU() {
    return std::unique_ptr<Activation>(new Activation(ActivationType::SiLU));
}

// ── Forward pass ──
// Applies the activation function and attaches a ModuleBackward node
// to the output tensor for autograd graph construction.

Tensor Activation::forward(const Tensor& input) {
    if (GradMode::is_enabled()) {
        lastInput_ = input.clone();
    }
    Tensor output(input.shape());
    
    std::span<const float> in_span(input.rawData(), input.size());
    std::span<float> out_span(output.rawData(), output.size());

    switch (type_) {
        case ActivationType::ReLU:
            Device::activeBackend()->relu(in_span, out_span);
            break;
        case ActivationType::Sigmoid:
            Device::activeBackend()->sigmoid(in_span, out_span);
            break;
        case ActivationType::Tanh:
            Device::activeBackend()->tanh(in_span, out_span);
            break;
        case ActivationType::Softmax:
            Device::activeBackend()->softmax(in_span, out_span, input.shape().back());
            if (GradMode::is_enabled()) {
                lastOutput_ = output.clone();
            }
            break;
        case ActivationType::LeakyReLU:
            Device::activeBackend()->leakyRelu(in_span, out_span, alpha_);
            break;
        case ActivationType::GELU:
            Device::activeBackend()->gelu(in_span, out_span);
            break;
        case ActivationType::SiLU:
            Device::activeBackend()->silu(in_span, out_span);
            break;
    }

    // Attach autograd node: capture this->backward() into the graph
    if (GradMode::is_enabled()) {
        output.setRequiresGrad(true);
        output.setGradFn(std::make_shared<ModuleBackward>(
            [this](const Tensor& g) { return this->backward(g); },
            input
        ));
    }

    return output;
}

// ── Backward pass ──
// Computes gradInput = gradOutput ⊙ activation'(lastInput_)
// Called by the autograd engine via ModuleBackward, NOT directly.

Tensor Activation::backward(const Tensor& gradOutput) {
    // TODO: Implement backward pass manually.
    // The logic should compute the element-wise product of gradOutput
    // with the derivative of the activation function evaluated at lastInput_.
    //
    // Reference implementation (uncomment and adapt):
    //
    Tensor gradInput(gradOutput.shape());
    
    std::span<const float> in_span(lastInput_.rawData(), lastInput_.size());
    std::span<const float> go_span(gradOutput.rawData(), gradOutput.size());
    std::span<float> gi_span(gradInput.rawData(), gradInput.size());

    switch (type_) {
        case ActivationType::ReLU:
            Device::activeBackend()->reluBackward(in_span, go_span, gi_span);
            break;
        case ActivationType::Sigmoid:
            Device::activeBackend()->sigmoidBackward(in_span, go_span, gi_span);
            break;
        case ActivationType::Tanh:
            Device::activeBackend()->tanhBackward(in_span, go_span, gi_span);
            break;
        case ActivationType::Softmax: {
            std::span<const float> out_span(lastOutput_.rawData(), lastOutput_.size());
            // Pass lastOutput_ instead of lastInput_ for Softmax backward
            Device::activeBackend()->softmaxBackward(out_span, go_span, gi_span, gradOutput.shape().back());
            break;
        }
        case ActivationType::LeakyReLU:
            Device::activeBackend()->leakyReluBackward(in_span, go_span, gi_span, alpha_);
            break;
        case ActivationType::GELU:
            Device::activeBackend()->geluBackward(in_span, go_span, gi_span);
            break;
        case ActivationType::SiLU:
            Device::activeBackend()->siluBackward(in_span, go_span, gi_span);
            break;
    }
    return gradInput;
}

} // namespace nn
