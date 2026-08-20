#pragma once

#include "nn/Tensor/Tensor.hpp"

namespace nn {

class Vector : public Tensor {
    friend class Matrix;  // Matrix creates Vector views by setting protected members
public:
    // Construct a zero-filled vector of the given dimension
    explicit Vector(int dimension);

    // Construct a vector from existing data
    Vector(std::vector<float> data);

    // Dimension accessor (alias for shape()[0])
    int dimension() const;

    // Dot product of two vectors
    static float dot(const Vector& A, const Vector& B);

    // Scale all elements in-place
    void scaleInPlace(float s);

    // 1D element access via operator[]
    float& operator[](int i);
    float operator[](int i) const;

    // Create a Vector from a Tensor (must be 1D)
    static Vector fromTensor(const Tensor& t);
};

} // namespace nn
