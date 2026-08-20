#include "nn/Compute/CPUBackend.hpp"
#include "nn/Core/ThreadPool.hpp"
#include <arm_neon.h>
#include <cmath>

namespace nn {

// ── Singleton ──
CPUBackend& CPUBackend::instance() {
    static CPUBackend backend;
    return backend;
}

// ── Element-wise arithmetic ──

void CPUBackend::add(std::span<const float> a, std::span<const float> b, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, b, out](int start, int end) {
        int i = start;
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            vst1q_f32(&out[i], vaddq_f32(va, vb));
        }
        for (; i < end; ++i) {
            out[i] = a[i] + b[i];
        }
    });
}

void CPUBackend::sub(std::span<const float> a, std::span<const float> b, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, b, out](int start, int end) {
        int i = start;
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            vst1q_f32(&out[i], vsubq_f32(va, vb));
        }
        for (; i < end; ++i) {
            out[i] = a[i] - b[i];
        }
    });
}

void CPUBackend::mul(std::span<const float> a, std::span<const float> b, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, b, out](int start, int end) {
        int i = start;
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            vst1q_f32(&out[i], vmulq_f32(va, vb));
        }
        for (; i < end; ++i) {
            out[i] = a[i] * b[i];
        }
    });
}

void CPUBackend::div(std::span<const float> a, std::span<const float> b, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, b, out](int start, int end) {
        int i = start;
#if defined(__aarch64__)
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            vst1q_f32(&out[i], vdivq_f32(va, vb));
        }
#else
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            float32x4_t rec = vrecpeq_f32(vb);
            rec = vmulq_f32(vrecpsq_f32(vb, rec), rec);
            vst1q_f32(&out[i], vmulq_f32(va, rec));
        }
#endif
        for (; i < end; ++i) {
            out[i] = a[i] / b[i];
        }
    });
}

// ── Scalar operations ──

void CPUBackend::scalarMul(std::span<const float> a, float s, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, s, out](int start, int end) {
        int i = start;
        float32x4_t vs = vdupq_n_f32(s);
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            vst1q_f32(&out[i], vmulq_f32(va, vs));
        }
        for (; i < end; ++i) {
            out[i] = a[i] * s;
        }
    });
}

void CPUBackend::scalarAdd(std::span<const float> a, float s, std::span<float> out) {
    int n = a.size();
    ThreadPool::instance().parallelFor(0, n, [a, s, out](int start, int end) {
        int i = start;
        float32x4_t vs = vdupq_n_f32(s);
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            vst1q_f32(&out[i], vaddq_f32(va, vs));
        }
        for (; i < end; ++i) {
            out[i] = a[i] + s;
        }
    });
}

// ── Matrix multiplication ──

void CPUBackend::matmul(std::span<const float> A, std::span<const float> B, std::span<float> C, int M, int K, int N) {
    // Zero-fill output: single memset, no ThreadPool overhead
    std::memset(C.data(), 0, M * N * sizeof(float));

    constexpr int TILE = 32;
    ThreadPool::instance().parallelFor(0, M, [A, B, C, M, K, N, TILE](int start, int end) {
        for (int i0 = start; i0 < end; i0 += TILE) {
            for (int k0 = 0; k0 < K; k0 += TILE) {
                for (int j0 = 0; j0 < N; j0 += TILE) {
                    int i_end = std::min(i0 + TILE, end);
                    int k_end = std::min(k0 + TILE, K);
                    int j_end = std::min(j0 + TILE, N);
                    
                    for (int i = i0; i < i_end; ++i) {
                        for (int k = k0; k < k_end; ++k) {
                            float a_ik = A[i * K + k];
                            float32x4_t va = vdupq_n_f32(a_ik);
                            int j = j0;
                            for (; j + 3 < j_end; j += 4) {
                                float32x4_t vb = vld1q_f32(&B[k * N + j]);
                                float32x4_t vc = vld1q_f32(&C[i * N + j]);
                                vc = vfmaq_f32(vc, vb, va);
                                vst1q_f32(&C[i * N + j], vc);
                            }
                            for (; j < j_end; ++j) {
                                C[i * N + j] += a_ik * B[k * N + j];
                            }
                        }
                    }
                }
            }
        }
    });
}

