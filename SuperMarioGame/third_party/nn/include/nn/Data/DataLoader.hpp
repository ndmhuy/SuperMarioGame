#pragma once

#include "nn/Data/DataSample.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <span>

namespace nn {

// DataLoader provides shuffling and batched access to a dataset.
// In the C# Perceptron project, DatasetLoader combined data generation with
// iteration. Here we separate concerns: DataSample holds data, DataLoader
// handles shuffling and batching.
//
// Phase 3 (Task 3.18) will add a proper Iterator interface for range-based for.

class DataLoader {
public:
    DataLoader(const std::vector<DataSample>& samples, int batchSize);

    // Shuffle the dataset (Fisher-Yates via std::shuffle)
    void shuffle();

    // Get a batch of samples by batch index
    // Returns samples[batchIndex * batchSize_ .. (batchIndex+1) * batchSize_)
    std::span<const DataSample> getBatch(int batchIndex) const;

    // Number of complete batches (drops incomplete last batch)
    int numBatches() const;

    // Total number of samples
    int numSamples() const;

    // Direct access to individual sample (for single-sample training like C# Program.cs)
    const DataSample& operator[](int index) const;

    // ── Iterator Pattern (Task 3.18) ──
    class Iterator {
    public:
        Iterator(const DataLoader* loader, int batchIndex) : loader_(loader), batchIndex_(batchIndex) {}

        std::span<const DataSample> operator*() const {
            return loader_->getBatch(batchIndex_);
        }

        Iterator& operator++() {
            batchIndex_++;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return batchIndex_ != other.batchIndex_;
        }

    private:
        const DataLoader* loader_;
        int batchIndex_;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, numBatches()); }

private:
    std::vector<DataSample> samples_;
    int batchSize_;
    std::mt19937 rng_;
};

} // namespace nn
