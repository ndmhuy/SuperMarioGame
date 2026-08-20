#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"
#include <vector>
#include <memory>

namespace nn {

// Sequential — container that chains modules in order (Chain of Responsibility).
//
// In the "Pure Module" architecture, Sequential inherits Module directly
// (NOT Layer). It has NO backward() method — the autograd graph is built
// automatically during forward() by each leaf module attaching its own
// ModuleBackward GradFn node. Calling output.backward(grad) traverses
// the entire graph without Sequential needing to know about gradients.
//
// This matches PyTorch's nn.Sequential exactly: it's just a container
// that runs forward() through each child module in order.

class Sequential : public Module {
public:
    Sequential() = default;

    // Add a module to the sequence and register as submodule
    void add(std::unique_ptr<Module> module);

    // Forward pass through all modules (Chain of Responsibility)
    // Each module's forward() attaches its own GradFn to the output
    Tensor forward(const Tensor& input) override;

private:
    std::vector<std::unique_ptr<Module>> modules_;
};

} // namespace nn
