#pragma once

#include <string>
#include <unordered_map>

// One entity's tunable values, as read from assets/config/entities.json.
//
// Negative / empty means "the file did not say", and the entity keeps whatever
// its constructor set. That matters: the file is optional, and a half-filled
// entry must not silently zero a value the code had.
struct EntityConfigEntry {
    float speed = -1.0f;
    int score = -1;
    std::string strategy;   // descriptive; validated on load, not applied
};

// Task 10.2 — config-driven entity tuning.
//
// assets/config/entities.json has shipped since early in the project and was
// read by nothing at all. It also covered three of thirteen enemies and
// disagreed with the code (goomba speed 60 against the code's 50), which is
// what happens to data nobody consumes.
//
// It is now the tuning source for every enemy, seeded from the values the
// constructors were already using so adopting it changed no behaviour. Balance
// edits are a data change from here on.
//
// Static functions over a value type, like Serializer and CampaignProgress:
// this is a file that gets read once, not a thing that needs holding open.
class EntityConfig {
public:
    // Entry for an entity's getTypeName(), or null when the file has nothing to
    // say about it. Loads the file on first use.
    static const EntityConfigEntry* find(const std::string& typeName);

    // How many entries were parsed. Zero means the file was missing or bad,
    // which is survivable — every entity still has its constructor's values.
    static std::size_t entryCount();

    // Drops the cache so the next lookup re-reads the file. For the dev panel
    // and the tests; the game itself loads once.
    static void reload();

private:
    static const std::unordered_map<std::string, EntityConfigEntry>& entries();
};
