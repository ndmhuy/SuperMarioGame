#pragma once

#include "nn/Tensor/Storage.hpp"
#include "nn/Tensor/Expression.hpp"
#include "nn/Autograd/AutogradMeta.hpp"
#include "nn/Autograd/NoGrad.hpp"
#include <functional>
#include <initializer_list>
#include <memory>
#include <vector>

namespace nn {

class Backend;  // Forward declaration
class GradFn;   // Forward declaration

class LazyTensor;

class Tensor : public TensorBase<Tensor> {
   public:
    // ── Constructors ──
    Tensor() = default;
    // Create a zero-filled tensor with the given shape
    explicit Tensor(std::vector<int> shape);
    // Create a tensor from existing data with the given shape
    Tensor(std::vector<float> data, std::vector<int> shape);

    // Copy semantics: shares Storage (Flyweight / view semantics)
    Tensor(const Tensor& other);
    Tensor& operator=(const Tensor& other);

    // Move semantics
    Tensor(Tensor&& other) noexcept = default;
    Tensor& operator=(Tensor&& other) noexcept = default;

    // Expression Template Integration
    template <TensorExpression Expr>
    Tensor(const Expr& expr);

    template <TensorExpression Expr>
    Tensor& operator=(const Expr& expr);

    virtual ~Tensor() = default;

    // ── Accessors ──
    int rank() const;
    int size() const;
    const std::vector<int>& shape() const;
    const std::vector<int>& strides() const;
    int offset() const;

    // ── N-dim indexing ──
    // Usage: tensor({i, j, k}) accesses element at [i][j][k]
    float& operator()(std::initializer_list<int> indices);
    float operator()(std::initializer_list<int> indices) const;

    // Flat 1D access (ignores shape, directly indexes storage + offset)
    float& flat(int i);
    float flat(int i) const;

    // For expression templates (scalar fallback for tail elements)
    float operator[](size_t i) const { return flat(i); }

#ifdef __ARM_NEON
    // NEON: load 4 contiguous floats from raw data at position i.
    // This is the leaf node in the expression tree — all expression
    // nodes ultimately call this to fetch source data.
    float32x4_t eval_neon(size_t i) const { return vld1q_f32(rawData() + i); }
#endif

    // ── Reshaping and views ──
    Tensor reshape(std::vector<int> newShape) const;
    Tensor view(std::vector<int> newShape) const;
    Tensor clone() const;

    // Contiguity checks (matches PyTorch semantics)
    bool isContiguous() const;
    Tensor contiguous() const;

    // ── Raw data access ──
    float* rawData();
    const float* rawData() const;

    // In-place operations (evaluate expressions)
    template <TensorExpression Expr>
    Tensor& operator+=(const Expr& expr);

    template <TensorExpression Expr>
    Tensor& operator-=(const Expr& expr);

    // Fill the tensor with a constant value
    void fill(float value);

    // ── Autograd ──
    // All copies of a Tensor share the same AutogradMeta via shared_ptr.
    // This means setting grad on a copy (e.g., inside a GradFn node)
    // correctly updates the original leaf tensor.
    bool requiresGrad() const { return meta_ && meta_->requiresGrad; }
    void setRequiresGrad(bool val) { ensureMeta(); meta_->requiresGrad = val; }

    std::shared_ptr<Tensor> grad() const { return meta_ ? meta_->grad : nullptr; }
    void setGrad(std::shared_ptr<Tensor> g) { ensureMeta(); meta_->grad = g; }

    std::shared_ptr<GradFn> gradFn() const { return meta_ ? meta_->gradFn : nullptr; }
    void setGradFn(std::shared_ptr<GradFn> fn) { ensureMeta(); meta_->gradFn = fn; }

    // Access to raw meta pointer (for identity checks in backward)
    AutogradMeta* meta() const { return meta_.get(); }

    // Computes gradients traversing the computational graph backwards
    void backward();
    void backward(const Tensor& gradient);

    // Propagate gradients to leaf nodes (for Fused Autograd)
    void propagate_grad(size_t i, float grad) const;
#ifdef __ARM_NEON
    void propagate_grad_neon(size_t i, float32x4_t grad) const;
#endif

    // Collect leaves for fused autograd
    void collect_leaves(std::vector<Tensor*>& leaves) const;

    // Opt into lazy Expression Template dispatch
    LazyTensor lazy() const;

   protected:
    std::shared_ptr<Storage> storage_;
    std::vector<int> shape_;
    std::vector<int> strides_;
    int offset_{0};
    int size_{0};  // Cached — avoids recomputing via std::accumulate per operator call

    std::shared_ptr<AutogradMeta> meta_;  // Shared autograd identity

    // Lazily create AutogradMeta on first autograd use
    void ensureMeta() {
        if (!meta_) meta_ = std::make_shared<AutogradMeta>();
    }

    // Helper: compute strides from shape (row-major / C-contiguous)
    static std::vector<int> computeStrides(const std::vector<int>& shape);

    // Helper: compute total number of elements from shape
    static int computeSize(const std::vector<int>& shape);