// matmulTransA: C[M x N] = A^T[K x M] * B[K x N]
// A is stored as [K x M] (row=k, col=m), so A^T[m,k] = A[k*M + m]
void CPUBackend::matmulTransA(std::span<const float> A, std::span<const float> B, std::span<float> C, int M, int K, int N) {
    std::memset(C.data(), 0, M * N * sizeof(float));
    constexpr int TILE = 32;
    ThreadPool::instance().parallelFor(0, M, [A, B, C, M, K, N, TILE](int start, int end) {
        for (int i0 = start; i0 < end; i0 += TILE) {
            for (int k0 = 0; k0 < K; k0 += TILE) {
                for (int j0 = 0; j0 < N; j0 += TILE) {
                    int i_end = std::min(i0 + TILE, end);
                    int k_end = std::min(k0 + TILE, K);
                    int j_end = std::min(j0 + TILE, N);
                    for (int i = i0; i < i_end; ++i) {
                        for (int k = k0; k < k_end; ++k) {
                            float a_ki = A[k * M + i]; // A^T[i,k] = A[k,i]
                            float32x4_t va = vdupq_n_f32(a_ki);
                            int j = j0;
                            for (; j + 3 < j_end; j += 4) {
                                float32x4_t vb = vld1q_f32(&B[k * N + j]);
                                float32x4_t vc = vld1q_f32(&C[i * N + j]);
                                vc = vfmaq_f32(vc, vb, va);
                                vst1q_f32(&C[i * N + j], vc);
                            }
                            for (; j < j_end; ++j) {
                                C[i * N + j] += a_ki * B[k * N + j];
                            }
                        }
                    }
                }
            }
        }
    });
}

// matmulTransB: C[M x N] = A[M x K] * B^T[N x K]
// B is stored as [N x K] (row=n, col=k), so B^T[k,n] = B[n*K + k]
void CPUBackend::matmulTransB(std::span<const float> A, std::span<const float> B, std::span<float> C, int M, int K, int N) {
    std::memset(C.data(), 0, M * N * sizeof(float));
    constexpr int TILE = 32;
    ThreadPool::instance().parallelFor(0, M, [A, B, C, M, K, N, TILE](int start, int end) {
        for (int i0 = start; i0 < end; i0 += TILE) {
            for (int j0 = 0; j0 < N; j0 += TILE) {
                for (int k0 = 0; k0 < K; k0 += TILE) {
                    int i_end = std::min(i0 + TILE, end);
                    int j_end = std::min(j0 + TILE, N);
                    int k_end = std::min(k0 + TILE, K);
                    for (int i = i0; i < i_end; ++i) {
                        for (int j = j0; j < j_end; ++j) {
                            float32x4_t acc = vdupq_n_f32(0.0f);
                            int k = k0;
                            for (; k + 3 < k_end; k += 4) {
                                float32x4_t va = vld1q_f32(&A[i * K + k]);
                                float32x4_t vb = vld1q_f32(&B[j * K + k]); // B^T[k,j] = B[j,k]
                                acc = vfmaq_f32(acc, va, vb);
                            }
                            // Horizontal add of NEON accumulator
                            float sum = vaddvq_f32(acc);
                            for (; k < k_end; ++k) {
                                sum += A[i * K + k] * B[j * K + k];
                            }
                            C[i * N + j] += sum;
                        }
                    }
                }
            }
        }
    });
}

// ── Activations ──

void CPUBackend::relu(std::span<const float> in, std::span<float> out) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out](int start, int end) {
        int i = start;
        float32x4_t zero = vdupq_n_f32(0.0f);
        for (; i + 3 < end; i += 4) {
            float32x4_t va = vld1q_f32(&in[i]);
            vst1q_f32(&out[i], vmaxq_f32(va, zero));
        }
        for (; i < end; ++i) {
            out[i] = in[i] > 0.0f ? in[i] : 0.0f;
        }
    });
}

void CPUBackend::reluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn](int start, int end) {
        int i = start;
        float32x4_t zero = vdupq_n_f32(0.0f);
        for (; i + 3 < end; i += 4) {
            float32x4_t v_in = vld1q_f32(&in[i]);
            float32x4_t v_go = vld1q_f32(&gradOut[i]);
            uint32x4_t mask = vcgtq_f32(v_in, zero);
            vst1q_f32(&gradIn[i], vbslq_f32(mask, v_go, zero));
        }
        for (; i < end; ++i) {
            gradIn[i] = in[i] > 0.0f ? gradOut[i] : 0.0f;
        }
    });
}

void CPUBackend::sigmoid(std::span<const float> in, std::span<float> out) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out](int start, int end) {
        for (int i = start; i < end; ++i) {
            out[i] = 1.0f / (1.0f + std::exp(-in[i]));
        }
    });
}

void CPUBackend::sigmoidBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn](int start, int end) {
        for (int i = start; i < end; ++i) {
            float s = 1.0f / (1.0f + std::exp(-in[i]));
            gradIn[i] = gradOut[i] * s * (1.0f - s);
        }
    });
}

void CPUBackend::tanh(std::span<const float> in, std::span<float> out) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out](int start, int end) {
        for (int i = start; i < end; ++i) {
            out[i] = std::tanh(in[i]);
        }
    });
}

void CPUBackend::tanhBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn](int start, int end) {
        for (int i = start; i < end; ++i) {
            float t = std::tanh(in[i]);
            gradIn[i] = gradOut[i] * (1.0f - t * t);
        }
    });
}

