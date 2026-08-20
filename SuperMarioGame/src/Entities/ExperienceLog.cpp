#include "Entities/ExperienceLog.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>

ExperienceLog::~ExperienceLog() { close(); }

bool ExperienceLog::open(const std::string& path) {
    close();

    std::error_code ignored;
    const std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::filesystem::create_directories(fsPath.parent_path(), ignored);
    }

    // Append: several runs make one dataset, and truncating on open would mean
    // the last run silently replaced everything collected before it.
    m_file.open(path, std::ios::app);
    if (!m_file.is_open()) {
        std::cerr << "[ExperienceLog] Cannot open '" << path << "' for writing."
                  << std::endl;
        return false;
    }

    // Header row per session, so a reader can tell where one run ends and the
    // next begins, and can refuse a feature layout it was not trained for.
    nlohmann::json header;
    header["type"] = "header";
    header["observationVersion"] = kAIObservationVersion;
    header["featureCount"] = AIObservation::featureCount();
    header["visionWidth"] = kAIVisionWidth;
    header["visionHeight"] = kAIVisionHeight;
    header["cellStates"] = kAICellStateCount;
    m_file << header.dump() << '\n';
    m_file.flush();

    std::cout << "[ExperienceLog] Recording transitions to " << path << " ("
              << AIObservation::featureCount() << " features per row)." << std::endl;
    return true;
}

void ExperienceLog::close() {
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
        std::cout << "[ExperienceLog] Wrote " << m_rows << " transitions." << std::endl;
    }
    m_rows = 0;
}

void ExperienceLog::record(const AIObservation& observation, const AIAction& action,
                           float reward, bool terminal) {
    if (!m_file.is_open()) return;

    nlohmann::json row;
    row["obs"] = observation.toFeatureVector();
    // Buttons in AIAction declaration order, which is also the network's output
    // order — so a trained head's outputs line up with these labels directly.
    row["act"] = {action.moveLeft, action.moveRight, action.jump, action.run,
                  action.crouch, action.shoot, action.groundPound};
    row["rew"] = reward;
    row["done"] = terminal;

    m_file << row.dump() << '\n';
    ++m_rows;

    // Flushed per row rather than buffered: a run that ends in the crash you
    // were trying to capture must not lose the rows leading up to it.
    m_file.flush();
}
