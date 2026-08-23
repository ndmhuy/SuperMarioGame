#include "Utils/SerializationUtils.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"

#include "Entities/Goomba.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/Boo.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Spiny.hpp"

#include "Entities/Mushroom.hpp"
#include "Entities/OneUpMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/Coin.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/Trampoline.hpp"

#include "Entities/Pipe.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/BrickBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"

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
    // The canonical names live in EntityCatalogue, which is also what the level
    // editor's palette is built from. This function used to carry its own
    // hand-written copy of the same 40-entry list; the two drifted, and the
    // editor's version was missing every enemy and every block.
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

} // namespace SerializationUtils
