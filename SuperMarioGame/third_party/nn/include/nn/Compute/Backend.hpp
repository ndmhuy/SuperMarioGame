#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <span>

namespace nn {

// Strategy pattern: Abstract interface for all math operations.
// Concrete backends (CPUBackend, MetalBackend) implement these.
// Methods operate on std::span so Tensor metadata stays decoupled but memory is bounded.
class Backend {
public:
    virtual ~Backend() = default;

    // Element-wise operations
    virtual void add(std::span<const float> a, std::span<const float> b, std::span<float> out) = 0;
    virtual void sub(std::span<const float> a, std::span<const float> b, std::span<float> out) = 0;
    virtual void mul(std::span<const float> a, std::span<const float> b, std::span<float> out) = 0;
    virtual void div(std::span<const float> a, std::span<const float> b, std::span<float> out) = 0;

    // Scalar operations
    virtual void scalarMul(std::span<const float> a, float s, std::span<float> out) = 0;
    virtual void scalarAdd(std::span<const float> a, float s, std::span<float> out) = 0;

    // Matrix multiplication: C[M x N] = A[M x K] * B[K x N]
    virtual void matmul(std::span<const float> A, std::span<const float> B, std::span<float> C,
                        int M, int K, int N) = 0;

    // Activation functions (vectorized)
    virtual void relu(std::span<const float> in, std::span<float> out) = 0;
    virtual void reluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) = 0;

    virtual void sigmoid(std::span<const float> in, std::span<float> out) = 0;
    virtual void sigmoidBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) = 0;

    virtual void tanh(std::span<const float> in, std::span<float> out) = 0;
    virtual void tanhBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) = 0;

    virtual void softmax(std::span<const float> in, std::span<float> out, int numClasses) = 0;
    virtual void softmaxBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, int numClasses) = 0;

    virtual void leakyRelu(std::span<const float> in, std::span<float> out, float alpha) = 0;
    virtual void leakyReluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, float alpha) = 0;

    virtual void gelu(std::span<const float> in, std::span<float> out) = 0;
    virtual void geluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) = 0;

    virtual void silu(std::span<const float> in, std::span<float> out) = 0;
    virtual void siluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) = 0;

    virtual std::string name() const = 0;

    // ── Transposed matmul variants ──
    // Avoids materializing .contiguous() copies in Linear::backward.
    // matmulTransA: C = A^T * B  — A is [K x M], B is [K x N], C is [M x N]
    // matmulTransB: C = A * B^T  — A is [M x K], B is [N x K], C is [M x N]
    virtual void matmulTransA(std::span<const float> A, std::span<const float> B,
                              std::span<float> C, int M, int K, int N) = 0;
    virtual void matmulTransB(std::span<const float> A, std::span<const float> B,
                              std::span<float> C, int M, int K, int N) = 0;

    // ── Loss functions ──
    virtual void mseLoss(std::span<const float> pred, std::span<const float> target,
                         float& loss, int N) = 0;
    virtual void mseLossGrad(std::span<const float> pred, std::span<const float> target,
                             std::span<float> grad, int N) = 0;
    virtual void crossEntropyLoss(std::span<const float> pred, std::span<const float> target,
                                  float& loss, int batchSize) = 0;
    virtual void crossEntropyLossGrad(std::span<const float> pred, std::span<const float> target,
                                      std::span<float> grad, int batchSize) = 0;

    // ── BatchNorm (fused 2-pass) ──
    virtual void batchNormForward(std::span<const float> in, std::span<float> out,
                                  std::span<float> mean, std::span<float> var,
                                  std::span<float> xHat, std::span<float> stdInv,
                                  std::span<const float> gamma, std::span<const float> beta,
                                  std::span<float> runMean, std::span<float> runVar,
                                  int batch, int features, float eps, float momentum,
                                  bool training) = 0;
    virtual void batchNormBackward(std::span<const float> gradOut, std::span<const float> xHat,
                                   std::span<const float> gamma, std::span<const float> stdInv,
                                   std::span<float> gradIn, std::span<float> dGamma,
                                   std::span<float> dBeta, int batch, int features,
                                   bool training, std::span<const float> runVar, float eps) = 0;

    // ── Optimizer kernels (single dispatch point for GPU) ──
    virtual void sgdStep(std::span<float> param, std::span<const float> grad,
                         std::span<float> velocity, float lr, float momentum) = 0;
    virtual void adamStep(std::span<float> param, std::span<const float> grad,
                          std::span<float> m, std::span<float> v,
                          float lr, float b1, float b2, float eps,
                          float bc1, float bc2, int N) = 0;
    virtual void rmspropStep(std::span<float> param, std::span<const float> grad,
                             std::span<float> v, float lr, float alpha, float eps, int N) = 0;
    virtual void adamwStep(std::span<float> param, std::span<const float> grad,
                           std::span<float> m, std::span<float> v,
                           float lr, float b1, float b2, float eps,
                           float bc1, float bc2, float weightDecay, int N) = 0;
};

} // namespace nn
