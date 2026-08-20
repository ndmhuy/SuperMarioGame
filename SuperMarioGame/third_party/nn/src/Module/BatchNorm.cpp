#include "nn/Module/BatchNorm.hpp"
#include "nn/Autograd/ModuleBackward.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include "nn/Core/Device.hpp"
#include <cmath>
#include <stdexcept>

namespace nn {

BatchNorm::BatchNorm(int numFeatures, float eps, float momentum)
    : numFeatures_(numFeatures),
      eps_(eps),
      momentum_(momentum),
      gamma_({1, numFeatures}),
      beta_({1, numFeatures}),
      runningMean_({1, numFeatures}),
      runningVar_({1, numFeatures}),
      xHat_({1}),
      stdInv_({1}),
      mean_({1}),
      var_({1}) {
    
    gamma_.setRequiresGrad(true);
    beta_.setRequiresGrad(true);

    gamma_.fill(1.0f);
    beta_.fill(0.0f);

    runningMean_.fill(0.0f);
    runningVar_.fill(1.0f);
}

Tensor BatchNorm::forward(const Tensor& input) {
    if (input.rank() != 2) {
        throw std::invalid_argument("BatchNorm only supports 2D inputs [batchSize, features]");
    }
    int batchSize = input.shape()[0];
    int features = input.shape()[1];
    if (features != numFeatures_) {
        throw std::invalid_argument("Input feature size does not match numFeatures");
    }

    Tensor output(input.shape());

    if (training_) {
        mean_ = Tensor({1, features});
        var_ = Tensor({1, features});
        stdInv_ = Tensor({1, features});
        xHat_ = Tensor(input.shape());
    } else if (GradMode::is_enabled()) {
        xHat_ = Tensor(input.shape());
    }

    std::span<const float> inSpan(input.rawData(), input.size());
    std::span<float> outSpan(output.rawData(), output.size());
    
    std::span<float> meanSpan;
    std::span<float> varSpan;
    std::span<float> xHatSpan;
    std::span<float> stdInvSpan;

    if (training_) {
        meanSpan = std::span<float>(mean_.rawData(), mean_.size());
        varSpan = std::span<float>(var_.rawData(), var_.size());
        stdInvSpan = std::span<float>(stdInv_.rawData(), stdInv_.size());
        xHatSpan = std::span<float>(xHat_.rawData(), xHat_.size());
    } else if (GradMode::is_enabled()) {
        xHatSpan = std::span<float>(xHat_.rawData(), xHat_.size());
    }

    std::span<const float> gammaSpan(gamma_.rawData(), gamma_.size());
    std::span<const float> betaSpan(beta_.rawData(), beta_.size());
    std::span<float> runMeanSpan(runningMean_.rawData(), runningMean_.size());
    std::span<float> runVarSpan(runningVar_.rawData(), runningVar_.size());

    Device::activeBackend()->batchNormForward(
        inSpan, outSpan, meanSpan, varSpan, xHatSpan, stdInvSpan,
        gammaSpan, betaSpan, runMeanSpan, runVarSpan,
        batchSize, features, eps_, momentum_, training_
    );

    if (GradMode::is_enabled()) {
        output.setRequiresGrad(true);
        output.setGradFn(std::make_shared<ModuleBackward>(
            [this](const Tensor& g) { return this->backward(g); },
            input
        ));
    }

    return output;
}

Tensor BatchNorm::backward(const Tensor& gradOutput) {
    int batchSize = gradOutput.shape()[0];
    int features = gradOutput.shape()[1];

    Tensor dGamma({1, features});
    Tensor dBeta({1, features});
    Tensor gradInput(gradOutput.shape());

    std::span<const float> goSpan(gradOutput.rawData(), gradOutput.size());
    std::span<const float> xHatSpan(xHat_.rawData(), xHat_.size());
    std::span<const float> gammaSpan(gamma_.rawData(), gamma_.size());
    std::span<const float> stdInvSpan(stdInv_.rawData(), stdInv_.size());
    std::span<float> giSpan(gradInput.rawData(), gradInput.size());
    std::span<float> dgSpan(dGamma.rawData(), dGamma.size());
    std::span<float> dbSpan(dBeta.rawData(), dBeta.size());
    std::span<const float> runVarSpan(runningVar_.rawData(), runningVar_.size());

    Device::activeBackend()->batchNormBackward(
        goSpan, xHatSpan, gammaSpan, stdInvSpan,
        giSpan, dgSpan, dbSpan,
        batchSize, features, training_, runVarSpan, eps_
    );

    // Accumulate parameter gradients
    if (!gamma_.grad()) {
        gamma_.setGrad(std::make_shared<Tensor>(dGamma));
    } else {
        *gamma_.grad() += dGamma;
    }
    if (!beta_.grad()) {
        beta_.setGrad(std::make_shared<Tensor>(dBeta));
    } else {
        *beta_.grad() += dBeta;
    }

    return gradInput;
}

std::vector<Tensor*> BatchNorm::parameters() {
    return {&gamma_, &beta_};
}

std::map<std::string, Tensor*> BatchNorm::namedParameters() {
    return {{"gamma", &gamma_}, {"beta", &beta_}};
}

} // namespace nn