#include "Core/TimeRewindManager.hpp"

TimeRewindManager::TimeRewindManager(std::size_t maxCapacity)
    : m_maxCapacity(maxCapacity), m_isRewinding(false) {}

void TimeRewindManager::recordSnapshot(const GameSnapshot& snapshot) {
    m_snapshots.push_back(snapshot);
    if (m_snapshots.size() > m_maxCapacity) {
        m_snapshots.pop_front();
    }
}

bool TimeRewindManager::hasSnapshots() const {
    return !m_snapshots.empty();
}

GameSnapshot TimeRewindManager::popSnapshot() {
    if (m_snapshots.empty()) {
        return GameSnapshot{};
    }
    GameSnapshot last = m_snapshots.back();
    m_snapshots.pop_back();
    return last;
}

void TimeRewindManager::clear() {
    m_snapshots.clear();
    m_isRewinding = false;
}