void CPUBackend::softmax(std::span<const float> in, std::span<float> out, int numClasses) {
    int batchSize = in.size() / numClasses;
    ThreadPool::instance().parallelFor(0, batchSize, [in, out, numClasses](int start, int end) {
        for (int i = start; i < end; ++i) {
            int offset = i * numClasses;
            float maxVal = in[offset];
            for (int j = 1; j < numClasses; ++j) {
                if (in[offset + j] > maxVal) maxVal = in[offset + j];
            }
            float sumExp = 0.0f;
            for (int j = 0; j < numClasses; ++j) {
                float e = std::exp(in[offset + j] - maxVal);
                out[offset + j] = e;
                sumExp += e;
            }
            for (int j = 0; j < numClasses; ++j) {
                out[offset + j] /= sumExp;
            }
        }
    });
}

void CPUBackend::softmaxBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, int numClasses) {
    int batchSize = in.size() / numClasses;
    ThreadPool::instance().parallelFor(0, batchSize, [in, gradOut, gradIn, numClasses](int start, int end) {
        for (int i = start; i < end; ++i) {
            int offset = i * numClasses;
            float dot = 0.0f;
            for (int j = 0; j < numClasses; ++j) {
                dot += gradOut[offset + j] * in[offset + j];
            }
            for (int j = 0; j < numClasses; ++j) {
                gradIn[offset + j] = in[offset + j] * (gradOut[offset + j] - dot);
            }
        }
    });
}

void CPUBackend::leakyRelu(std::span<const float> in, std::span<float> out, float alpha) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out, alpha](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        float32x4_t alpha_v = vdupq_n_f32(alpha);
        float32x4_t zero_v = vdupq_n_f32(0.0f);
        for (; i + 3 < end; i += 4) {
            float32x4_t x = vld1q_f32(&in[i]);
            uint32x4_t mask = vcgtq_f32(x, zero_v);
            float32x4_t ax = vmulq_f32(x, alpha_v);
            vst1q_f32(&out[i], vbslq_f32(mask, x, ax));
        }
#endif
        for (; i < end; ++i) {
            out[i] = in[i] > 0.0f ? in[i] : alpha * in[i];
        }
    });
}

void CPUBackend::leakyReluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn, float alpha) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn, alpha](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        float32x4_t alpha_v = vdupq_n_f32(alpha);
        float32x4_t zero_v = vdupq_n_f32(0.0f);
        for (; i + 3 < end; i += 4) {
            float32x4_t x = vld1q_f32(&in[i]);
            float32x4_t go = vld1q_f32(&gradOut[i]);
            uint32x4_t mask = vcgtq_f32(x, zero_v);
            float32x4_t alpha_go = vmulq_f32(go, alpha_v);
            vst1q_f32(&gradIn[i], vbslq_f32(mask, go, alpha_go));
        }
#endif
        for (; i < end; ++i) {
            gradIn[i] = in[i] > 0.0f ? gradOut[i] : alpha * gradOut[i];
        }
    });
}

void CPUBackend::gelu(std::span<const float> in, std::span<float> out) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out](int start, int end) {
        for (int i = start; i < end; ++i) {
            float x = in[i];
            float phi = 0.79788456f * (x + 0.044715f * x * x * x);
            out[i] = 0.5f * x * (1.0f + std::tanh(phi));
        }
    });
}

void CPUBackend::geluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn](int start, int end) {
        for (int i = start; i < end; ++i) {
            float x = in[i];
            float phi = 0.79788456f * (x + 0.044715f * x * x * x);
            float tanh_val = std::tanh(phi);
            float sech2 = 1.0f - tanh_val * tanh_val;
            float d_phi = 0.79788456f + 0.107032224f * x * x;
            gradIn[i] = gradOut[i] * 0.5f * (1.0f + tanh_val + x * sech2 * d_phi);
        }
    });
}

void CPUBackend::silu(std::span<const float> in, std::span<float> out) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, out](int start, int end) {
        for (int i = start; i < end; ++i) {
            float x = in[i];
            out[i] = x / (1.0f + std::exp(-x));
        }
    });
}

void CPUBackend::siluBackward(std::span<const float> in, std::span<const float> gradOut, std::span<float> gradIn) {
    int n = in.size();
    ThreadPool::instance().parallelFor(0, n, [in, gradOut, gradIn](int start, int end) {
        for (int i = start; i < end; ++i) {
            float x = in[i];
            float s = 1.0f / (1.0f + std::exp(-x));
            gradIn[i] = gradOut[i] * s * (1.0f + x * (1.0f - s));
        }
    });
}

// ── Loss functions ──

