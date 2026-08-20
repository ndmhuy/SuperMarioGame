#pragma once
#include "nn/Autograd/GradFn.hpp"
#include "nn/Tensor/Tensor.hpp"
#include "nn/Core/ThreadPool.hpp"
#include <vector>
#include <string>

namespace nn {

template <TensorExpression Expr>
class ExprBackward : public GradFn {
public:
    ExprBackward(Expr expr)
        : expr_(std::move(expr)) {
        expr_.collect_leaves(leaves_);
    }

    std::string name() const override { return "ExprBackward"; }

    std::vector<Tensor> backward(const Tensor& gradOutput) override {
        // 1. Pre-allocate/zero leaf gradients safely before multi-threaded propagation.
        // We do this to avoid race conditions when allocating/initializing grad_ pointers inside the parallel loop.
        for (Tensor* leaf : leaves_) {
            if (leaf->requiresGrad()) {
                if (!leaf->grad()) {
                    leaf->setGrad(std::make_shared<Tensor>(leaf->shape()));
                    leaf->grad()->fill(0.0f);
                }
            }
        }

        // 2. Perform fused parallel gradient propagation using NEON and ThreadPool
        const float* go = gradOutput.rawData();
        int totalSize = gradOutput.size();

        ThreadPool::instance().parallelFor(0, totalSize, [this, go](int start, int end) {
            int i = start;
#ifdef __ARM_NEON
            for (; i + 3 < end; i += 4) {
                float32x4_t g = vld1q_f32(&go[i]);
                expr_.propagate_grad_neon(i, g);
            }
#endif
            for (; i < end; ++i) {
                expr_.propagate_grad(i, go[i]);
            }
        });

        // 3. Return gradients corresponding to leaves.
        std::vector<Tensor> input_grads;
        input_grads.reserve(leaves_.size());
        for (Tensor* leaf : leaves_) {
            if (leaf->grad()) {
                input_grads.push_back(*leaf->grad());
            } else {
                input_grads.push_back(Tensor(leaf->shape())); // Fallback
            }
        }
        return input_grads;
    }

    std::vector<Tensor> inputs() const override {
        std::vector<Tensor> result;
        result.reserve(leaves_.size());
        for (Tensor* leaf : leaves_) {
            result.push_back(*leaf);
        }
        return result;
    }

private:
    Expr expr_;
    std::vector<Tensor*> leaves_;
};

} // namespace nn
