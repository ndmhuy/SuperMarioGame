#include "nn/Autograd/MatMulBackward.hpp"
#include "nn/Tensor/Matrix.hpp"
#include "nn/Tensor/Tensor.hpp"

namespace nn {

MatMulBackward::MatMulBackward(const Tensor& lhs, const Tensor& rhs) : lhs_(lhs), rhs_(rhs) {
}

std::vector<Tensor> MatMulBackward::backward(const Tensor& gradOutput) {
    // If Y = A * B, then:
    // dL/dA = dL/dY * B^T
    // dL/dB = A^T * dL/dY
    Tensor gradLhs = matmul(gradOutput, Matrix::fromTensor(rhs_).transpose());
    Tensor gradRhs = matmul(Matrix::fromTensor(lhs_).transpose(), gradOutput);
    return {gradLhs, gradRhs};
}

}  // namespace nn
