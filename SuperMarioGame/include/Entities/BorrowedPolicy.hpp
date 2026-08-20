#pragma once

#include "Entities/IAIPolicy.hpp"

// Lets an AIController use a policy it does not own.
//
// AIController::setPolicy takes a unique_ptr, which is right for normal play —
// the controller owns its brain for its whole life. Training breaks that
// assumption: the network outlives any single episode because it is the thing
// being trained, while the controller is handed a different driver each episode
// as DAgger anneals control from teacher to learner. Handing over ownership
// would mean either destroying the network between episodes or repeatedly
// moving it in and out of the controller.
//
// So this forwards to a policy someone else owns. The referent must outlive the
// controller holding this.
class BorrowedPolicy : public IAIPolicy {
public:
    explicit BorrowedPolicy(IAIPolicy& target) : m_target(&target) {}

    AIAction decide(const AIObservation& observation) override {
        return m_target->decide(observation);
    }
    const char* name() const override { return m_target->name(); }
    void reset() override { m_target->reset(); }

private:
    IAIPolicy* m_target;
};
