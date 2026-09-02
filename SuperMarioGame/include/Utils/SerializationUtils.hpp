#pragma once

#include <string>
#include "Utils/TileMap.hpp"
#include "Entities/Pipe.hpp"

class Entity;

#include "Entities/EntityFactory.hpp"

namespace SerializationUtils {
    std::string getTileTypeName(TileType type);
    TileType parseTileTypeName(const std::string& name);
    std::string getEntityTypeName(const Entity& entity);
    EntityType parseEntityTypeName(const std::string& name);

    // The level schema's optional pipe `entry` field. Named here rather than in
    // LevelLoader because the loader and the saver both need the mapping, and
    // this batch already had to fix three fields the loader read and the saver
    // silently dropped — a second private copy of the spelling is how a fourth
    // one starts.
    //
    // An unrecognised name parses back to Top, which is also the default: every
    // level written before this field existed keeps working unchanged.
    std::string getPipeEntryModeName(Pipe::EntryMode mode);
    Pipe::EntryMode parsePipeEntryModeName(const std::string& name);
}

