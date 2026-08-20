#include "nn/Module/Module.hpp"

namespace nn {
// bool training_{true};
// std::vector<std::pair<std::string, Module*>> submodules_;

std::vector<Tensor*> Module::parameters() {
    std::vector<Tensor*> params;
    for (auto& [name, submodule] : submodules_) {
        auto sub_params = submodule->parameters();
        params.insert(params.end(), sub_params.begin(), sub_params.end());
    }
    return std::move(params);
}

void Module::registerModule(const std::string& name, Module* module) { submodules_.push_back({name, module}); }
 
void Module::train(bool mode) {
    training_ = mode;
    for (auto& [name, submodule] : submodules_) {
        submodule->train(mode);
    }
}

std::map<std::string, Tensor*> Module::namedParameters() {
    std::map<std::string, Tensor*> params;
    for (auto& [name, submodule] : submodules_) {
        auto sub_params = submodule->namedParameters();
        for (auto& [sub_name, sub_param] : sub_params) {
            params[name + "." + sub_name] = sub_param;
        }
    }
    return std::move(params);
}

}  // namespace nn