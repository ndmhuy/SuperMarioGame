#pragma once

#include "nn/Compute/Backend.hpp"
#include <span>

namespace nn {

// Concrete Strategy: Naive scalar CPU implementation.
// Phase 2 will add NEON SIMD intrinsics to these same methods.
class CPUBackend : public Backend {
public:
    ~CPUBackend() override = default;

    void add(std::span<const float> a, std::span<const float> b, std::span<float> out) override;
    void sub(std::span<const float> a, std::span<const float> b, std::span<float> out) override;
    void mul(std::span<const float> a, std::span<const float> b, std::span<float> out) override;
    void div(std::span<const float> a, std::span<const float> b, std::span<float> out) override;

    void scalarMul(std::span<const float> a, float s, std::span<float> out) override;
    void scalarAdd(std::span<const float> a, float s, std::span<float> out) override;

    void matmul(std::span<const float> A, std::span<const float> B, std::span<float> C,
                int M, int K, int N) override;

    void relu(std::span<const float> in, std::span<float> out) override;
    void reluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) override;

    void sigmoid(std::span<const float> in, std::span<float> out) override;
    void sigmoidBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) override;

    void tanh(std::span<const float> in, std::span<float> out) override;
    void tanhBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) override;

    void softmax(std::span<const float> in, std::span<float> out, int numClasses) override;
    void softmaxBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, int numClasses) override;

    void leakyRelu(std::span<const float> in, std::span<float> out, float alpha) override;
    void leakyReluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, float alpha) override;

    void gelu(std::span<const float> in, std::span<float> out) override;
    void geluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) override;

    void silu(std::span<const float> in, std::span<float> out) override;
    void siluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) override;

    std::string name() const override { return "CPUBackend"; }

    // Transposed matmul
    void matmulTransA(std::span<const float> A, std::span<const float> B,
                      std::span<float> C, int M, int K, int N) override;
    void matmulTransB(std::span<const float> A, std::span<const float> B,
                      std::span<float> C, int M, int K, int N) override;

    // Loss functions
    void mseLoss(std::span<const float> pred, std::span<const float> target,
                 float& loss, int N) override;
    void mseLossGrad(std::span<const float> pred, std::span<const float> target,
                     std::span<float> grad, int N) override;
    void crossEntropyLoss(std::span<const float> pred, std::span<const float> target,
                          float& loss, int batchSize) override;
    void crossEntropyLossGrad(std::span<const float> pred, std::span<const float> target,
                              std::span<float> grad, int batchSize) override;

    // BatchNorm fused
    void batchNormForward(std::span<const float> in, std::span<float> out,
                          std::span<float> mean, std::span<float> var,
                          std::span<float> xHat, std::span<float> stdInv,
                          std::span<const float> gamma, std::span<const float> beta,
                          std::span<float> runMean, std::span<float> runVar,
                          int batch, int features, float eps, float momentum,
                          bool training) override;
    void batchNormBackward(std::span<const float> gradOut, std::span<const float> xHat,
                           std::span<const float> gamma, std::span<const float> stdInv,
                           std::span<float> gradIn, std::span<float> dGamma,
                           std::span<float> dBeta, int batch, int features,
                           bool training, std::span<const float> runVar, float eps) override;

    // Optimizer kernels
    void sgdStep(std::span<float> param, std::span<const float> grad,
                 std::span<float> velocity, float lr, float momentum) override;
    void adamStep(std::span<float> param, std::span<const float> grad,
                  std::span<float> m, std::span<float> v,
                  float lr, float b1, float b2, float eps,
                  float bc1, float bc2, int N) override;
    void rmspropStep(std::span<float> param, std::span<const float> grad,
                     std::span<float> v, float lr, float alpha, float eps, int N) override;
    void adamwStep(std::span<float> param, std::span<const float> grad,
                   std::span<float> m, std::span<float> v,
                   float lr, float b1, float b2, float eps,
                   float bc1, float bc2, float weightDecay, int N) override;

    // Singleton access (global CPU backend instance)
    static CPUBackend& instance();
};

} // namespace nn