void CPUBackend::mseLoss(std::span<const float> pred, std::span<const float> target, float& loss, int N) {
    std::atomic<float> total_sum{0.0f};
    ThreadPool::instance().parallelFor(0, N, [&](int start, int end) {
        float local_sum = 0.0f;
        int i = start;
#ifdef __ARM_NEON
        float32x4_t sum_v = vdupq_n_f32(0.0f);
        for (; i + 3 < end; i += 4) {
            float32x4_t p = vld1q_f32(&pred[i]);
            float32x4_t t = vld1q_f32(&target[i]);
            float32x4_t diff = vsubq_f32(p, t);
            sum_v = vfmaq_f32(sum_v, diff, diff);
        }
        local_sum = vaddvq_f32(sum_v);
#endif
        for (; i < end; ++i) {
            float diff = pred[i] - target[i];
            local_sum += diff * diff;
        }
        float current = total_sum.load(std::memory_order_relaxed);
        while (!total_sum.compare_exchange_weak(current, current + local_sum, std::memory_order_relaxed)) {}
    });
    loss = total_sum.load() / (2.0f * static_cast<float>(N));
}

void CPUBackend::mseLossGrad(std::span<const float> pred, std::span<const float> target, std::span<float> grad, int N) {
    float invN = 1.0f / static_cast<float>(N);
    ThreadPool::instance().parallelFor(0, N, [pred, target, grad, invN](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        float32x4_t invN_v = vdupq_n_f32(invN);
        for (; i + 3 < end; i += 4) {
            float32x4_t p = vld1q_f32(&pred[i]);
            float32x4_t t = vld1q_f32(&target[i]);
            float32x4_t diff = vsubq_f32(p, t);
            vst1q_f32(&grad[i], vmulq_f32(diff, invN_v));
        }
#endif
        for (; i < end; ++i) {
            grad[i] = (pred[i] - target[i]) * invN;
        }
    });
}

void CPUBackend::crossEntropyLoss(std::span<const float> pred, std::span<const float> target, float& loss, int batchSize) {
    int n = pred.size();
    std::atomic<float> total_sum{0.0f};
    const float eps = 1e-7f;
    ThreadPool::instance().parallelFor(0, n, [&](int start, int end) {
        float local_sum = 0.0f;
        for (int i = start; i < end; ++i) {
            if (target[i] > 0.5f) {
                local_sum -= std::log(pred[i] + eps);
            }
        }
        float current = total_sum.load(std::memory_order_relaxed);
        while (!total_sum.compare_exchange_weak(current, current + local_sum, std::memory_order_relaxed)) {}
    });
    loss = total_sum.load() / static_cast<float>(batchSize);
}

void CPUBackend::crossEntropyLossGrad(std::span<const float> pred, std::span<const float> target, std::span<float> grad, int batchSize) {
    int n = pred.size();
    float invB = 1.0f / static_cast<float>(batchSize);
    const float eps = 1e-7f;
    ThreadPool::instance().parallelFor(0, n, [pred, target, grad, invB, eps](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        float32x4_t invB_v = vdupq_n_f32(invB);
        float32x4_t eps_v = vdupq_n_f32(eps);
        for (; i + 3 < end; i += 4) {
            float32x4_t p = vld1q_f32(&pred[i]);
            float32x4_t t = vld1q_f32(&target[i]);
            float32x4_t num = vmulq_f32(vnegq_f32(t), invB_v);
            float32x4_t denom = vaddq_f32(p, eps_v);
            float32x4_t g = vdivq_f32(num, denom);
            vst1q_f32(&grad[i], g);
        }
#endif
        for (; i < end; ++i) {
            grad[i] = -target[i] * invB / (pred[i] + eps);
        }
    });
}

// ── BatchNorm (fused 2-pass) ──

