#pragma once

// The definition of NeuralPolicy::Net, shared by the two C++20 translation
// units that need it (NeuralPolicy.cpp and PolicyTrainer.cpp).
//
// It lives in its own header rather than in NeuralPolicy.hpp because it names
// nn:: types, and NeuralPolicy.hpp is included all over the C++17 game. Anything
// that includes THIS header must be compiled as C++20 — see CMakeLists.txt §16b.

#include "Entities/NeuralPolicy.hpp"

#include "nn/Module/Sequential.hpp"

#include <vector>

struct NeuralPolicy::Net {
    nn::Sequential model;
    std::vector<int> hidden;
};
