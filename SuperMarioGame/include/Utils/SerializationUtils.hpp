#pragma once

#include <string>
#include "Utils/TileMap.hpp"

class Entity;

namespace SerializationUtils {
    std::string getTileTypeName(TileType type);
    TileType parseTileTypeName(const std::string& name);
    std::string getEntityTypeName(const Entity& entity);
}
