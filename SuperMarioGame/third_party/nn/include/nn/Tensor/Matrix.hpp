#pragma once

#include "nn/Tensor/Tensor.hpp"

namespace nn {

class Vector;  // Forward declaration

class Matrix : public Tensor {
    friend class Vector;  // Vector may need Matrix internals for views
public:
    // Construct a zero-filled matrix of size rows x cols
    Matrix(int rows, int cols);

    // Construct a matrix from existing data
    Matrix(std::vector<float> data, int rows, int cols);

    // Matrix-specific accessors
    int rows() const;
    int cols() const;

    // Transpose: returns a new matrix with swapped strides (zero-copy view)
    Matrix transpose() const;

    // Determinant (only for square matrices, recursive expansion)
    float determinant() const;

    // View helpers: return Vector views into rows/columns
    // These share Storage with this Matrix (Flyweight)
    Vector rowView(int i) const;
    Vector colView(int j) const;

    // Factory: create an identity matrix of the given size
    static Matrix identity(int size);

    // Create a Matrix from a Tensor (must be 2D)
    static Matrix fromTensor(const Tensor& t);
};

} // namespace nn
