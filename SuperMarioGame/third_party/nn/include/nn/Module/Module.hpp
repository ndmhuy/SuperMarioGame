#pragma once

#include <vector>
#include <map>

namespace nn {

class Tensor;

class Module {
   public:
    virtual ~Module() = default;

    virtual Tensor forward(const Tensor& input) = 0;

    virtual std::vector<Tensor*> parameters();

    void train(bool mode = true);
    void eval() { train(false); }
    bool isTraining() const { return training_; }

    void registerModule(const std::string& name, Module* module);

    virtual std::map<std::string, Tensor*> namedParameters();

   protected:
    bool training_{true};
    std::vector<std::pair<std::string, Module*>> submodules_;
};

}  // namespace nn