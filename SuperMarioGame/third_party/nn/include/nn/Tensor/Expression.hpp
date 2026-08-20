#pragma once

#include <vector>
#include <concepts>
#include <type_traits>
#include <cmath>

// ARM NEON SIMD: each expression node evaluates 4 floats at a time.
// The entire expression tree composes NEON intrinsics — no intermediate
// Tensor allocations AND no scalar-only evaluation bottleneck.
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace nn {

class Tensor;

// CRTP Base class for all Tensors and Expressions
template <typename Derived>
class TensorBase {
   public:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Concept to restrict operations to our Tensor expression tree
template <typename T>
concept TensorExpression = std::derived_from<T, TensorBase<T>>;

// ── AddExpr ──
template <TensorExpression LHS, TensorExpression RHS>
class AddExpr : public TensorBase<AddExpr<LHS, RHS>> {
   public:
    AddExpr(const LHS& lhs, const RHS& rhs) : lhs_(lhs), rhs_(rhs) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] + rhs_[i]; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        return vaddq_f32(lhs_.eval_neon(i), rhs_.eval_neon(i));
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        lhs_.propagate_grad_neon(i, grad);
        rhs_.propagate_grad_neon(i, grad);
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        lhs_.propagate_grad(i, grad);
        rhs_.propagate_grad(i, grad);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
        rhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    RHS rhs_;
};

template <TensorExpression LHS, TensorExpression RHS>
AddExpr<LHS, RHS> operator+(const LHS& lhs, const RHS& rhs) {
    return AddExpr<LHS, RHS>(lhs, rhs);
}

// ── SubExpr ──
template <TensorExpression LHS, TensorExpression RHS>
class SubExpr : public TensorBase<SubExpr<LHS, RHS>> {
   public:
    SubExpr(const LHS& lhs, const RHS& rhs) : lhs_(lhs), rhs_(rhs) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] - rhs_[i]; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        return vsubq_f32(lhs_.eval_neon(i), rhs_.eval_neon(i));
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        lhs_.propagate_grad_neon(i, grad);
        rhs_.propagate_grad_neon(i, vnegq_f32(grad));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        lhs_.propagate_grad(i, grad);
        rhs_.propagate_grad(i, -grad);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
        rhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    RHS rhs_;
};

template <TensorExpression LHS, TensorExpression RHS>
SubExpr<LHS, RHS> operator-(const LHS& lhs, const RHS& rhs) {
    return SubExpr<LHS, RHS>(lhs, rhs);
}

// ── MulExpr (element-wise Hadamard product) ──
template <TensorExpression LHS, TensorExpression RHS>
class MulExpr : public TensorBase<MulExpr<LHS, RHS>> {
   public:
    MulExpr(const LHS& lhs, const RHS& rhs) : lhs_(lhs), rhs_(rhs) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] * rhs_[i]; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        return vmulq_f32(lhs_.eval_neon(i), rhs_.eval_neon(i));
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        lhs_.propagate_grad_neon(i, vmulq_f32(grad, rhs_.eval_neon(i)));
        rhs_.propagate_grad_neon(i, vmulq_f32(grad, lhs_.eval_neon(i)));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        lhs_.propagate_grad(i, grad * rhs_[i]);
        rhs_.propagate_grad(i, grad * lhs_[i]);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
        rhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    RHS rhs_;
};

template <TensorExpression LHS, TensorExpression RHS>
MulExpr<LHS, RHS> operator*(const LHS& lhs, const RHS& rhs) {
    return MulExpr<LHS, RHS>(lhs, rhs);
}

// ── DivExpr ──
template <TensorExpression LHS, TensorExpression RHS>
class DivExpr : public TensorBase<DivExpr<LHS, RHS>> {
   public:
    DivExpr(const LHS& lhs, const RHS& rhs) : lhs_(lhs), rhs_(rhs) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] / rhs_[i]; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
#if defined(__aarch64__)
        return vdivq_f32(lhs_.eval_neon(i), rhs_.eval_neon(i));
#else
        float32x4_t b = rhs_.eval_neon(i);
        float32x4_t rec = vrecpeq_f32(b);
        rec = vmulq_f32(vrecpsq_f32(b, rec), rec);
        return vmulq_f32(lhs_.eval_neon(i), rec);
#endif
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
#if defined(__aarch64__)
        float32x4_t r = rhs_.eval_neon(i);
        lhs_.propagate_grad_neon(i, vdivq_f32(grad, r));
        float32x4_t l = lhs_.eval_neon(i);
        float32x4_t grad_r = vnegq_f32(vdivq_f32(vmulq_f32(grad, l), vmulq_f32(r, r)));
        rhs_.propagate_grad_neon(i, grad_r);
#else
        float32x4_t r = rhs_.eval_neon(i);
        float32x4_t rec = vrecpeq_f32(r);
        rec = vmulq_f32(vrecpsq_f32(r, rec), rec);
        lhs_.propagate_grad_neon(i, vmulq_f32(grad, rec));
        
        float32x4_t l = lhs_.eval_neon(i);
        float32x4_t rec2 = vmulq_f32(rec, rec);
        float32x4_t grad_r = vmulq_f32(vnegq_f32(vmulq_f32(grad, l)), rec2);
        rhs_.propagate_grad_neon(i, grad_r);
