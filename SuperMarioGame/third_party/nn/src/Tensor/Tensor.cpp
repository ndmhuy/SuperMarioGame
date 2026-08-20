#include "nn/Tensor/Tensor.hpp"
#include "nn/Core/Device.hpp"
#include "nn/Autograd/AutogradMeta.hpp"
#include "nn/Autograd/ArithmeticOps.hpp"
#include "nn/Autograd/MatMulBackward.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <span>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace nn {

// ── Static helpers ──

std::vector<int> Tensor::computeStrides(const std::vector<int>& shape) {
    // Row-major (C-contiguous) strides.
    // For shape {2, 3, 4}: strides = {12, 4, 1}
    // stride[i] = product of shape[i+1] * shape[i+2] * ... * shape[N-1]
    std::vector<int> strides(shape.size());
    if (shape.empty()) return strides;

    strides.back() = 1;
    for (int i = static_cast<int>(shape.size()) - 2; i >= 0; --i) {
        strides[i] = strides[i + 1] * shape[i + 1];
    }
    return strides;
}

int Tensor::computeSize(const std::vector<int>& shape) {
    if (shape.empty()) return 0;
    return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
}

// ── Constructors ──

Tensor::Tensor(std::vector<int> shape) : shape_(std::move(shape)), strides_(computeStrides(shape_)), offset_(0) {
    size_ = computeSize(shape_);
    storage_ = std::make_shared<Storage>(size_);
}

Tensor::Tensor(std::vector<float> data, std::vector<int> shape)
    : shape_(std::move(shape)), strides_(computeStrides(shape_)), offset_(0) {
    size_ = computeSize(shape_);
    if (static_cast<int>(data.size()) != size_) {
        throw std::invalid_argument("Data size does not match shape");
    }
    storage_ = std::make_shared<Storage>(size_);
    // Copy data into aligned storage
    std::copy(data.begin(), data.end(), storage_->data());
}

// Protected view constructor
// Note: members initialize in declaration order (storage_, shape_, strides_,
// offset_, size_) so shape_ is valid when size_ computes from it.
Tensor::Tensor(std::shared_ptr<Storage> storage, std::vector<int> shape, std::vector<int> strides, int offset)
    : storage_(std::move(storage)),
      shape_(std::move(shape)),
      strides_(std::move(strides)),
      offset_(offset),
      size_(computeSize(shape_)) {}

// ── Copy semantics (view / shared storage) ──
// Copies share both Storage (Flyweight) AND AutogradMeta (shared identity).
// This is critical: when a GradFn stores a copy of a tensor, setting grad
// on that copy must update the original tensor's grad too.

Tensor::Tensor(const Tensor& other)
    : storage_(other.storage_),
      shape_(other.shape_),
      strides_(other.strides_),
      offset_(other.offset_),
      size_(other.size_),
      meta_(other.meta_) {
}

Tensor& Tensor::operator=(const Tensor& other) {
    if (this != &other) {
        storage_ = other.storage_;
        shape_ = other.shape_;
        strides_ = other.strides_;
        offset_ = other.offset_;
        size_ = other.size_;
        meta_ = other.meta_;
    }
    return *this;
}

// ── Accessors ──

int Tensor::rank() const { return static_cast<int>(shape_.size()); }

int Tensor::size() const {
    return size_;  // O(1) — cached at construction, not recomputed per call
}

const std::vector<int>& Tensor::shape() const { return shape_; }

const std::vector<int>& Tensor::strides() const { return strides_; }

int Tensor::offset() const { return offset_; }

// ── N-dim indexing ──
// Maps multi-dimensional indices to flat storage index using strides.
// index = offset + sum(indices[i] * strides[i])

float& Tensor::operator()(std::initializer_list<int> indices) {
    auto it = indices.begin();
    int flatIdx = offset_;
    for (int dim = 0; dim < rank(); ++dim) {
        flatIdx += (*it) * strides_[dim];
        ++it;
    }
    return storage_->data()[flatIdx];
}

float Tensor::operator()(std::initializer_list<int> indices) const {
    auto it = indices.begin();
    int flatIdx = offset_;
    for (int dim = 0; dim < rank(); ++dim) {
        flatIdx += (*it) * strides_[dim];
        ++it;
    }
    return storage_->data()[flatIdx];
}

// ── Flat 1D access ──

float& Tensor::flat(int i) { return storage_->data()[offset_ + i]; }

float Tensor::flat(int i) const { return storage_->data()[offset_ + i]; }

// ── Raw data access ──

float* Tensor::rawData() { return storage_->data() + offset_; }

const float* Tensor::rawData() const { return storage_->data() + offset_; }

// ── Reshaping and views ──
//
// PyTorch semantics:
//   view()    — REQUIRES contiguous memory. Guarantees zero-copy (shared storage).
//               Throws if the tensor is non-contiguous.
//   reshape() — Works on BOTH contiguous and non-contiguous tensors.
//               Returns a view if possible, copies data if not.
//
// This is intentionally different from the C# Perceptron which had no
// contiguity concept — all operations copied data.

