#include "nn/Autograd/ActivationOps.hpp"
#include "nn/Core/Device.hpp"
#include <span>

namespace nn {

// ── ReluBackward ──
ReluBackward::ReluBackward(const Tensor& input, const Tensor& output) : input_(input) {}

std::vector<Tensor> ReluBackward::backward(const Tensor& gradOutput) {
    Tensor gradInput(gradOutput.shape());
    std::span<const float> in_span(input_.rawData(), input_.size());
    std::span<const float> go_span(gradOutput.rawData(), gradOutput.size());
    std::span<float> gi_span(gradInput.rawData(), gradInput.size());
    
    Device::activeBackend()->reluBackward(in_span, go_span, gi_span);
    return {gradInput};
}

// ── SigmoidBackward ──
SigmoidBackward::SigmoidBackward(const Tensor& input, const Tensor& output) : input_(input) {}

std::vector<Tensor> SigmoidBackward::backward(const Tensor& gradOutput) {
    Tensor gradInput(gradOutput.shape());
    std::span<const float> in_span(input_.rawData(), input_.size());
    std::span<const float> go_span(gradOutput.rawData(), gradOutput.size());
    std::span<float> gi_span(gradInput.rawData(), gradInput.size());
    
    Device::activeBackend()->sigmoidBackward(in_span, go_span, gi_span);
    return {gradInput};
}

// ── TanhBackward ──
TanhBackward::TanhBackward(const Tensor& input, const Tensor& output) : input_(input) {}

std::vector<Tensor> TanhBackward::backward(const Tensor& gradOutput) {
    Tensor gradInput(gradOutput.shape());
    std::span<const float> in_span(input_.rawData(), input_.size());
    std::span<const float> go_span(gradOutput.rawData(), gradOutput.size());
    std::span<float> gi_span(gradInput.rawData(), gradInput.size());
    
    Device::activeBackend()->tanhBackward(in_span, go_span, gi_span);
    return {gradInput};
}

// ── SoftmaxBackward ──
SoftmaxBackward::SoftmaxBackward(const Tensor& input, const Tensor& output) : input_(input), output_(output) {}

std::vector<Tensor> SoftmaxBackward::backward(const Tensor& gradOutput) {
    Tensor gradInput(gradOutput.shape());
    std::span<const float> out_span(output_.rawData(), output_.size());
    std::span<const float> go_span(gradOutput.rawData(), gradOutput.size());
    std::span<float> gi_span(gradInput.rawData(), gradInput.size());
    
    Device::activeBackend()->softmaxBackward(out_span, go_span, gi_span, gradOutput.shape().back());
    return {gradInput};
}

} // namespace nn
