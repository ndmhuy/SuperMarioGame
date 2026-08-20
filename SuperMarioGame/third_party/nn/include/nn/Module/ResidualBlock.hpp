#pragma once

#include "nn/Module/Module.hpp"
#include "nn/Tensor/Tensor.hpp"
#include <memory>

namespace nn {

class ResidualBlock : public Module {
   public:
    explicit ResidualBlock(std::unique_ptr<Module> wrappedModule);
    ResidualBlock(std::unique_ptr<Module> wrappedModule, std::unique_ptr<Module> shortcut);

    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradOutput);
    std::vector<Tensor*> parameters() override;
    std::map<std::string, Tensor*> namedParameters() override;

   private:
    std::unique_ptr<Module> wrappedModule_;
    std::unique_ptr<Module> shortcut_;
    Tensor lastInput_;
    Tensor lastWrappedOutput_;
    Tensor lastShortcutOutput_;
};

}  // namespace nn