void CPUBackend::batchNormForward(std::span<const float> in, std::span<float> out,
                                  std::span<float> mean, std::span<float> var,
                                  std::span<float> xHat, std::span<float> stdInv,
                                  std::span<const float> gamma, std::span<const float> beta,
                                  std::span<float> runMean, std::span<float> runVar,
                                  int batch, int features, float eps, float momentum,
                                  bool training) {
    if (training) {
        std::memset(mean.data(), 0, features * sizeof(float));
        std::memset(var.data(), 0, features * sizeof(float));

        // 1. Compute mean
        for (int i = 0; i < batch; ++i) {
            int offset = i * features;
            int j = 0;
#ifdef __ARM_NEON
            for (; j + 3 < features; j += 4) {
                float32x4_t sum_v = vld1q_f32(&mean[j]);
                float32x4_t in_v = vld1q_f32(&in[offset + j]);
                vst1q_f32(&mean[j], vaddq_f32(sum_v, in_v));
            }
#endif
            for (; j < features; ++j) {
                mean[j] += in[offset + j];
            }
        }
        
        float invBatch = 1.0f / static_cast<float>(batch);
        int j = 0;
#ifdef __ARM_NEON
        float32x4_t invBatch_v = vdupq_n_f32(invBatch);
        for (; j + 3 < features; j += 4) {
            float32x4_t m_v = vld1q_f32(&mean[j]);
            vst1q_f32(&mean[j], vmulq_f32(m_v, invBatch_v));
        }
#endif
        for (; j < features; ++j) {
            mean[j] *= invBatch;
        }

        // 2. Compute variance
        for (int i = 0; i < batch; ++i) {
            int offset = i * features;
            int k = 0;
#ifdef __ARM_NEON
            for (; k + 3 < features; k += 4) {
                float32x4_t in_v = vld1q_f32(&in[offset + k]);
                float32x4_t m_v = vld1q_f32(&mean[k]);
                float32x4_t diff = vsubq_f32(in_v, m_v);
                float32x4_t v_v = vld1q_f32(&var[k]);
                v_v = vfmaq_f32(v_v, diff, diff);
                vst1q_f32(&var[k], v_v);
            }
#endif
            for (; k < features; ++k) {
                float diff = in[offset + k] - mean[k];
                var[k] += diff * diff;
            }
        }

        j = 0;
#ifdef __ARM_NEON
        for (; j + 3 < features; j += 4) {
            float32x4_t v_v = vld1q_f32(&var[j]);
            vst1q_f32(&var[j], vmulq_f32(v_v, invBatch_v));
        }
#endif
        for (; j < features; ++j) {
            var[j] *= invBatch;
        }

        // 3. Compute stdInv and update running stats
        j = 0;
#ifdef __ARM_NEON
        float32x4_t eps_v = vdupq_n_f32(eps);
        float32x4_t momentum_v = vdupq_n_f32(momentum);
        float32x4_t one_minus_mom_v = vdupq_n_f32(1.0f - momentum);
        for (; j + 3 < features; j += 4) {
            float32x4_t v_v = vld1q_f32(&var[j]);
            float32x4_t std_v = vsqrtq_f32(vaddq_f32(v_v, eps_v));
            float32x4_t std_inv_v = vdivq_f32(vdupq_n_f32(1.0f), std_v);
            vst1q_f32(&stdInv[j], std_inv_v);

            float32x4_t rm_v = vld1q_f32(&runMean[j]);
            float32x4_t rv_v = vld1q_f32(&runVar[j]);
            float32x4_t m_v = vld1q_f32(&mean[j]);

            rm_v = vaddq_f32(vmulq_f32(one_minus_mom_v, rm_v), vmulq_f32(momentum_v, m_v));
            rv_v = vaddq_f32(vmulq_f32(one_minus_mom_v, rv_v), vmulq_f32(momentum_v, v_v));

            vst1q_f32(&runMean[j], rm_v);
            vst1q_f32(&runVar[j], rv_v);
        }
#endif
        for (; j < features; ++j) {
            stdInv[j] = 1.0f / std::sqrt(var[j] + eps);
            runMean[j] = (1.0f - momentum) * runMean[j] + momentum * mean[j];
            runVar[j] = (1.0f - momentum) * runVar[j] + momentum * var[j];
        }

        // 4. Compute xHat and out
        ThreadPool::instance().parallelFor(0, batch, [in, out, mean, stdInv, xHat, gamma, beta, features](int start, int end) {
            for (int i = start; i < end; ++i) {
                int offset = i * features;
                int k = 0;
#ifdef __ARM_NEON
                for (; k + 3 < features; k += 4) {
                    float32x4_t in_v = vld1q_f32(&in[offset + k]);
                    float32x4_t m_v = vld1q_f32(&mean[k]);
                    float32x4_t std_inv_v = vld1q_f32(&stdInv[k]);
                    float32x4_t xhat_v = vmulq_f32(vsubq_f32(in_v, m_v), std_inv_v);
                    vst1q_f32(&xHat[offset + k], xhat_v);

                    float32x4_t g_v = vld1q_f32(&gamma[k]);
                    float32x4_t b_v = vld1q_f32(&beta[k]);
                    float32x4_t out_v = vaddq_f32(vmulq_f32(g_v, xhat_v), b_v);
                    vst1q_f32(&out[offset + k], out_v);
                }
#endif
                for (; k < features; ++k) {
                    xHat[offset + k] = (in[offset + k] - mean[k]) * stdInv[k];
                    out[offset + k] = gamma[k] * xHat[offset + k] + beta[k];
                }
            }
        });
    } else {
        std::vector<float> stdInvVal(features);
        int j = 0;
#ifdef __ARM_NEON
        float32x4_t eps_v = vdupq_n_f32(eps);
        for (; j + 3 < features; j += 4) {
            float32x4_t rv_v = vld1q_f32(&runVar[j]);
            float32x4_t std_v = vsqrtq_f32(vaddq_f32(rv_v, eps_v));
            vst1q_f32(&stdInvVal[j], vdivq_f32(vdupq_n_f32(1.0f), std_v));
        }
#endif
        for (; j < features; ++j) {
            stdInvVal[j] = 1.0f / std::sqrt(runVar[j] + eps);
        }

        ThreadPool::instance().parallelFor(0, batch, [in, out, xHat, runMean, stdInvVal, gamma, beta, features](int start, int end) {
            for (int i = start; i < end; ++i) {
                int offset = i * features;
                int k = 0;
#ifdef __ARM_NEON
                for (; k + 3 < features; k += 4) {
                    float32x4_t in_v = vld1q_f32(&in[offset + k]);
                    float32x4_t rm_v = vld1q_f32(&runMean[k]);
                    float32x4_t std_inv_v = vld1q_f32(&stdInvVal[k]);
                    float32x4_t xhat_v = vmulq_f32(vsubq_f32(in_v, rm_v), std_inv_v);
                    
                    if (xHat.data()) {
                        vst1q_f32(&xHat[offset + k], xhat_v);
                    }

                    float32x4_t g_v = vld1q_f32(&gamma[k]);
                    float32x4_t b_v = vld1q_f32(&beta[k]);
                    float32x4_t out_v = vaddq_f32(vmulq_f32(g_v, xhat_v), b_v);
                    vst1q_f32(&out[offset + k], out_v);
                }
#endif
                for (; k < features; ++k) {
                    float normX = (in[offset + k] - runMean[k]) * stdInvVal[k];
                    if (xHat.data()) {
                        xHat[offset + k] = normX;
                    }
                    out[offset + k] = gamma[k] * normX + beta[k];
                }
            }
        });
    }
}

