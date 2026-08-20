#pragma once

#include <memory>

namespace nn {

class Tensor;
class GradFn;

// Shared autograd metadata for Tensor.
// All copies/views of a Tensor share the same AutogradMeta via shared_ptr,
// so setting grad on any copy updates ALL copies — including the original
// leaf weight tensor held by the optimizer.
//
// This mirrors PyTorch's AutogradMeta inside TensorImpl.
struct AutogradMeta {
    bool requiresGrad{false};
    std::shared_ptr<Tensor> grad;
    std::shared_ptr<GradFn> gradFn;
};

} // namespace nn