Tensor Tensor::reshape(std::vector<int> newShape) const {
    int newSize = computeSize(newShape);
    if (newSize != size_) {
        throw std::invalid_argument("reshape: total size must match");
    }
    if (isContiguous()) {
        std::vector<int> newStrides = computeStrides(newShape);
        return Tensor(storage_, std::move(newShape), std::move(newStrides), offset_);
    }
    // Non-contiguous: copy to contiguous layout, then reshape the copy.
    // clone() always produces contiguous output, so the recursive call
    // hits the fast path above.
    return clone().reshape(std::move(newShape));
}

Tensor Tensor::view(std::vector<int> newShape) const {
    // view() guarantees zero-copy — it will NOT silently allocate.
    // This matches PyTorch: torch.Tensor.view() requires contiguous input.
    int newSize = computeSize(newShape);
    if (newSize != size_) {
        throw std::invalid_argument("view: total size must match");
    }
    if (!isContiguous()) {
        throw std::runtime_error(
            "view: tensor must be contiguous. "
            "Call .contiguous() first, or use .reshape() which handles this automatically.");
    }
    std::vector<int> newStrides = computeStrides(newShape);
    return Tensor(storage_, std::move(newShape), std::move(newStrides), offset_);
}

Tensor Tensor::clone() const {
    // Deep copy: allocates new Storage in contiguous (C-order) layout.
    // Correctly handles non-contiguous sources (e.g., transposed views)
    // by iterating in logical row-major order rather than physical memory order.
    Tensor result(shape_);

    if (isContiguous()) {
        // Fast path: contiguous memory — bulk copy via std::copy
        std::copy(rawData(), rawData() + size_, result.rawData());
    } else {
        // Slow path: iterate in logical (row-major) order for strided views.
        // This is necessary for transposed matrices where physical layout
        // doesn't match logical element order.
        const int r = rank();
        float* dst = result.rawData();
        std::vector<int> indices(r, 0);
        for (int i = 0; i < size_; ++i) {
            // Compute physical index from logical indices
            int srcIdx = offset_;
            for (int d = 0; d < r; ++d) {
                srcIdx += indices[d] * strides_[d];
            }
            dst[i] = storage_->data()[srcIdx];

            // Increment N-dim index in row-major (C) order: rightmost dimension first
            for (int d = r - 1; d >= 0; --d) {
                if (++indices[d] < shape_[d]) break;
                indices[d] = 0;
            }
        }
    }
    return result;
}

// ── Contiguity checks ──

bool Tensor::isContiguous() const {
    // A tensor is contiguous if its strides match the default row-major strides
    // for its shape. This is equivalent to PyTorch's is_contiguous().
    // Non-contiguous examples: transposed matrices, column views.
    return strides_ == computeStrides(shape_);
}

Tensor Tensor::contiguous() const {
    // Returns self (shared storage) if already contiguous.
    // Otherwise, deep-copies into contiguous layout.
    // Matches PyTorch's tensor.contiguous() semantics.
    if (isContiguous()) return *this;
    return clone();
}

// ── Fill ──

void Tensor::fill(float value) { std::fill(rawData(), rawData() + size_, value); }

// ── Free functions ──

Tensor matmul(const Tensor& A, const Tensor& B) {
    // A must be [M x K], B must be [K x N]
    if (A.rank() != 2 || B.rank() != 2) {
        throw std::invalid_argument("matmul: both tensors must be 2D");
    }
    int M = A.shape()[0];
    int K = A.shape()[1];
    int K2 = B.shape()[0];
    int N = B.shape()[1];
    if (K != K2) {
        throw std::invalid_argument("matmul: inner dimensions must match");
    }
    Tensor C({M, N});
    Device::activeBackend()->matmul(std::span<const float>(A.rawData(), M * K),
                                    std::span<const float>(B.rawData(), K * N), std::span<float>(C.rawData(), M * N), M,
                                    K, N);
    
    if ((A.requiresGrad() || B.requiresGrad()) && GradMode::is_enabled()) {
        C.setRequiresGrad(true);
        C.setGradFn(std::make_shared<MatMulBackward>(A, B));
    }
    
    return C;
}

// ── Autograd ──

void Tensor::backward() {
    Tensor grad(shape_);
    grad.fill(1.0f);
    backward(grad);
}

