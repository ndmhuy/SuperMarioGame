#pragma once

#include "Core/GameSnapshot.hpp"
#include <cstddef>
#include <string>
#include <vector>

// Task 10.3 — recording and replaying a run.
//
// This extends the Memento the game already has rather than adding a second
// mechanism: GameSnapshot is exactly what TimeRewindManager stores, and
// PlayingState already builds one every frame for rewind. A replay is that same
// snapshot stream, kept for longer, thinned out, and written to disk.
//
// Snapshots rather than inputs
// ----------------------------
// The usual way to record a replay is to store the player's inputs and re-run
// the simulation. That needs the simulation to be bit-for-bit deterministic, and
// this one is not: float physics, an entity list that spawns and prunes, and
// enemy strategies that read a shared Game singleton. Storing state is larger on
// disk but it plays back exactly what happened, which is the point.
//
// Snapshots are kept every Nth frame and interpolation is left out, so a replay
// is a faithful flip-book rather than a smooth re-simulation.
class ReplayRecorder {
public:
    static ReplayRecorder& getInstance();

    ReplayRecorder(const ReplayRecorder&) = delete;
    ReplayRecorder& operator=(const ReplayRecorder&) = delete;

    // --- Recording ---
    void startRecording(const std::string& levelName);
    void stopRecording();
    bool isRecording() const { return m_recording; }
    // Offered every frame; keeps one in every kFrameInterval.
    void record(const GameSnapshot& snapshot);

    // --- Playback ---
    // Returns false when there is nothing loaded to play.
    bool startPlayback();
    void stopPlayback();
    bool isPlaying() const { return m_playing; }
    // The next snapshot to apply, or null when the replay has run out.
    const GameSnapshot* advance();

    // --- Persistence ---
    bool save(const std::string& name) const;
    bool load(const std::string& name);
    static std::vector<std::string> list();

    std::size_t frameCount() const { return m_frames.size(); }
    const std::string& levelName() const { return m_levelName; }
    void clear();

    // One snapshot in every this-many frames. At 60fps that is 10 a second,
    // which is enough to watch and small enough that a three-minute level is a
    // few hundred kilobytes rather than several megabytes.
    static constexpr int kFrameInterval = 6;
    // Roughly ten minutes at that rate. A recorder with no cap is a memory leak
    // that only shows up on a long session.
    static constexpr std::size_t kMaxFrames = 6000;

private:
    ReplayRecorder() = default;
    ~ReplayRecorder() = default;

    static std::string pathFor(const std::string& name);

    std::vector<GameSnapshot> m_frames;
    std::string m_levelName;
    bool m_recording = false;
    bool m_playing = false;
    int m_frameSkip = 0;
    std::size_t m_playhead = 0;
};
