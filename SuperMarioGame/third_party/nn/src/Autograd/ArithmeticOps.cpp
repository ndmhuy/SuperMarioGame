#include "nn/Autograd/ArithmeticOps.hpp"
#include "nn/Tensor/Expression.hpp"
#include <vector>

namespace nn {

// ── AddBackward ──

AddBackward::AddBackward(const Tensor& lhs, const Tensor& rhs) : lhs_(lhs), rhs_(rhs) {
}

std::vector<Tensor> AddBackward::backward(const Tensor& gradOutput) {
    // d(x + y)/dx = 1, d(x + y)/dy = 1
    // Chain rule: dL/dx = gradOutput * 1, dL/dy = gradOutput * 1
    return {gradOutput, gradOutput};
}

// ── SubBackward ──

SubBackward::SubBackward(const Tensor& lhs, const Tensor& rhs) : lhs_(lhs), rhs_(rhs) {
}

std::vector<Tensor> SubBackward::backward(const Tensor& gradOutput) {
    // d(x - y)/dx = 1, d(x - y)/dy = -1
    // Chain rule: dL/dx = gradOutput * 1, dL/dy = gradOutput * -1
    return {gradOutput, gradOutput * -1.0f};
}

// ── MulBackward ──

MulBackward::MulBackward(const Tensor& lhs, const Tensor& rhs) : lhs_(lhs), rhs_(rhs) {
}

std::vector<Tensor> MulBackward::backward(const Tensor& gradOutput) {
    // d(x * y)/dx = y, d(x * y)/dy = x
    // Chain rule: dL/dx = gradOutput * y, dL/dy = gradOutput * x
    return {gradOutput * rhs_, gradOutput * lhs_};
}

// ── DivBackward ──

DivBackward::DivBackward(const Tensor& lhs, const Tensor& rhs) : lhs_(lhs), rhs_(rhs) {
}

std::vector<Tensor> DivBackward::backward(const Tensor& gradOutput) {
    // d(x / y)/dx = 1 / y
    // d(x / y)/dy = -x / y^2
    // Chain rule: 
    // dL/dx = gradOutput / y
    // dL/dy = gradOutput * (-x / (y * y))
    
    Tensor gradLhs = gradOutput / rhs_;
    Tensor gradRhs = gradOutput * (lhs_ * -1.0f) / (rhs_ * rhs_);
    return {gradLhs, gradRhs};
}

}  // namespace nn
