#include "nn/Tensor/Matrix.hpp"
#include "nn/Tensor/Vector.hpp"
#include "nn/Compute/CPUBackend.hpp"
#include <algorithm>
#include <stdexcept>

namespace nn {

// ── Constructors ──

Matrix::Matrix(int rows, int cols) : Tensor({rows, cols}) {}

Matrix::Matrix(std::vector<float> data, int rows, int cols)
    : Tensor(std::move(data), {rows, cols}) {}

// ── Accessors ──

int Matrix::rows() const { return shape()[0]; }
int Matrix::cols() const { return shape()[1]; }

// ── Transpose ──
// In the C# project, transpose() physically copied data.
// Here we swap strides for a zero-copy view (Flyweight).
// The transposed matrix shares the same Storage — no data is moved.

Matrix Matrix::transpose() const {
    // Create a view with swapped shape and swapped strides
    Matrix result(cols(), rows());
    // Manually set up as a view into this matrix's storage
    result.storage_ = storage_;
    result.shape_ = {cols(), rows()};
    result.strides_ = {strides()[1], strides()[0]};
    result.offset_ = offset_;
    result.size_ = size_;  // Same total elements — just reordered access
    return result;
}

// ── Determinant ──
// Recursive cofactor expansion (for educational purposes, not performance).
// Only works for square matrices.

float Matrix::determinant() const {
    if (rows() != cols()) {
        throw std::invalid_argument("determinant: matrix must be square");
    }
    int n = rows();
    if (n == 1) return (*this)({0, 0});
    if (n == 2) {
        return (*this)({0, 0}) * (*this)({1, 1}) -
               (*this)({0, 1}) * (*this)({1, 0});
    }

    float det = 0.0f;
    for (int j = 0; j < n; ++j) {
        // Build (n-1)×(n-1) minor matrix
        Matrix minor(n - 1, n - 1);
        for (int mi = 1; mi < n; ++mi) {
            int mj = 0;
            for (int mj_src = 0; mj_src < n; ++mj_src) {
                if (mj_src == j) continue;
                minor({mi - 1, mj}) = (*this)({mi, mj_src});
                ++mj;
            }
        }
        float sign = (j % 2 == 0) ? 1.0f : -1.0f;
        det += sign * (*this)({0, j}) * minor.determinant();
    }
    return det;
}

// ── Row / Column views ──
// These create Vector objects that share Storage with this Matrix.
// rowView(i): contiguous row → stride = 1, offset = i * cols
// colView(j): non-contiguous column → stride = cols, offset = j

Vector Matrix::rowView(int i) const {
    Vector v(cols());
    v.storage_ = storage_;
    v.shape_ = {cols()};
    v.strides_ = {strides()[1]};  // inner stride (usually 1)
    v.offset_ = offset_ + i * strides()[0];
    v.size_ = cols();
    return v;
}

Vector Matrix::colView(int j) const {
    Vector v(rows());
    v.storage_ = storage_;
    v.shape_ = {rows()};
    v.strides_ = {strides()[0]};  // outer stride (usually cols)
    v.offset_ = offset_ + j * strides()[1];
    v.size_ = rows();
    return v;
}

// ── Identity factory ──

Matrix Matrix::identity(int size) {
    Matrix m(size, size);
    for (int i = 0; i < size; ++i) {
        m({i, i}) = 1.0f;
    }
    return m;
}

// ── fromTensor ──

Matrix Matrix::fromTensor(const Tensor& t) {
    if (t.rank() != 2) {
        throw std::invalid_argument("Matrix::fromTensor: tensor must be 2D");
    }
    Matrix m(t.shape()[0], t.shape()[1]);
    std::copy(t.rawData(), t.rawData() + t.size(), m.rawData());
    return m;
}

} // namespace nn