#endif
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        float r = rhs_[i];
        lhs_.propagate_grad(i, grad / r);
        rhs_.propagate_grad(i, -grad * lhs_[i] / (r * r));
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
        rhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    RHS rhs_;
};

template <TensorExpression LHS, TensorExpression RHS>
DivExpr<LHS, RHS> operator/(const LHS& lhs, const RHS& rhs) {
    return DivExpr<LHS, RHS>(lhs, rhs);
}

// ── ScalarMulExpr ──
template <TensorExpression LHS>
class ScalarMulExpr : public TensorBase<ScalarMulExpr<LHS>> {
   public:
    ScalarMulExpr(const LHS& lhs, float scalar) : lhs_(lhs), scalar_(scalar) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] * scalar_; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        return vmulq_f32(lhs_.eval_neon(i), vdupq_n_f32(scalar_));
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        lhs_.propagate_grad_neon(i, vmulq_f32(grad, vdupq_n_f32(scalar_)));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        lhs_.propagate_grad(i, grad * scalar_);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    float scalar_;
};

template <TensorExpression LHS>
ScalarMulExpr<LHS> operator*(const LHS& lhs, float scalar) {
    return ScalarMulExpr<LHS>(lhs, scalar);
}

template <TensorExpression RHS>
ScalarMulExpr<RHS> operator*(float scalar, const RHS& rhs) {
    return ScalarMulExpr<RHS>(rhs, scalar);
}

// ── ScalarAddExpr ──
template <TensorExpression LHS>
class ScalarAddExpr : public TensorBase<ScalarAddExpr<LHS>> {
   public:
    ScalarAddExpr(const LHS& lhs, float scalar) : lhs_(lhs), scalar_(scalar) {}

    const std::vector<int>& shape() const { return lhs_.shape(); }

    float operator[](size_t i) const { return lhs_[i] + scalar_; }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        return vaddq_f32(lhs_.eval_neon(i), vdupq_n_f32(scalar_));
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        lhs_.propagate_grad_neon(i, grad);
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        lhs_.propagate_grad(i, grad);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        lhs_.collect_leaves(leaves);
    }

   private:
    LHS lhs_;
    float scalar_;
};

template <TensorExpression LHS>
ScalarAddExpr<LHS> operator+(const LHS& lhs, float scalar) {
    return ScalarAddExpr<LHS>(lhs, scalar);
}

template <TensorExpression RHS>
ScalarAddExpr<RHS> operator+(float scalar, const RHS& rhs) {
    return ScalarAddExpr<RHS>(rhs, scalar);
}

// ── SinExpr ──
template <TensorExpression Expr>
class SinExpr : public TensorBase<SinExpr<Expr>> {
   public:
    SinExpr(const Expr& expr) : expr_(expr) {}

    const std::vector<int>& shape() const { return expr_.shape(); }

    float operator[](size_t i) const { return std::sin(expr_[i]); }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        output[0] = std::sin(input[0]);
        output[1] = std::sin(input[1]);
        output[2] = std::sin(input[2]);
        output[3] = std::sin(input[3]);
        return vld1q_f32(output);
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        float32x4_t val = expr_.eval_neon(i);
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, val);
        output[0] = std::cos(input[0]);
        output[1] = std::cos(input[1]);
        output[2] = std::cos(input[2]);
        output[3] = std::cos(input[3]);
        float32x4_t cos_val = vld1q_f32(output);
        expr_.propagate_grad_neon(i, vmulq_f32(grad, cos_val));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        expr_.propagate_grad(i, grad * std::cos(expr_[i]));
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        expr_.collect_leaves(leaves);
    }

   private:
    Expr expr_;
};

template <TensorExpression Expr>
SinExpr<Expr> sin(const Expr& expr) {
    return SinExpr<Expr>(expr);
}

// ── CosExpr ──
template <TensorExpression Expr>
class CosExpr : public TensorBase<CosExpr<Expr>> {
   public:
    CosExpr(const Expr& expr) : expr_(expr) {}

    const std::vector<int>& shape() const { return expr_.shape(); }

    float operator[](size_t i) const { return std::cos(expr_[i]); }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        output[0] = std::cos(input[0]);
        output[1] = std::cos(input[1]);
        output[2] = std::cos(input[2]);
        output[3] = std::cos(input[3]);
        return vld1q_f32(output);
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        float32x4_t val = expr_.eval_neon(i);
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, val);
        output[0] = std::sin(input[0]);
        output[1] = std::sin(input[1]);
        output[2] = std::sin(input[2]);
        output[3] = std::sin(input[3]);
        float32x4_t sin_val = vld1q_f32(output);
        expr_.propagate_grad_neon(i, vmulq_f32(grad, vnegq_f32(sin_val)));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        expr_.propagate_grad(i, -grad * std::sin(expr_[i]));
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        expr_.collect_leaves(leaves);
    }

   private:
    Expr expr_;
};

