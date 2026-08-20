#pragma once

#include "nn/Tensor/Tensor.hpp"
#include <string>
#include <vector>

namespace nn {

// Checkpoint implements the Memento design pattern.
// It allows saving and restoring the weights of a list of Tensors to/from a binary file.
class Checkpoint {
public:
    static bool save(const std::string& filepath, const std::vector<Tensor*>& parameters);
    static bool load(const std::string& filepath, const std::vector<Tensor*>& parameters);
};

} // namespace nn