    // Protected constructor for creating views (shared storage, custom offset/strides)
    Tensor(std::shared_ptr<Storage> storage, std::vector<int> shape, std::vector<int> strides, int offset);
};

// ── LazyTensor Definition ──
class LazyTensor : public TensorBase<LazyTensor> {
   public:
    explicit LazyTensor(Tensor tensor) : tensor_(std::move(tensor)) {}

    const std::vector<int>& shape() const { return tensor_.shape(); }
    float operator[](size_t i) const { return tensor_[i]; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const { return tensor_.eval_neon(i); }
    void propagate_grad_neon(size_t i, float32x4_t grad) const { tensor_.propagate_grad_neon(i, grad); }
#endif

    void propagate_grad(size_t i, float grad) const { tensor_.propagate_grad(i, grad); }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        tensor_.collect_leaves(leaves);
    }

    const Tensor& tensor() const { return tensor_; }

   private:
    Tensor tensor_;
};

inline LazyTensor Tensor::lazy() const {
    return LazyTensor(*this);
}

// Allow passing Tensor directly to free math functions
inline SinExpr<LazyTensor> sin(const Tensor& tensor) { return sin(tensor.lazy()); }
inline CosExpr<LazyTensor> cos(const Tensor& tensor) { return cos(tensor.lazy()); }
inline LogExpr<LazyTensor> log(const Tensor& tensor) { return log(tensor.lazy()); }
inline ExpExpr<LazyTensor> exp(const Tensor& tensor) { return exp(tensor.lazy()); }
inline SqrtExpr<LazyTensor> sqrt(const Tensor& tensor) { return sqrt(tensor.lazy()); }

// Free function: matrix multiplication  (dispatches to Backend)
Tensor matmul(const Tensor& A, const Tensor& B);

// ── Eager Operator Overloads (Autograd Dual-Dispatch) ──
// These non-template overloads take precedence over the Expression Template ones.
// When called, they will eagerly evaluate the operation and attach a GradFn if
// requiresGrad is true on either operand.
Tensor operator+(const Tensor& lhs, const Tensor& rhs);
Tensor operator-(const Tensor& lhs, const Tensor& rhs);
Tensor operator*(const Tensor& lhs, const Tensor& rhs);
Tensor operator/(const Tensor& lhs, const Tensor& rhs);

}  // namespace nn

// Implementation of template methods
#include "nn/Core/ThreadPool.hpp"
#include "nn/Autograd/ExprBackward.hpp"

namespace nn {

template <TensorExpression Expr>
Tensor::Tensor(const Expr& expr) : Tensor(expr.shape()) {
    *this = expr;
}

template <TensorExpression Expr>
Tensor& Tensor::operator=(const Expr& expr) {
    // Lazy evaluation: the expression tree is evaluated here in a single
    // fused pass. With NEON, each iteration processes 4 floats — the
    // entire expression tree composes NEON intrinsics (no intermediates).
    float* dst = rawData();
    int totalSize = size();
    ThreadPool::instance().parallelFor(0, totalSize, [dst, &expr](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        for (; i + 3 < end; i += 4) {
            vst1q_f32(&dst[i], expr.eval_neon(i));
        }
#endif
        for (; i < end; ++i) {
            dst[i] = expr[i];
        }
    });

    // Check if any leaf in the expression tree requires gradients.
    // If so, we build a fused autograd node for the entire expression tree.
    std::vector<Tensor*> leaves;
    expr.collect_leaves(leaves);
    bool anyRequiresGrad = false;
    for (Tensor* leaf : leaves) {
        if (leaf->requiresGrad()) {
            anyRequiresGrad = true;
            break;
        }
    }

    if (anyRequiresGrad && GradMode::is_enabled()) {
        setRequiresGrad(true);
        setGradFn(std::make_shared<ExprBackward<Expr>>(expr));
    } else {
        if (meta_) {
            meta_->requiresGrad = false;
            meta_->gradFn = nullptr;
        }
    }

    return *this;
}

template <TensorExpression Expr>
Tensor& Tensor::operator+=(const Expr& expr) {
    float* dst = rawData();
    int totalSize = size();
    ThreadPool::instance().parallelFor(0, totalSize, [dst, &expr](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        for (; i + 3 < end; i += 4) {
            float32x4_t existing = vld1q_f32(&dst[i]);
            vst1q_f32(&dst[i], vaddq_f32(existing, expr.eval_neon(i)));
        }
#endif
        for (; i < end; ++i) {
            dst[i] += expr[i];
        }
    });
    return *this;
}

template <TensorExpression Expr>
Tensor& Tensor::operator-=(const Expr& expr) {
    float* dst = rawData();
    int totalSize = size();
    ThreadPool::instance().parallelFor(0, totalSize, [dst, &expr](int start, int end) {
        int i = start;
#ifdef __ARM_NEON
        for (; i + 3 < end; i += 4) {
            float32x4_t existing = vld1q_f32(&dst[i]);
            vst1q_f32(&dst[i], vsubq_f32(existing, expr.eval_neon(i)));
        }
#endif
        for (; i < end; ++i) {
            dst[i] -= expr[i];
        }
    });
    return *this;
}

}  // namespace nn
