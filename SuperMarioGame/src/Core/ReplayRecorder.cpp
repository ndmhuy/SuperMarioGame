#include "Core/ReplayRecorder.hpp"
#include "Utils/Serializer.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

ReplayRecorder& ReplayRecorder::getInstance() {
    static ReplayRecorder instance;
    return instance;
}

std::string ReplayRecorder::pathFor(const std::string& name) {
    return Serializer::saveDirectory() + "/replays/" + name + ".json";
}

void ReplayRecorder::clear() {
    m_frames.clear();
    m_levelName.clear();
    m_recording = false;
    m_playing = false;
    m_frameSkip = 0;
    m_playhead = 0;
}

void ReplayRecorder::startRecording(const std::string& levelName) {
    // Recording and playing at once would append the replay to itself.
    m_playing = false;
    m_frames.clear();
    m_playhead = 0;
    m_frameSkip = 0;
    m_levelName = levelName;
    m_recording = true;
    std::cout << "[Replay] Recording " << levelName << std::endl;
}

void ReplayRecorder::stopRecording() {
    if (!m_recording) return;
    m_recording = false;
    std::cout << "[Replay] Stopped at " << m_frames.size() << " frames" << std::endl;
}

void ReplayRecorder::record(const GameSnapshot& snapshot) {
    if (!m_recording) return;

    if (m_frameSkip++ % kFrameInterval != 0) return;
    if (m_frames.size() >= kMaxFrames) {
        // Stop rather than wrap: a ring buffer would silently turn a long run
        // into a replay of its last ten minutes, which is not what was asked for.
        stopRecording();
        std::cerr << "[Replay] Hit the frame cap; recording stopped." << std::endl;
        return;
    }
    m_frames.push_back(snapshot);
}

bool ReplayRecorder::startPlayback() {
    if (m_frames.empty()) return false;
    m_recording = false;
    m_playing = true;
    m_playhead = 0;
    return true;
}

void ReplayRecorder::stopPlayback() {
    m_playing = false;
    m_playhead = 0;
}

const GameSnapshot* ReplayRecorder::advance() {
    if (!m_playing) return nullptr;
    if (m_playhead >= m_frames.size()) {
        m_playing = false;
        return nullptr;
    }
    return &m_frames[m_playhead++];
}

bool ReplayRecorder::save(const std::string& name) const {
    if (m_frames.empty()) return false;

    try {
        const std::string path = pathFor(name);
        std::filesystem::path fsPath(path);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["version"] = "1.0";
        j["level"] = m_levelName;
        j["frameInterval"] = kFrameInterval;
        j["frames"] = nlohmann::json::array();

        for (const GameSnapshot& frame : m_frames) {
            nlohmann::json f;
            f["t"] = frame.levelTimer;
            f["cam"] = {frame.cameraCenter.x, frame.cameraCenter.y};
            f["p"] = {frame.playerState.position.x, frame.playerState.position.y,
                      frame.playerState.velocity.x, frame.playerState.velocity.y,
                      frame.playerState.score, frame.playerState.coins,
                      frame.playerState.lives, frame.playerState.onGround ? 1 : 0};

            // Entities are written as flat arrays rather than objects: a replay
            // is thousands of these, and the key names would be most of the file.
            nlohmann::json entities = nlohmann::json::array();
            for (const EntitySnapshot& e : frame.entityStates) {
                entities.push_back({e.id, e.position.x, e.position.y,
                                    e.velocity.x, e.velocity.y, e.active ? 1 : 0});
            }
            f["e"] = std::move(entities);
            j["frames"].push_back(std::move(f));
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump();   // no indent: replays are machine-read and large
        std::cout << "[Replay] Saved " << m_frames.size() << " frames to " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Replay] Save failed: " << e.what() << std::endl;
        return false;
    }
}

bool ReplayRecorder::load(const std::string& name) {
    try {
        const std::string path = pathFor(name);
        if (!std::filesystem::exists(path)) return false;

        std::ifstream file(path);
        if (!file.is_open()) return false;

        nlohmann::json j;
        file >> j;

        clear();
        m_levelName = j.value("level", std::string());
        if (!j.contains("frames") || !j["frames"].is_array()) return false;

        for (const auto& f : j["frames"]) {
            GameSnapshot frame;
            frame.levelTimer = f.value("t", 0.0f);
            if (f.contains("cam") && f["cam"].size() >= 2) {
                frame.cameraCenter = {f["cam"][0].get<float>(), f["cam"][1].get<float>()};
            }
            if (f.contains("p") && f["p"].size() >= 8) {
                const auto& p = f["p"];
                frame.playerState.position = {p[0].get<float>(), p[1].get<float>()};
                frame.playerState.velocity = {p[2].get<float>(), p[3].get<float>()};
                frame.playerState.score = p[4].get<int>();
                frame.playerState.coins = p[5].get<int>();
                frame.playerState.lives = p[6].get<int>();
                frame.playerState.onGround = p[7].get<int>() != 0;
            }
            if (f.contains("e")) {
                for (const auto& e : f["e"]) {
                    if (e.size() < 6) continue;
                    EntitySnapshot snapshot;
                    snapshot.id = e[0].get<std::uint32_t>();
                    snapshot.position = {e[1].get<float>(), e[2].get<float>()};
                    snapshot.velocity = {e[3].get<float>(), e[4].get<float>()};
                    snapshot.active = e[5].get<int>() != 0;
                    frame.entityStates.push_back(snapshot);
                }
            }
            m_frames.push_back(std::move(frame));
        }
        std::cout << "[Replay] Loaded " << m_frames.size() << " frames from " << path << std::endl;
        return !m_frames.empty();
    } catch (const std::exception& e) {
        std::cerr << "[Replay] Load failed: " << e.what() << std::endl;
        clear();
        return false;
    }
}

std::vector<std::string> ReplayRecorder::list() {
    std::vector<std::string> names;
    try {
        const std::string dir = Serializer::saveDirectory() + "/replays";
        if (!std::filesystem::is_directory(dir)) return names;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".json") {
                names.push_back(entry.path().stem().string());
            }
        }
    } catch (const std::exception&) {
        // A missing or unreadable replay directory is not an error worth
        // reporting: it just means nothing has been recorded yet.
    }
    return names;
}
