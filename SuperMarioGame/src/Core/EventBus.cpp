#include "Core/EventBus.hpp"
#include <algorithm>
#include <utility>

namespace {
// Set once, in ~EventBus(). AchievementManager, StatisticsTracker and Camera
// each hold long-lived ScopedSubscriptions in a Meyer's-singleton-owned (or,
// for Camera, program-lifetime) container; at program exit, function-local
// statics destruct in reverse order of first construction, which is
// determined by which code path calls EventBus::getInstance() first — not
// something callers control or should have to reason about. If EventBus
// happens to destruct before one of those containers does, each
// ScopedSubscription's destructor would call EventBus::getInstance() again
// AFTER it was already destroyed — a dead-reference access, not merely a
// leak. This is a compile-time-constant-initialized global (zero/constant
// init happens before any dynamic or function-local static construction),
// so it is safe to read from any other object's destructor regardless of
// destruction order.
bool g_eventBusAlive = true;
}

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

EventBus::~EventBus() {
    g_eventBusAlive = false;
}

EventBus::SubscriptionId EventBus::subscribe(EventType type, Callback callback) {
    SubscriptionId id = m_nextId++;
    m_subscribers[type].push_back(Subscription{id, std::move(callback), true});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    for (auto& [type, subs] : m_subscribers) {
        for (Subscription& sub : subs) {
            if (sub.id != id || !sub.alive) continue;

            // Tombstone rather than erase. A handler is allowed to unsubscribe
            // itself, or anything else, while an event is being delivered;
            // erasing here would invalidate the vector publish() is walking.
            sub.alive = false;
            sub.callback = nullptr;   // release whatever the lambda captured
            m_needsCompaction = true;
            if (m_publishDepth == 0) compact();
            return;
        }
    }
}

void EventBus::publish(const GameEvent& event) {
    auto it = m_subscribers.find(event.type);
    if (it == m_subscribers.end()) return;

    ++m_publishDepth;

    // Iterate by index over the live vector rather than over a copy.
    //
    // This used to copy the whole subscriber vector on every publish, which
    // copies a std::function per subscriber — a heap allocation each, on an
    // event bus that fires on every coin, stomp, block and jump (audit X-7). The
    // copy was there to survive a handler unsubscribing mid-delivery; the
    // tombstone in unsubscribe() handles that without copying anything.
    //
    // The size is taken once, so a handler that subscribes during delivery does
    // not receive the event it is currently handling — which would otherwise be
    // an easy accidental infinite loop.
    std::vector<Subscription>& subs = it->second;
    const std::size_t count = subs.size();
    for (std::size_t i = 0; i < count && i < subs.size(); ++i) {
        // Re-checked each step: a handler may append, which can reallocate.
        if (!subs[i].alive || !subs[i].callback) continue;
        subs[i].callback(event);
    }

    --m_publishDepth;
    if (m_publishDepth == 0 && m_needsCompaction) compact();
}

void EventBus::compact() {
    m_needsCompaction = false;
    for (auto& [type, subs] : m_subscribers) {
        subs.erase(std::remove_if(subs.begin(), subs.end(),
                                  [](const Subscription& sub) { return !sub.alive; }),
                   subs.end());
    }
}

// --- ScopedSubscription ------------------------------------------------------

EventBus::ScopedSubscription::ScopedSubscription(EventType type, Callback callback)
    : m_id(EventBus::getInstance().subscribe(type, std::move(callback))), m_active(true) {}

EventBus::ScopedSubscription::~ScopedSubscription() {
    reset();
}

EventBus::ScopedSubscription::ScopedSubscription(ScopedSubscription&& other) noexcept
    : m_id(other.m_id), m_active(other.m_active) {
    other.m_active = false;
}

EventBus::ScopedSubscription& EventBus::ScopedSubscription::operator=(
    ScopedSubscription&& other) noexcept {
    if (this != &other) {
        reset();
        m_id = other.m_id;
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

void EventBus::ScopedSubscription::reset() {
    if (!m_active) return;
    m_active = false;
    // Guards against static destruction order: see g_eventBusAlive above.
    if (g_eventBusAlive) EventBus::getInstance().unsubscribe(m_id);
}
