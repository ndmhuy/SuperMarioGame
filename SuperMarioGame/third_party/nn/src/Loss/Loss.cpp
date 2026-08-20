#include "nn/Loss/Loss.hpp"
#include "nn/Autograd/ModuleBackward.hpp"
#include "nn/Autograd/NoGrad.hpp"

namespace nn {

Tensor Loss::forward(const Tensor& input, const Tensor& target) {
    if (input.shape() != target.shape()) {
        throw std::invalid_argument("Loss: Input and target shapes must match");
    }

    Tensor lossVal = compute(input, target);

    if (GradMode::is_enabled() && (input.requiresGrad() || target.requiresGrad())) {
        lossVal.setRequiresGrad(true);
        
        // Clone input and target to preserve values for the backward pass
        Tensor in_clone = input.clone();
        Tensor tgt_clone = target.clone();

        lossVal.setGradFn(std::make_shared<ModuleBackward>(
            [this, in_clone, tgt_clone](const Tensor& g) {
                Tensor rawGrad = this->gradient(in_clone, tgt_clone);
                float go = g.flat(0);
                if (go != 1.0f) {
                    float* ptr = rawGrad.rawData();
                    for (int i = 0; i < rawGrad.size(); ++i) {
                        ptr[i] *= go;
                    }
                }
                return rawGrad;
            },
            input
        ));
    }

    return lossVal;
}

} // namespace nn
