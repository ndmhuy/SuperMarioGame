#pragma once

#include "Core/GameSnapshot.hpp"
#include <deque>
#include <cstddef>

class TimeRewindManager {
public:
    explicit TimeRewindManager(std::size_t maxCapacity = 300); // 300 frames = 5 seconds at 60 FPS
    ~TimeRewindManager() = default;

    void recordSnapshot(const GameSnapshot& snapshot);
    bool hasSnapshots() const;
    GameSnapshot popSnapshot();
    void clear();

    bool isRewinding() const { return m_isRewinding; }
    void setRewinding(bool rewinding) { m_isRewinding = rewinding; }

    std::size_t getSnapshotCount() const { return m_snapshots.size(); }
    std::size_t getMaxCapacity() const { return m_maxCapacity; }

private:
    std::deque<GameSnapshot> m_snapshots;
    std::size_t m_maxCapacity;
    bool m_isRewinding = false;
};
