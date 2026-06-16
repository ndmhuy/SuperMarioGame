#include "Core/EventBus.hpp"
#include <algorithm>

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

EventBus::SubscriptionId EventBus::subscribe(EventType type, Callback callback) {
    SubscriptionId id = m_nextId++;
    m_subscribers[type].push_back(Subscription{id, callback});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    for (auto& [type, subs] : m_subscribers) {
        auto it = std::remove_if(subs.begin(), subs.end(),
            [id](const Subscription& sub) { return sub.id == id; });
        if (it != subs.end()) {
            subs.erase(it, subs.end());
            return;
        }
    }
}

void EventBus::publish(const GameEvent& event) {
    auto it = m_subscribers.find(event.type);
    if (it != m_subscribers.end()) {
        // Copy callbacks to prevent modification of the vector (e.g., unsubscribing) during iteration
        auto subs = it->second;
        for (const auto& sub : subs) {
            sub.callback(event);
        }
    }
}
