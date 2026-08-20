#include "nn/Module/ResidualBlock.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include "nn/Autograd/ModuleBackward.hpp"

namespace nn {

ResidualBlock::ResidualBlock(std::unique_ptr<Module> wrappedModule)
    : wrappedModule_(std::move(wrappedModule)), shortcut_(nullptr) {
    if (wrappedModule_) {
        registerModule("wrapped", wrappedModule_.get());
    }
}

ResidualBlock::ResidualBlock(std::unique_ptr<Module> wrappedModule, std::unique_ptr<Module> shortcut)
    : wrappedModule_(std::move(wrappedModule)), shortcut_(std::move(shortcut)) {
    if (wrappedModule_) {
        registerModule("wrapped", wrappedModule_.get());
    }
    if (shortcut_) {
        registerModule("shortcut", shortcut_.get());
    }
}

Tensor ResidualBlock::forward(const Tensor& input) {
    if (GradMode::is_enabled()) {
        lastInput_ = input; // Shared view (Storage + AutogradMeta)
        if (shortcut_) {
            lastShortcutOutput_ = shortcut_->forward(input);
            lastWrappedOutput_ = wrappedModule_->forward(input);
            Tensor output = lastShortcutOutput_ + lastWrappedOutput_;
            
            output.setRequiresGrad(true);
            output.setGradFn(std::make_shared<ModuleBackward>(
                [this](const Tensor& g) { return this->backward(g); },
                input
            ));
            return output;
        } else {
            lastWrappedOutput_ = wrappedModule_->forward(input);
            Tensor output = input + lastWrappedOutput_;
            
            output.setRequiresGrad(true);
            output.setGradFn(std::make_shared<ModuleBackward>(
                [this](const Tensor& g) { return this->backward(g); },
                input
            ));
            return output;
        }
    } else {
        if (shortcut_) {
            return shortcut_->forward(input) + wrappedModule_->forward(input);
        } else {
            return input + wrappedModule_->forward(input);
        }
    }
}

Tensor ResidualBlock::backward(const Tensor& gradOutput) {
    // Temporarily save and clear lastInput_'s gradient to isolate gradients from the branches
    std::shared_ptr<Tensor> originalGrad = lastInput_.grad();
    lastInput_.setGrad(nullptr);

    // 1. Backpropagate gradOutput through lastWrappedOutput_
    lastWrappedOutput_.backward(gradOutput);
    Tensor subGradInput(lastInput_.shape());
    subGradInput.fill(0.0f);
    if (lastInput_.grad()) {
        subGradInput += *lastInput_.grad();
    }

    // 2. If there is a shortcut module, backpropagate through it; otherwise, identity shortcut
    if (shortcut_) {
        lastInput_.setGrad(nullptr); // clear again to get just shortcut's gradient
        lastShortcutOutput_.backward(gradOutput);
        if (lastInput_.grad()) {
            subGradInput += *lastInput_.grad();
        }
    } else {
        subGradInput += gradOutput;
    }

    // Restore the original input gradient
    lastInput_.setGrad(originalGrad);

    return subGradInput;
}

std::vector<Tensor*> ResidualBlock::parameters() {
    return Module::parameters();
}

std::map<std::string, Tensor*> ResidualBlock::namedParameters() {
    return Module::namedParameters();
}

}  // namespace nn