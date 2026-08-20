#include "nn/Module/Sequential.hpp"

namespace nn {

// ── Add module ──
// Takes ownership of the module via unique_ptr (no object slicing).

void Sequential::add(std::unique_ptr<Module> module) {
    std::string name = "layer_" + std::to_string(modules_.size());
    registerModule(name, module.get());
    modules_.push_back(std::move(module));
}

// ── Forward pass (Chain of Responsibility) ──
// Data flows through each module sequentially: Module1 → Module2 → ... → ModuleN.
// This is identical in concept to C# NeuralNetwork.Predict(),
// but uses polymorphic dispatch via unique_ptr instead of casting.
//
// Each leaf module (Linear, Activation) attaches its own ModuleBackward
// GradFn node during its forward() call. The autograd graph is built
// automatically — Sequential does NOT need a backward() method.

Tensor Sequential::forward(const Tensor& input) {
    Tensor current = input;
    for (auto& module : modules_) {
        current = module->forward(current);
    }
    return current;
}

} // namespace nn