template <TensorExpression Expr>
CosExpr<Expr> cos(const Expr& expr) {
    return CosExpr<Expr>(expr);
}

// ── LogExpr ──
template <TensorExpression Expr>
class LogExpr : public TensorBase<LogExpr<Expr>> {
   public:
    LogExpr(const Expr& expr) : expr_(expr) {}

    const std::vector<int>& shape() const { return expr_.shape(); }

    float operator[](size_t i) const { return std::log(expr_[i]); }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        output[0] = std::log(input[0]);
        output[1] = std::log(input[1]);
        output[2] = std::log(input[2]);
        output[3] = std::log(input[3]);
        return vld1q_f32(output);
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
#if defined(__aarch64__)
        expr_.propagate_grad_neon(i, vdivq_f32(grad, expr_.eval_neon(i)));
#else
        float32x4_t val = expr_.eval_neon(i);
        float32x4_t rec = vrecpeq_f32(val);
        rec = vmulq_f32(vrecpsq_f32(val, rec), rec);
        expr_.propagate_grad_neon(i, vmulq_f32(grad, rec));
#endif
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        expr_.propagate_grad(i, grad / expr_[i]);
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        expr_.collect_leaves(leaves);
    }

   private:
    Expr expr_;
};

template <TensorExpression Expr>
LogExpr<Expr> log(const Expr& expr) {
    return LogExpr<Expr>(expr);
}

// ── ExpExpr ──
template <TensorExpression Expr>
class ExpExpr : public TensorBase<ExpExpr<Expr>> {
   public:
    ExpExpr(const Expr& expr) : expr_(expr) {}

    const std::vector<int>& shape() const { return expr_.shape(); }

    float operator[](size_t i) const { return std::exp(expr_[i]); }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        output[0] = std::exp(input[0]);
        output[1] = std::exp(input[1]);
        output[2] = std::exp(input[2]);
        output[3] = std::exp(input[3]);
        return vld1q_f32(output);
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
        float32x4_t val = this->eval_neon(i);
        expr_.propagate_grad_neon(i, vmulq_f32(grad, val));
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        expr_.propagate_grad(i, grad * std::exp(expr_[i]));
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        expr_.collect_leaves(leaves);
    }

   private:
    Expr expr_;
};

template <TensorExpression Expr>
ExpExpr<Expr> exp(const Expr& expr) {
    return ExpExpr<Expr>(expr);
}

// ── SqrtExpr ──
template <TensorExpression Expr>
class SqrtExpr : public TensorBase<SqrtExpr<Expr>> {
   public:
    SqrtExpr(const Expr& expr) : expr_(expr) {}

    const std::vector<int>& shape() const { return expr_.shape(); }

    float operator[](size_t i) const { return std::sqrt(expr_[i]); }

#ifdef __ARM_NEON
    float32x4_t eval_neon(size_t i) const {
#if defined(__aarch64__)
        return vsqrtq_f32(expr_.eval_neon(i));
#else
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        output[0] = std::sqrt(input[0]);
        output[1] = std::sqrt(input[1]);
        output[2] = std::sqrt(input[2]);
        output[3] = std::sqrt(input[3]);
        return vld1q_f32(output);
#endif
    }
    void propagate_grad_neon(size_t i, float32x4_t grad) const {
#if defined(__aarch64__)
        float32x4_t sqrt_val = vsqrtq_f32(expr_.eval_neon(i));
        float32x4_t denom = vmulq_f32(vdupq_n_f32(2.0f), sqrt_val);
        expr_.propagate_grad_neon(i, vdivq_f32(grad, denom));
#else
        alignas(16) float input[4];
        alignas(16) float output[4];
        vst1q_f32(input, expr_.eval_neon(i));
        float val0 = std::sqrt(input[0]);
        float val1 = std::sqrt(input[1]);
        float val2 = std::sqrt(input[2]);
        float val3 = std::sqrt(input[3]);
        output[0] = 1.0f / (2.0f * val0);
        output[1] = 1.0f / (2.0f * val1);
        output[2] = 1.0f / (2.0f * val2);
        output[3] = 1.0f / (2.0f * val3);
        float32x4_t grad_sqrt = vld1q_f32(output);
        expr_.propagate_grad_neon(i, vmulq_f32(grad, grad_sqrt));
#endif
    }
#endif

    void propagate_grad(size_t i, float grad) const {
        expr_.propagate_grad(i, grad / (2.0f * std::sqrt(expr_[i])));
    }

    void collect_leaves(std::vector<Tensor*>& leaves) const {
        expr_.collect_leaves(leaves);
    }

   private:
    Expr expr_;
};

template <TensorExpression Expr>
SqrtExpr<Expr> sqrt(const Expr& expr) {
    return SqrtExpr<Expr>(expr);
}

}  // namespace nn
