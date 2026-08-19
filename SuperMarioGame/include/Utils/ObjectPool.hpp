#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

// Task 10.1 — a pool of pre-allocated, recycled objects.
//
// Why the interface looks like this
// --------------------------------
// The obvious pool hands out raw pointers and owns everything itself. That does
// not fit this game: PlayingState owns the world as
// std::vector<std::unique_ptr<Entity>> and prunes inactive entries every frame,
// so a pool that kept ownership would mean rewriting how every entity is stored.
//
// So this pool trades in std::unique_ptr<T> instead. acquire() hands one over,
// release() takes it back, and in between the object is owned exactly the way an
// unpooled one would be — the entity list does not know or care that it came
// from a pool. The only change at the call site is that the prune step offers
// spent objects back instead of dropping them.
//
// T must provide `resetForPool(Args...)` matching whatever acquire() is called
// with: a recycled object has to be put back into its just-constructed state,
// and only T knows what that means.
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t reserve = 0) {
        m_free.reserve(reserve);
    }

    // Recycle a free object if there is one, and construct otherwise. The
    // arguments are forwarded to T's constructor on a miss, and to
    // T::resetForPool on a hit, so both paths leave the caller with the same
    // object it asked for.
    template <typename... Args>
    std::unique_ptr<T> acquire(Args&&... args) {
        if (!m_free.empty()) {
            std::unique_ptr<T> object = std::move(m_free.back());
            m_free.pop_back();
            ++m_recycled;
            object->resetForPool(std::forward<Args>(args)...);
            return object;
        }
        ++m_constructed;
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Offer a spent object back. Null is ignored, and a pool that has reached
    // its cap simply lets the object die rather than growing without bound —
    // a leak that never frees is worse than an allocation.
    void release(std::unique_ptr<T> object) {
        if (!object) return;
        if (m_maxRetained > 0 && m_free.size() >= m_maxRetained) return;
        m_free.push_back(std::move(object));
    }

    // Objects held ready for reuse right now.
    std::size_t freeCount() const { return m_free.size(); }
    // How many were ever built. The point of the pool is that this stops rising
    // once the working set is reached, and it is what the tests assert on.
    std::size_t constructedCount() const { return m_constructed; }
    // How many acquisitions were served from the free list.
    std::size_t recycledCount() const { return m_recycled; }

    void setMaxRetained(std::size_t maxRetained) { m_maxRetained = maxRetained; }
    void clear() { m_free.clear(); }

private:
    std::vector<std::unique_ptr<T>> m_free;
    std::size_t m_constructed = 0;
    std::size_t m_recycled = 0;
    // 0 means unbounded. Set by owners that know their working set.
    std::size_t m_maxRetained = 16;
};
