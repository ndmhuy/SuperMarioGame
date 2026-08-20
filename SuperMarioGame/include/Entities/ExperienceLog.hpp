#pragma once

#include "Entities/IAIPolicy.hpp"

#include <fstream>
#include <string>

// Writes the transitions a trainer learns from: one JSON object per line.
//
// This is the other half of the training loop, and the half that has to live in
// the game. Everything a learner needs about a step is here — what the agent
// saw, what it did, what that earned — in a format anything can read a line at a
// time without loading the whole run into memory.
//
// It records whoever is playing. Pointed at a *human*, the same rows become
// imitation-learning data, which is only possible because AIAction was defined
// with the same seven buttons PlayerFramePacket records. That was a deliberate
// choice on the main branch, not a coincidence found later.
//
// One line per decision, not per frame: the agent acts at its difficulty's
// reaction cadence, and logging frames it did not act on would fill the file
// with duplicate rows.
class ExperienceLog {
public:
    ExperienceLog() = default;
    ~ExperienceLog();

    ExperienceLog(const ExperienceLog&) = delete;
    ExperienceLog& operator=(const ExperienceLog&) = delete;

    // Open `path` for append. Records the observation version in a header line
    // so a training run can refuse a file whose feature layout it does not
    // match, rather than quietly learning against misaligned columns.
    bool open(const std::string& path);
    void close();
    bool isOpen() const { return m_file.is_open(); }

    // One transition. `reward` is what accumulated since the previous call —
    // RewardTracker::consume() — and `terminal` marks the last step of an
    // episode, which is what stops a learner bootstrapping a value estimate
    // across a death.
    void record(const AIObservation& observation, const AIAction& action, float reward,
                bool terminal);

    std::size_t rowsWritten() const { return m_rows; }

private:
    std::ofstream m_file;
    std::size_t m_rows = 0;
};
