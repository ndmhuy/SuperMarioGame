#include "Utils/SerializationUtils.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
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

namespace SerializationUtils {

std::string getTileTypeName(TileType type) {
    switch (type) {
        case TileType::Ground: return "ground";
        case TileType::Brick: return "brick";
        case TileType::Question: return "question_block";
        case TileType::Pipe: return "pipe";
        case TileType::Ice: return "ice";
        case TileType::Conveyor: return "conveyor";
        case TileType::Water: return "water";
        case TileType::Coin: return "coin_tile";
        default: return "empty";
    }
}

TileType parseTileTypeName(const std::string& name) {
    if (name == "ground") return TileType::Ground;
    if (name == "brick") return TileType::Brick;
    if (name == "question_block") return TileType::Question;
    if (name == "pipe") return TileType::Pipe;
    if (name == "ice") return TileType::Ice;
    if (name == "conveyor") return TileType::Conveyor;
    if (name == "water") return TileType::Water;
    if (name == "coin_tile" || name == "coin") return TileType::Coin;
    return TileType::Empty;
}

std::string getEntityTypeName(const Entity& entity) {
    if (dynamic_cast<const Mario*>(&entity)) return "mario";
    if (dynamic_cast<const Luigi*>(&entity)) return "luigi";
    if (dynamic_cast<const Toad*>(&entity)) return "toad";
    if (dynamic_cast<const Peach*>(&entity)) return "peach";
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
    return "unknown";
}

} // namespace SerializationUtils
