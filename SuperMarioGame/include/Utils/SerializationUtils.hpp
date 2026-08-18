#pragma once

#include <string>
#include "Utils/TileMap.hpp"

class Entity;

#include "Entities/EntityFactory.hpp"

namespace SerializationUtils {
    std::string getTileTypeName(TileType type);
    TileType parseTileTypeName(const std::string& name);
    std::string getEntityTypeName(const Entity& entity);
    EntityType parseEntityTypeName(const std::string& name);
}