void CPUBackend::batchNormBackward(std::span<const float> gradOut, std::span<const float> xHat,
                                   std::span<const float> gamma, std::span<const float> stdInv,
                                   std::span<float> gradIn, std::span<float> dGamma,
                                   std::span<float> dBeta, int batch, int features,
                                   bool training, std::span<const float> runVar, float eps) {
    std::memset(dGamma.data(), 0, features * sizeof(float));
    std::memset(dBeta.data(), 0, features * sizeof(float));

    for (int i = 0; i < batch; ++i) {
        int offset = i * features;
        int j = 0;
#ifdef __ARM_NEON
        for (; j + 3 < features; j += 4) {
            float32x4_t go_v = vld1q_f32(&gradOut[offset + j]);
            float32x4_t xh_v = vld1q_f32(&xHat[offset + j]);
            
            float32x4_t dg_v = vld1q_f32(&dGamma[j]);
            dg_v = vfmaq_f32(dg_v, go_v, xh_v);
            vst1q_f32(&dGamma[j], dg_v);

            float32x4_t db_v = vld1q_f32(&dBeta[j]);
            db_v = vaddq_f32(db_v, go_v);
            vst1q_f32(&dBeta[j], db_v);
        }
#endif
        for (; j < features; ++j) {
            dGamma[j] += gradOut[offset + j] * xHat[offset + j];
            dBeta[j] += gradOut[offset + j];
        }
    }

    if (training) {
        float invB = 1.0f / static_cast<float>(batch);
        ThreadPool::instance().parallelFor(0, batch, [gradOut, xHat, gamma, stdInv, gradIn, dGamma, dBeta, batch, features, invB](int start, int end) {
            for (int i = start; i < end; ++i) {
                int offset = i * features;
                int j = 0;
#ifdef __ARM_NEON
                float32x4_t batch_v = vdupq_n_f32(static_cast<float>(batch));
                float32x4_t invB_v = vdupq_n_f32(invB);
                for (; j + 3 < features; j += 4) {
                    float32x4_t go_v = vld1q_f32(&gradOut[offset + j]);
                    float32x4_t xh_v = vld1q_f32(&xHat[offset + j]);
                    float32x4_t g_v = vld1q_f32(&gamma[j]);
                    float32x4_t si_v = vld1q_f32(&stdInv[j]);
                    float32x4_t dg_v = vld1q_f32(&dGamma[j]);
                    float32x4_t db_v = vld1q_f32(&dBeta[j]);

                    float32x4_t term1 = vmulq_f32(batch_v, go_v);
                    float32x4_t term3 = vmulq_f32(xh_v, dg_v);

                    float32x4_t diff = vsubq_f32(vsubq_f32(term1, db_v), term3);
                    float32x4_t dx_v = vmulq_f32(vmulq_f32(g_v, si_v), vmulq_f32(invB_v, diff));
                    vst1q_f32(&gradIn[offset + j], dx_v);
                }
#endif
                for (; j < features; ++j) {
                    float term1 = static_cast<float>(batch) * gradOut[offset + j];
                    float term2 = dBeta[j];
                    float term3 = xHat[offset + j] * dGamma[j];
                    gradIn[offset + j] = gamma[j] * stdInv[j] * invB * (term1 - term2 - term3);
                }
            }
        });
    } else {
        ThreadPool::instance().parallelFor(0, batch, [gradOut, gamma, runVar, gradIn, features, eps](int start, int end) {
            for (int i = start; i < end; ++i) {
                int offset = i * features;
                int j = 0;
#ifdef __ARM_NEON
                float32x4_t eps_v = vdupq_n_f32(eps);
                for (; j + 3 < features; j += 4) {
                    float32x4_t rv_v = vld1q_f32(&runVar[j]);
                    float32x4_t std_v = vsqrtq_f32(vaddq_f32(rv_v, eps_v));
                    float32x4_t std_inv_v = vdivq_f32(vdupq_n_f32(1.0f), std_v);
                    float32x4_t go_v = vld1q_f32(&gradOut[offset + j]);
                    float32x4_t g_v = vld1q_f32(&gamma[j]);

                    float32x4_t dx_v = vmulq_f32(go_v, vmulq_f32(g_v, std_inv_v));
                    vst1q_f32(&gradIn[offset + j], dx_v);
                }
#endif
                for (; j < features; ++j) {
                    float stdInvVal = 1.0f / std::sqrt(runVar[j] + eps);
                    gradIn[offset + j] = gradOut[offset + j] * gamma[j] * stdInvVal;
                }
            }
        });
    }
}

