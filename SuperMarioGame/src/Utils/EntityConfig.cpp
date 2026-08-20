#include "Utils/EntityConfig.hpp"
#include "Core/ResourceManager.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>

namespace {

// The strategy names the factory actually wires. Recorded in the file for
// documentation and validated here, so a typo is reported rather than ignored —
// an unread config is how this file drifted from the code in the first place.
const std::set<std::string> kKnownStrategies = {
    "patrol", "chase", "fly", "linear", "hammerthrow",
    "tetheredchase", "timeremergence", "proximitytrigger", "boss"
};

std::unique_ptr<std::unordered_map<std::string, EntityConfigEntry>> g_entries;

void loadEntries() {
    g_entries = std::make_unique<std::unordered_map<std::string, EntityConfigEntry>>();

    const std::string path = ResourceManager::resolvePath("assets/config/entities.json");
    if (!std::filesystem::exists(path)) {
        std::cerr << "[EntityConfig] No entities.json at " << path
                  << "; entities keep their built-in values." << std::endl;
        return;
    }

    try {
        std::ifstream file(path);
        nlohmann::json j;
        file >> j;

        for (auto it = j.begin(); it != j.end(); ++it) {
            // Keys beginning with '_' are notes to whoever edits the file.
            if (it.key().rfind('_', 0) == 0) continue;
            if (!it.value().is_object()) continue;

            EntityConfigEntry entry;
            entry.speed = it.value().value("speed", -1.0f);
            entry.score = it.value().value("score", -1);
            entry.strategy = it.value().value("strategy", std::string());

            if (!entry.strategy.empty() && kKnownStrategies.count(entry.strategy) == 0) {
                std::cerr << "[EntityConfig] \"" << it.key() << "\" names an unknown strategy \""
                          << entry.strategy << "\"" << std::endl;
            }
            (*g_entries)[it.key()] = entry;
        }
        std::cout << "[EntityConfig] Loaded " << g_entries->size() << " entity entries from "
                  << path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[EntityConfig] Could not parse entities.json: " << e.what()
                  << "; entities keep their built-in values." << std::endl;
        g_entries->clear();
    }
}

} // namespace

const std::unordered_map<std::string, EntityConfigEntry>& EntityConfig::entries() {
    if (!g_entries) loadEntries();
    return *g_entries;
}

const EntityConfigEntry* EntityConfig::find(const std::string& typeName) {
    const auto& table = entries();
    auto it = table.find(typeName);
    return (it == table.end()) ? nullptr : &it->second;
}

std::size_t EntityConfig::entryCount() {
    return entries().size();
}

void EntityConfig::reload() {
    g_entries.reset();
}
