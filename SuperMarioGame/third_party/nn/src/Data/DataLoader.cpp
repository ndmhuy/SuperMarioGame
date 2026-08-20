#include "nn/Data/DataLoader.hpp"
#include <stdexcept>

namespace nn {

DataLoader::DataLoader(const std::vector<DataSample>& samples, int batchSize)
    : samples_(samples), batchSize_(batchSize), rng_(std::random_device{}()) {}

// ── Shuffle ──
// Fisher-Yates shuffle via std::shuffle with Mersenne Twister.
// In the C# project, shuffling was not implemented.
// Here we use the modern C++ <random> library as specified in AGENTS.md.

void DataLoader::shuffle() {
    std::shuffle(samples_.begin(), samples_.end(), rng_);
}

// ── Batch access ──
// Returns a vector of DataSamples for the given batch index.
// Matches the C# DatasetLoader[index] pattern but operates on batches.

std::span<const DataSample> DataLoader::getBatch(int batchIndex) const {
    int start = batchIndex * batchSize_;
    int end = std::min(start + batchSize_, static_cast<int>(samples_.size()));
    if (start >= static_cast<int>(samples_.size())) {
        throw std::out_of_range("DataLoader::getBatch: batch index out of range");
    }
    return std::span<const DataSample>(samples_.data() + start, end - start);
}

// ── Batch count ──
// Number of complete batches. Drops incomplete last batch
// to avoid shape mismatches in batch-mode operations.

int DataLoader::numBatches() const {
    return static_cast<int>(samples_.size()) / batchSize_;
}

int DataLoader::numSamples() const {
    return static_cast<int>(samples_.size());
}

const DataSample& DataLoader::operator[](int index) const {
    return samples_[index];
}

} // namespace nn
