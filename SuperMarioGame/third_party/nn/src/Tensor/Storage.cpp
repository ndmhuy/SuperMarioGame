#include "nn/Tensor/Storage.hpp"

namespace nn {

Storage::Storage(int size) : data_(size, 0.0f) {}

float* Storage::data() { return data_.data(); }

const float* Storage::data() const { return data_.data(); }

int Storage::size() const { return data_.size(); }

}  // namespace nn
