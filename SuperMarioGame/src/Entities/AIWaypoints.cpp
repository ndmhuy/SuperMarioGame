#include "Entities/AIWaypoints.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

std::vector<sf::Vector2f> loadAIWaypoints(const std::string& levelPath) {
    // assets/levels/level_1.json -> assets/levels/level_1.waypoints.json
    std::string sidecar = levelPath;
    const std::string suffix = ".json";
    if (sidecar.size() < suffix.size() ||
        sidecar.compare(sidecar.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return {};
    }
    sidecar.replace(sidecar.size() - suffix.size(), suffix.size(), ".waypoints.json");

    std::ifstream in(sidecar);
    if (!in) return {};

    try {
        const nlohmann::json doc = nlohmann::json::parse(in);
        // Version is a contract, exactly as it is for weight files: a layout
        // change must not be silently reinterpreted as pixel coordinates.
        if (doc.value("version", 0) != 1) return {};
        if (!doc.value("winnable", false)) return {};

        std::vector<sf::Vector2f> waypoints;
        for (const auto& node : doc.at("waypoints")) {
            waypoints.emplace_back(node.at("x").get<float>(),
                                   node.at("y").get<float>());
        }
        return waypoints;
    } catch (const nlohmann::json::exception&) {
        return {};
    }
}
