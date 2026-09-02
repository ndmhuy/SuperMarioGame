#include "Utils/SerializationUtils.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/Entity.hpp"

// The thirty-one concrete entity headers that used to sit here are gone with
// the thirty dynamic_casts that needed them: an entity reports its own name now
// (Entity::getTypeName), and the reverse direction resolves through the
// EntityCatalogue registry.

namespace SerializationUtils {

// Canonical tile name <-> TileType mapping.
//
// These two functions are the ONLY place tile type strings are produced or
// consumed. Do not re-implement either direction elsewhere: a duplicated
// if-chain in LevelLoader once drifted from this table and silently dropped
// every coin tile in all seven level files (audit A-1).
//
// getTileTypeName emits the canonical name; parseTileTypeName additionally
// accepts historical aliases so older level JSON keeps loading.

std::string getTileTypeName(TileType type) {
    switch (type) {
        case TileType::Ground:   return "ground";
        case TileType::Brick:    return "brick";
        case TileType::Question: return "question_block";
        case TileType::Pipe:     return "pipe";
        case TileType::Ice:      return "ice";
        case TileType::Conveyor: return "conveyor";
        case TileType::Water:    return "water";
        case TileType::Lava:     return "lava";
        case TileType::Coin:     return "coin_tile";
        case TileType::Used:     return "used";
        case TileType::Empty:    return "empty";
    }
    return "empty";
}

TileType parseTileTypeName(const std::string& name) {
    if (name == "ground") return TileType::Ground;
    if (name == "brick") return TileType::Brick;
    if (name == "question_block" || name == "question") return TileType::Question; // "question" = legacy alias
    if (name == "pipe") return TileType::Pipe;
    if (name == "ice") return TileType::Ice;
    if (name == "conveyor") return TileType::Conveyor;
    if (name == "water") return TileType::Water;
    if (name == "lava") return TileType::Lava;
    if (name == "coin_tile" || name == "coin") return TileType::Coin;              // "coin" = legacy alias
    if (name == "used") return TileType::Used;
    return TileType::Empty;
}

std::string getEntityTypeName(const Entity& entity) {
    // Was 30 sequential dynamic_casts. Each entity now reports its own name, so
    // adding a type no longer means editing this function — and derived types
    // are no longer shadowed by their base (see Entity::getTypeName).
    return entity.getTypeName();
}

EntityType parseEntityTypeName(const std::string& name) {
    // The canonical names live in EntityCatalogue — the same registry the
    // palette is built from and the same one EntityFactory constructs through,
    // so a name that parses here is by construction a type that can be built.
    // This function used to carry its own hand-written copy of the same 40-entry
    // list; the two drifted, and the editor's version was missing every enemy
    // and every block.
    if (const auto* entry = EntityCatalogue::findByName(name)) {
        return entry->type;
    }

    // Historical aliases. These are a compatibility concern, not a fact about
    // the game's types, so they stay here rather than in the catalogue — older
    // level files must keep loading, but nothing should WRITE these names.
    if (name == "oneup")     return EntityType::OneUpMushroom;
    if (name == "boomboom")  return EntityType::BoomBoom;
    if (name == "axe")       return EntityType::BridgeAxe;

    // Unknown names still fall back to Goomba rather than throwing, because
    // level files are hand-edited — but every type the game can *write* now
    // parses back to itself, which is what verify_regressions asserts.
    return EntityType::Goomba;
}

std::string getPipeEntryModeName(Pipe::EntryMode mode) {
    switch (mode) {
        case Pipe::EntryMode::SideLeft:  return "side_left";
        case Pipe::EntryMode::SideRight: return "side_right";
        case Pipe::EntryMode::Top:
        default:                         return "top";
    }
}

Pipe::EntryMode parsePipeEntryModeName(const std::string& name) {
    if (name == "side_left")  return Pipe::EntryMode::SideLeft;
    if (name == "side_right") return Pipe::EntryMode::SideRight;
    return Pipe::EntryMode::Top;
}

} // namespace SerializationUtils
