#include "nn/Autograd/ModuleBackward.hpp"
#include <vector>

namespace nn {

ModuleBackward::ModuleBackward(BackwardFn fn, Tensor input)
    : backwardFn_(std::move(fn)), input_(std::move(input)) {}

std::vector<Tensor> ModuleBackward::backward(const Tensor& gradOutput) {
    Tensor gradInput = backwardFn_(gradOutput);
    return {gradInput};
}

std::vector<Tensor> ModuleBackward::inputs() const {
    return {input_};
}

}  // namespace nn
