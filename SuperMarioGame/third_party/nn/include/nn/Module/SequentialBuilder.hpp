#pragma once

#include "nn/Module/Sequential.hpp"
#include <memory>
#include <utility>

namespace nn {

// SequentialBuilder implements the Builder design pattern.
// It provides a fluent interface to construct Sequential models.
class SequentialBuilder {
public:
    SequentialBuilder() : sequential_(std::make_unique<Sequential>()) {}

    // Add a module constructed in-place with the given arguments.
    template <typename ModuleType, typename... Args>
    SequentialBuilder& add(Args&&... args) {
        sequential_->add(std::make_unique<ModuleType>(std::forward<Args>(args)...));
        return *this;
    }

    // Add an already constructed unique_ptr to a module.
    SequentialBuilder& add(std::unique_ptr<Module> module) {
        sequential_->add(std::move(module));
        return *this;
    }

    // Build and return the completed Sequential model.
    std::unique_ptr<Sequential> build() {
        return std::move(sequential_);
    }

private:
    std::unique_ptr<Sequential> sequential_;
};

} // namespace nn
