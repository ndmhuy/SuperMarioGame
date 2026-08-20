#include "nn/Tensor/Vector.hpp"
#include "nn/Compute/CPUBackend.hpp"
#include <algorithm>
#include <stdexcept>
#include <span>

namespace nn {

// ── Constructors ──

Vector::Vector(int dimension) : Tensor({dimension}) {}

Vector::Vector(std::vector<float> data) : Tensor({static_cast<int>(data.size())}) {
    // Copy data into aligned storage
    std::copy(data.begin(), data.end(), rawData());
}

// ── Accessors ──

int Vector::dimension() const { return shape()[0]; }

// ── Dot product ──
// In the C# project, this was a simple loop.
// Here it delegates to CPUBackend::mul + sum. Phase 2 replaces with NEON vmlaq.

float Vector::dot(const Vector& A, const Vector& B) {
    if (A.dimension() != B.dimension()) {
        throw std::invalid_argument("dot: vectors must have same dimension");
    }
    float result = 0.0f;
    int n = A.dimension();
    const float* a = A.rawData();
    const float* b = B.rawData();
    for (int i = 0; i < n; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

// ── Scale in-place ──

void Vector::scaleInPlace(float s) {
    CPUBackend::instance().scalarMul(std::span<const float>(rawData(), size()), s, std::span<float>(rawData(), size()));
}

// ── 1D element access ──

float& Vector::operator[](int i) {
    return storage_->data()[offset_ + i * strides()[0]];
}

float Vector::operator[](int i) const {
    return storage_->data()[offset_ + i * strides()[0]];
}

// ── fromTensor ──

Vector Vector::fromTensor(const Tensor& t) {
    if (t.rank() != 1) {
        throw std::invalid_argument("Vector::fromTensor: tensor must be 1D");
    }
    Vector v(t.shape()[0]);
    std::copy(t.rawData(), t.rawData() + t.size(), v.rawData());
    return v;
}

} // namespace nn
