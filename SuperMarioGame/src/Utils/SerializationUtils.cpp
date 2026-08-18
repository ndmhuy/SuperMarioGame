#include "Utils/SerializationUtils.hpp"
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
    if (name == "coin_tile" || name == "coin") return TileType::Coin;              // "coin" = legacy alias
    if (name == "used") return TileType::Used;
    return TileType::Empty;
}

std::string getEntityTypeName(const Entity& entity) {
    if (dynamic_cast<const Mario*>(&entity)) return "mario";
    if (dynamic_cast<const Luigi*>(&entity)) return "luigi";
    if (dynamic_cast<const Toad*>(&entity)) return "toad";
    if (dynamic_cast<const Peach*>(&entity)) return "peach";

    if (dynamic_cast<const Goomba*>(&entity)) return "goomba";
    if (dynamic_cast<const KoopaTroopa*>(&entity)) return "koopa_troopa";
    if (dynamic_cast<const KoopaParatroopa*>(&entity)) return "koopa_paratroopa";
    if (dynamic_cast<const PiranhaPlant*>(&entity)) return "piranha_plant";
    if (dynamic_cast<const HammerBro*>(&entity)) return "hammer_bro";
    if (dynamic_cast<const Thwomp*>(&entity)) return "thwomp";
    if (dynamic_cast<const Boo*>(&entity)) return "boo";
    if (dynamic_cast<const Lakitu*>(&entity)) return "lakitu";
    if (dynamic_cast<const Spiny*>(&entity)) return "spiny";

    if (dynamic_cast<const Mushroom*>(&entity)) return "mushroom";
    if (dynamic_cast<const OneUpMushroom*>(&entity)) return "oneup_mushroom";
    if (dynamic_cast<const MiniMushroom*>(&entity)) return "mini_mushroom";
    if (dynamic_cast<const MegaMushroom*>(&entity)) return "mega_mushroom";
    if (dynamic_cast<const CapeFeather*>(&entity)) return "cape_feather";
    if (dynamic_cast<const FireFlower*>(&entity)) return "fire_flower";
    if (dynamic_cast<const Star*>(&entity)) return "star";
    if (dynamic_cast<const Coin*>(&entity)) return "coin";
    if (dynamic_cast<const StarCoin*>(&entity)) return "star_coin";
    if (dynamic_cast<const PSwitch*>(&entity)) return "pswitch";
    if (dynamic_cast<const POWBlock*>(&entity)) return "pow_block";
    if (dynamic_cast<const Trampoline*>(&entity)) return "trampoline";

    if (dynamic_cast<const Pipe*>(&entity)) return "pipe";
    if (dynamic_cast<const Flagpole*>(&entity)) return "flagpole";
    if (dynamic_cast<const QuestionBlock*>(&entity)) return "question_block";
    if (dynamic_cast<const BrickBlock*>(&entity)) return "brick_block";
    if (dynamic_cast<const MovingPlatform*>(&entity)) return "moving_platform";
    if (dynamic_cast<const FallingPlatform*>(&entity)) return "falling_platform";

    return "unknown";
}

EntityType parseEntityTypeName(const std::string& name) {
    if (name == "mario") return EntityType::Mario;
    if (name == "luigi") return EntityType::Luigi;
    if (name == "toad") return EntityType::Toad;
    if (name == "peach") return EntityType::Peach;

    if (name == "goomba") return EntityType::Goomba;
    if (name == "koopa_troopa") return EntityType::KoopaTroopa;
    if (name == "koopa_paratroopa") return EntityType::KoopaParatroopa;
    if (name == "piranha_plant") return EntityType::PiranhaPlant;
    if (name == "hammer_bro") return EntityType::HammerBro;
    if (name == "thwomp") return EntityType::Thwomp;
    if (name == "boo") return EntityType::Boo;
    if (name == "lakitu") return EntityType::Lakitu;
    if (name == "spiny") return EntityType::Spiny;

    if (name == "mushroom") return EntityType::Mushroom;
    if (name == "fire_flower") return EntityType::FireFlower;
    if (name == "coin") return EntityType::Coin;
    if (name == "star") return EntityType::Star;
    if (name == "oneup_mushroom" || name == "oneup") return EntityType::OneUpMushroom;
    if (name == "cape_feather") return EntityType::CapeFeather;
    if (name == "mega_mushroom") return EntityType::MegaMushroom;
    if (name == "mini_mushroom") return EntityType::MiniMushroom;
    if (name == "pow_block") return EntityType::POWBlock;
    if (name == "pswitch") return EntityType::PSwitch;
    if (name == "trampoline") return EntityType::Trampoline;
    if (name == "star_coin") return EntityType::StarCoin;

    if (name == "pipe") return EntityType::Pipe;
    if (name == "flagpole") return EntityType::Flagpole;
    if (name == "question_block") return EntityType::QuestionBlock;
    if (name == "brick_block") return EntityType::BrickBlock;
    if (name == "moving_platform") return EntityType::MovingPlatform;
    if (name == "falling_platform") return EntityType::FallingPlatform;

    return EntityType::Goomba;
}

} // namespace SerializationUtils
