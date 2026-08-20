#pragma once

#include "nn/Core/AlignedAllocator.hpp"
#include <vector>

namespace nn {

// Flyweight: Shared tensor data storage
class Storage {
public:
    // TODO: Constructor to allocate size elements
    explicit Storage(int size);

    // TODO: Get raw pointer
    float* data();
    const float* data() const;

    // TODO: Get size
    int size() const;

private:
    // TODO: Store data using std::vector with AlignedAllocator
    std::vector<float, AlignedAllocator<float>> data_;
};

} // namespace nn
