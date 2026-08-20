#include "Entities/PolicyLosses.hpp"

#include "nn/Loss/Loss.hpp"
#include "nn/Tensor/Tensor.hpp"

#include <algorithm>
#include <cmath>

namespace {

// Bernoulli log-likelihood with a per-element weight.
//
//   L    = -(1/N) Σ w_i [ a_i log p_i + (1 - a_i) log(1 - p_i) ]
//   dL/dp_i = -(1/N) w_i [ a_i / p_i - (1 - a_i) / (1 - p_i) ]
//
// Cross-entropy rather than MSE, deliberately. The targets are Bernoulli, so
// this is the proper scoring rule; more practically, MSE's gradient vanishes
// exactly where the network is most wrong — a sigmoid saturated at 0.02 when
// the answer is 1 produces a tiny MSE gradient and a large cross-entropy one.
// For an action pressed on 3% of frames, that difference decides whether it is
// ever learned at all.
class WeightedBernoulli : public nn::Loss {
public:
    void setWeights(const std::vector<float>& weights) { m_weights = weights; }

    nn::Tensor compute(const nn::Tensor& p, const nn::Tensor& a) override {
        const int n = p.size();
        float total = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float pi = clampProbability(p.flat(i));
            const float ai = a.flat(i);
            total -= weightFor(i) * (ai * std::log(pi) + (1.0f - ai) * std::log(1.0f - pi));
        }
        nn::Tensor out({1});
        out.flat(0) = n > 0 ? total / static_cast<float>(n) : 0.0f;
        return out;
    }

    nn::Tensor gradient(const nn::Tensor& p, const nn::Tensor& a) override {
        const int n = p.size();
        nn::Tensor grad(p.shape());
        const float scale = n > 0 ? 1.0f / static_cast<float>(n) : 0.0f;
        for (int i = 0; i < n; ++i) {
            const float pi = clampProbability(p.flat(i));
            const float ai = a.flat(i);
            grad.flat(i) = -scale * weightFor(i) *
                           (ai / pi - (1.0f - ai) / (1.0f - pi));
        }
        return grad;
    }

private:
    // Guards log(0) and division by zero. 1e-6 keeps the largest possible
    // gradient magnitude near 1e6 * weight, which SGD at lr 0.01 tolerates.
    static float clampProbability(float p) { return std::clamp(p, 1e-6f, 1.0f - 1e-6f); }

    float weightFor(int index) const {
        if (m_weights.empty()) return 1.0f;
        const std::size_t i = static_cast<std::size_t>(index);
        return i < m_weights.size() ? m_weights[i] : m_weights.back();
    }

    std::vector<float> m_weights;
};

} // namespace

struct WeightedBernoulliLoss::Impl {
    WeightedBernoulli loss;
};

void WeightedBernoulliLoss::ImplDeleter::operator()(Impl* impl) const { delete impl; }

WeightedBernoulliLoss::WeightedBernoulliLoss() : m_impl(new Impl()) {}
WeightedBernoulliLoss::~WeightedBernoulliLoss() = default;

void WeightedBernoulliLoss::setWeights(const std::vector<float>& weights) {
    m_impl->loss.setWeights(weights);
}

void* WeightedBernoulliLoss::handle() const {
    return static_cast<void*>(&m_impl->loss);
}