// ── Optimizer kernels ──

void CPUBackend::sgdStep(std::span<float> param, std::span<const float> grad,
                         std::span<float> velocity, float lr, float momentum) {
    int N = param.size();
    ThreadPool::instance().parallelFor(0, N, [param, grad, velocity, lr, momentum](int start, int end) {
        int i = start;
        if (momentum > 0.0f) {
#ifdef __ARM_NEON
            float32x4_t mom_v = vdupq_n_f32(momentum);
            float32x4_t lr_v = vdupq_n_f32(lr);
            for (; i + 3 < end; i += 4) {
                float32x4_t p = vld1q_f32(&param[i]);
                float32x4_t g = vld1q_f32(&grad[i]);
                float32x4_t v = vld1q_f32(&velocity[i]);

                v = vfmaq_f32(g, mom_v, v);
                vst1q_f32(&velocity[i], v);

                p = vsubq_f32(p, vmulq_f32(lr_v, v));
                vst1q_f32(&param[i], p);
            }
#endif
            for (; i < end; ++i) {
                velocity[i] = momentum * velocity[i] + grad[i];
                param[i] -= lr * velocity[i];
            }
        } else {
#ifdef __ARM_NEON
            float32x4_t lr_v = vdupq_n_f32(lr);
            for (; i + 3 < end; i += 4) {
                float32x4_t p = vld1q_f32(&param[i]);
                float32x4_t g = vld1q_f32(&grad[i]);
                p = vsubq_f32(p, vmulq_f32(lr_v, g));
                vst1q_f32(&param[i], p);
            }
#endif
            for (; i < end; ++i) {
                param[i] -= lr * grad[i];
            }
        }
    });
}

void CPUBackend::adamStep(std::span<float> param, std::span<const float> grad,
                          std::span<float> m, std::span<float> v,
                          float lr, float b1, float b2, float eps,
                          float bc1, float bc2, int N) {
    ThreadPool::instance().parallelFor(0, N, [param, grad, m, v, lr, b1, b2, eps, bc1, bc2](int start, int end) {
        int j = start;
#ifdef __ARM_NEON
        float32x4_t b1_v = vdupq_n_f32(b1);
        float32x4_t one_minus_b1_v = vdupq_n_f32(1.0f - b1);
        float32x4_t b2_v = vdupq_n_f32(b2);
        float32x4_t one_minus_b2_v = vdupq_n_f32(1.0f - b2);
        float32x4_t lr_v = vdupq_n_f32(lr);
        float32x4_t eps_v = vdupq_n_f32(eps);
        float32x4_t bc1_v = vdupq_n_f32(bc1);
        float32x4_t bc2_v = vdupq_n_f32(bc2);

        for (; j + 3 < end; j += 4) {
            float32x4_t m_v = vld1q_f32(&m[j]);
            float32x4_t v_v = vld1q_f32(&v[j]);
            float32x4_t g_v = vld1q_f32(&grad[j]);
            float32x4_t p_v = vld1q_f32(&param[j]);

            m_v = vaddq_f32(vmulq_f32(b1_v, m_v), vmulq_f32(one_minus_b1_v, g_v));
            vst1q_f32(&m[j], m_v);

            v_v = vaddq_f32(vmulq_f32(b2_v, v_v), vmulq_f32(one_minus_b2_v, vmulq_f32(g_v, g_v)));
            vst1q_f32(&v[j], v_v);

            float32x4_t m_hat = vdivq_f32(m_v, bc1_v);
            float32x4_t v_hat = vdivq_f32(v_v, bc2_v);

            float32x4_t sqrt_v = vsqrtq_f32(v_hat);
            float32x4_t update = vdivq_f32(vmulq_f32(lr_v, m_hat), vaddq_f32(sqrt_v, eps_v));
            p_v = vsubq_f32(p_v, update);
            vst1q_f32(&param[j], p_v);
        }
#endif
        for (; j < end; ++j) {
            m[j] = b1 * m[j] + (1.0f - b1) * grad[j];
            v[j] = b2 * v[j] + (1.0f - b2) * grad[j] * grad[j];
            float m_hat = m[j] / bc1;
            float v_hat = v[j] / bc2;
            param[j] -= lr * m_hat / (std::sqrt(v_hat) + eps);
        }
    });
}

