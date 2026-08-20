#pragma once

namespace nn {

// GradMode — global state controller for autograd tracking.
class GradMode {
public:
    static bool is_enabled() { return enabled_; }
    static void set_enabled(bool enabled) { enabled_ = enabled; }

private:
    static inline bool enabled_ = true;
};

// NoGrad — RAII context manager to temporarily disable gradient computation.
//
// Usage:
// {
//     nn::NoGrad guard;
//     Tensor y = model.forward(x);  // No gradients computed or GradFn attached
// }
class NoGrad {
public:
    NoGrad() : prev_(GradMode::is_enabled()) {
        GradMode::set_enabled(false);
    }
    ~NoGrad() {
        GradMode::set_enabled(prev_);
    }

    // Prevent copying
    NoGrad(const NoGrad&) = delete;
    NoGrad& operator=(const NoGrad&) = delete;

private:
    bool prev_;
};

} // namespace nn