void Tensor::backward(const Tensor& gradient) {
    if (!requiresGrad()) return;

    Tensor grad = gradient;
    if (grad.size() == 0) {
        grad = Tensor(shape_);
        grad.fill(1.0f);
    }

    // Accumulate gradient into this tensor's shared meta
    if (!meta_->grad) {
        meta_->grad = std::make_shared<Tensor>(grad);
    } else {
        *meta_->grad += grad;
    }

    auto rootGradFn = gradFn();
    if (!rootGradFn) return;

    // Topological sort via DFS on GradFn nodes
    std::vector<GradFn*> topo;
    std::unordered_set<GradFn*> visited;

    std::function<void(GradFn*)> build_topo = [&](GradFn* v) {
        if (!v) return;
        if (visited.find(v) == visited.end()) {
            visited.insert(v);
            for (const Tensor& child : v->inputs()) {
                if (child.gradFn()) {
                    build_topo(child.gradFn().get());
                }
            }
            topo.push_back(v);
        }
    };

    build_topo(rootGradFn.get());
    std::reverse(topo.begin(), topo.end());

    // Map GradFn* -> accumulated gradient flowing into that node
    std::unordered_map<GradFn*, Tensor> node_gradients;
    node_gradients[rootGradFn.get()] = grad;

    for (GradFn* node : topo) {
        Tensor gradOutput = node_gradients[node];
        std::vector<Tensor> input_grads = node->backward(gradOutput);
        std::vector<Tensor> inputs = node->inputs();
        bool isExpr = (node->name() == "ExprBackward");

        for (size_t i = 0; i < inputs.size(); ++i) {
            Tensor& input = inputs[i];
            if (input.requiresGrad()) {
                // Accumulate gradient into the shared AutogradMeta if not already done by ExprBackward.
                if (!isExpr) {
                    if (!input.grad()) {
                        input.setGrad(std::make_shared<Tensor>(input_grads[i]));
                    } else {
                        *input.grad() += input_grads[i];
                    }
                }

                // Propagate to child GradFn nodes
                if (input.gradFn()) {
                    GradFn* child_fn = input.gradFn().get();
                    if (node_gradients.find(child_fn) == node_gradients.end()) {
                        node_gradients[child_fn] = input_grads[i];
                    } else {
                        node_gradients[child_fn] += input_grads[i];
                    }
                }
            }
        }
    }
}

// ── Eager Operator Overloads (Autograd Dual-Dispatch) ──

Tensor operator+(const Tensor& lhs, const Tensor& rhs) {
    if (lhs.shape() != rhs.shape()) throw std::invalid_argument("Shapes must match for addition");
    Tensor out(lhs.shape());
    Device::activeBackend()->add(
        std::span<const float>(lhs.rawData(), lhs.size()),
        std::span<const float>(rhs.rawData(), rhs.size()),
        std::span<float>(out.rawData(), out.size())
    );
    if ((lhs.requiresGrad() || rhs.requiresGrad()) && GradMode::is_enabled()) {
        out.setRequiresGrad(true);
        out.setGradFn(std::make_shared<AddBackward>(lhs, rhs));
    }
    return out; 
}

Tensor operator-(const Tensor& lhs, const Tensor& rhs) {
    if (lhs.shape() != rhs.shape()) throw std::invalid_argument("Shapes must match for subtraction");
    Tensor out(lhs.shape());
    Device::activeBackend()->sub(
        std::span<const float>(lhs.rawData(), lhs.size()),
        std::span<const float>(rhs.rawData(), rhs.size()),
        std::span<float>(out.rawData(), out.size())
    );
    if ((lhs.requiresGrad() || rhs.requiresGrad()) && GradMode::is_enabled()) {
        out.setRequiresGrad(true);
        out.setGradFn(std::make_shared<SubBackward>(lhs, rhs));
    }
    return out;
}

Tensor operator*(const Tensor& lhs, const Tensor& rhs) {
    if (lhs.shape() != rhs.shape()) throw std::invalid_argument("Shapes must match for multiplication");
    Tensor out(lhs.shape());
    Device::activeBackend()->mul(
        std::span<const float>(lhs.rawData(), lhs.size()),
        std::span<const float>(rhs.rawData(), rhs.size()),
        std::span<float>(out.rawData(), out.size())
    );
    if ((lhs.requiresGrad() || rhs.requiresGrad()) && GradMode::is_enabled()) {
        out.setRequiresGrad(true);
        out.setGradFn(std::make_shared<MulBackward>(lhs, rhs));
    }
    return out;
}

Tensor operator/(const Tensor& lhs, const Tensor& rhs) {
    if (lhs.shape() != rhs.shape()) throw std::invalid_argument("Shapes must match for division");
    Tensor out(lhs.shape());
    Device::activeBackend()->div(
        std::span<const float>(lhs.rawData(), lhs.size()),
        std::span<const float>(rhs.rawData(), rhs.size()),
        std::span<float>(out.rawData(), out.size())
    );
    if ((lhs.requiresGrad() || rhs.requiresGrad()) && GradMode::is_enabled()) {
        out.setRequiresGrad(true);
        out.setGradFn(std::make_shared<DivBackward>(lhs, rhs));
    }
    return out;
}

void Tensor::propagate_grad(size_t i, float grad) const {
    if (meta_ && meta_->requiresGrad) {
        if (meta_->grad) {
            meta_->grad->flat(static_cast<int>(i)) += grad;
        }
    }
}

#ifdef __ARM_NEON
void Tensor::propagate_grad_neon(size_t i, float32x4_t grad) const {
    if (meta_ && meta_->requiresGrad) {
        if (meta_->grad) {
            float* g = meta_->grad->rawData();
            float32x4_t existing = vld1q_f32(&g[i]);
            vst1q_f32(&g[i], vaddq_f32(existing, grad));
        }
    }
}
#endif

void Tensor::collect_leaves(std::vector<Tensor*>& leaves) const {
    leaves.push_back(const_cast<Tensor*>(this));
}

}  // namespace nn
