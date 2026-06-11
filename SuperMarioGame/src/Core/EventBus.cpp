#include "Core/EventBus.hpp"

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

EventBus::SubscriptionId EventBus::subscribe(EventType type, Callback callback) {
    // TODO: Implement by hand
    return 0;
}

void EventBus::unsubscribe(SubscriptionId id) {
    // TODO: Implement by hand
}

void EventBus::publish(const GameEvent& event) {
    // TODO: Implement by hand
}
