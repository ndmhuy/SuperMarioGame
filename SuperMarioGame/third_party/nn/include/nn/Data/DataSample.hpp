#pragma once

#include "nn/Tensor/Tensor.hpp"

namespace nn {

struct DataSample {
    Tensor features;
    Tensor label;

    DataSample(Tensor f, Tensor l) : features(std::move(f)), label(std::move(l)) {}
};

} // namespace nn
