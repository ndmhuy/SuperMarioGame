#pragma once

#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>

namespace nn {

// TrainingCallback represents the Observer design pattern base class.
// It allows observer components to subscribe to training lifecycle events.
class TrainingCallback {
public:
    virtual ~TrainingCallback() = default;

    virtual void onEpochBegin(int epoch) {}
    virtual void onEpochEnd(int epoch, float trainLoss, float valLoss) {}
    virtual void onBatchBegin(int batch) {}
    virtual void onBatchEnd(int batch, float loss) {}
    virtual void onTrainingBegin() {}
    virtual void onTrainingEnd() {}
};

// PrintLogger prints training loss metrics periodically.
class PrintLogger : public TrainingCallback {
public:
    explicit PrintLogger(int frequency = 1) : frequency_(frequency) {}

    void onEpochEnd(int epoch, float trainLoss, float valLoss) override {
        if (epoch % frequency_ == 0) {
            std::cout << "  Epoch " << std::setw(3) << epoch << " | Train Loss: " << std::fixed << std::setprecision(6) << trainLoss;
            if (valLoss >= 0.0f) {
                std::cout << " | Val Loss: " << valLoss;
            }
            std::cout << std::endl;
        }
    }

private:
    int frequency_;
};

// EarlyStopping halts training if the validation loss does not improve for a number of epochs.
class EarlyStopping : public TrainingCallback {
public:
    explicit EarlyStopping(int patience, float minDelta = 0.0f)
        : patience_(patience), minDelta_(minDelta), bestLoss_(std::numeric_limits<float>::infinity()), patienceCounter_(0), stop_(false) {}

    void onEpochEnd(int epoch, float trainLoss, float valLoss) override {
        float currentLoss = (valLoss >= 0.0f) ? valLoss : trainLoss;
        if (currentLoss < bestLoss_ - minDelta_) {
            bestLoss_ = currentLoss;
            patienceCounter_ = 0;
        } else {
            patienceCounter_++;
            if (patienceCounter_ >= patience_) {
                stop_ = true;
            }
        }
    }

    bool shouldStop() const { return stop_; }
    void reset() {
        bestLoss_ = std::numeric_limits<float>::infinity();
        patienceCounter_ = 0;
        stop_ = false;
    }

private:
    int patience_;
    float minDelta_;
    float bestLoss_;
    int patienceCounter_;
    bool stop_;
};

} // namespace nn