void CPUBackend::rmspropStep(std::span<float> param, std::span<const float> grad,
                             std::span<float> v, float lr, float alpha, float eps, int N) {
    ThreadPool::instance().parallelFor(0, N, [param, grad, v, lr, alpha, eps](int start, int end) {
        int j = start;
#ifdef __ARM_NEON
        float32x4_t alpha_v = vdupq_n_f32(alpha);
        float32x4_t one_minus_alpha_v = vdupq_n_f32(1.0f - alpha);
        float32x4_t lr_v = vdupq_n_f32(lr);
        float32x4_t eps_v = vdupq_n_f32(eps);

        for (; j + 3 < end; j += 4) {
            float32x4_t v_v = vld1q_f32(&v[j]);
            float32x4_t g_v = vld1q_f32(&grad[j]);
            float32x4_t p_v = vld1q_f32(&param[j]);

            v_v = vaddq_f32(vmulq_f32(alpha_v, v_v), vmulq_f32(one_minus_alpha_v, vmulq_f32(g_v, g_v)));
            vst1q_f32(&v[j], v_v);

            float32x4_t sqrt_v = vsqrtq_f32(v_v);
            float32x4_t update = vdivq_f32(vmulq_f32(lr_v, g_v), vaddq_f32(sqrt_v, eps_v));
            p_v = vsubq_f32(p_v, update);
            vst1q_f32(&param[j], p_v);
        }
#endif
        for (; j < end; ++j) {
            v[j] = alpha * v[j] + (1.0f - alpha) * grad[j] * grad[j];
            param[j] -= lr * grad[j] / (std::sqrt(v[j]) + eps);
        }
    });
}

void CPUBackend::adamwStep(std::span<float> param, std::span<const float> grad,
                           std::span<float> m, std::span<float> v,
                           float lr, float b1, float b2, float eps,
                           float bc1, float bc2, float weightDecay, int N) {
    ThreadPool::instance().parallelFor(0, N, [param, grad, m, v, lr, b1, b2, eps, bc1, bc2, weightDecay](int start, int end) {
        int j = start;
#ifdef __ARM_NEON
        float32x4_t b1_v = vdupq_n_f32(b1);
        float32x4_t one_minus_b1_v = vdupq_n_f32(1.0f - b1);
        float32x4_t b2_v = vdupq_n_f32(b2);
        float32x4_t one_minus_b2_v = vdupq_n_f32(1.0f - b2);
        float32x4_t lr_v = vdupq_n_f32(lr);
        float32x4_t eps_v = vdupq_n_f32(eps);
        float32x4_t bc1_v = vdupq_n_f32(bc1);
        float32x4_t bc2_v = vdupq_n_f32(bc2);
        float32x4_t decay_factor = vdupq_n_f32(1.0f - lr * weightDecay);

        for (; j + 3 < end; j += 4) {
            float32x4_t m_v = vld1q_f32(&m[j]);
            float32x4_t v_v = vld1q_f32(&v[j]);
            float32x4_t g_v = vld1q_f32(&grad[j]);
            float32x4_t p_v = vld1q_f32(&param[j]);

            m_v = vaddq_f32(vmulq_f32(b1_v, m_v), vmulq_f32(one_minus_b1_v, g_v));
            vst1q_f32(&m[j], m_v);

            v_v = vaddq_f32(vmulq_f32(b2_v, v_v), vmulq_f32(one_minus_b2_v, vmulq_f32(g_v, g_v)));
            vst1q_f32(&v[j], v_v);

            float32x4_t m_hat = vdivq_f32(m_v, bc1_v);
            float32x4_t v_hat = vdivq_f32(v_v, bc2_v);

            float32x4_t sqrt_v = vsqrtq_f32(v_hat);
            float32x4_t update = vdivq_f32(vmulq_f32(lr_v, m_hat), vaddq_f32(sqrt_v, eps_v));
            
            p_v = vsubq_f32(vmulq_f32(p_v, decay_factor), update);
            vst1q_f32(&param[j], p_v);
        }
#endif
        for (; j < end; ++j) {
            m[j] = b1 * m[j] + (1.0f - b1) * grad[j];
            v[j] = b2 * v[j] + (1.0f - b2) * grad[j] * grad[j];
            float m_hat = m[j] / bc1;
            float v_hat = v[j] / bc2;
            param[j] = param[j] * (1.0f - lr * weightDecay) - lr * m_hat / (std::sqrt(v_hat) + eps);
        }
    });
}

}  // namespace nn
