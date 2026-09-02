#include "Entities/EntityCatalogue.hpp"

#include "Utils/Constants.hpp"

// The registry declares how to CONSTRUCT each type, so this translation unit is
// the one that needs every concrete entity header. EntityFactory used to carry
// this include list next to its 42-case switch; the switch is what moved here.
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"

#include "Entities/Goomba.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Boo.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/BulletBill.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/ChainChomp.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Spiny.hpp"
#include "Entities/Hammer.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/BossFireball.hpp"
#include "Entities/BoomBoom.hpp"

#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Star.hpp"
#include "Entities/OneUpMushroom.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/BridgeAxe.hpp"

#include "Entities/BrickBlock.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/IceBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"
#include "Entities/ConveyorBelt.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Castle.hpp"

#include <algorithm>
#include <unordered_map>

namespace EntityCatalogue {
namespace {

// The ordinary creator: "construct T at this position", which is what all but
// three of the entries below need. Written once as a template so a new type
// costs exactly one table row.
template <typename T>
std::unique_ptr<Entity> make(sf::Vector2f position) {
    return std::make_unique<T>(position);
}

} // namespace

const std::vector<Entry>& all() {
    using C = Category;
    // Names here are the canonical ones. verify_regressions checks each against
    // the class's own getTypeName(), so a mismatch is a failing test rather than
    // a level file that loads as a Goomba.
    //
    // This table is a function-local static ON PURPOSE, and must stay one.
    // Registration data at namespace scope has no defined initialisation order
    // across translation units, so anything that touched the registry during
    // another TU's static init would read a half-built or empty vector — a
    // missing palette entry or a crash that depends on link order. A
    // function-local static is constructed on first use, which is always after
    // whatever asked for it.
    //
    // For the same family of reasons the entries are declared HERE, explicitly,
    // rather than each entity class self-registering from its own .cpp. Both
    // designs give one list; only this one does not depend on how the build is
    // linked. Self-registration works today solely because CMakeLists.txt
    // builds the app sources as OBJECT libraries (AppObjectsRelease /
    // AppObjectsTest), which link every object file unconditionally. Were
    // either ever turned into a static archive, the linker would drop the
    // objects whose symbols nothing else references, and those entities would
    // vanish from the palette at runtime with no build error at all — a
    // stealthier version of the exact defect this table removes.
    static const std::vector<Entry> kEntries = {
        // --- Players ---------------------------------------------------------
        {EntityType::Mario,           "mario",            "Mario",             C::Player,     &make<Mario>},
        {EntityType::Luigi,           "luigi",            "Luigi",             C::Player,     &make<Luigi>},
        {EntityType::Toad,            "toad",             "Toad",              C::Player,     &make<Toad>},
        {EntityType::Peach,           "peach",            "Peach",             C::Player,     &make<Peach>},

        // --- Enemies ---------------------------------------------------------
        {EntityType::Goomba,          "goomba",           "Goomba",            C::Enemy,      &make<Goomba>},
        {EntityType::KoopaTroopa,     "koopa_troopa",     "Koopa Troopa",      C::Enemy,      &make<KoopaTroopa>},
        {EntityType::KoopaParatroopa, "koopa_paratroopa", "Koopa Paratroopa",  C::Enemy,      &make<KoopaParatroopa>},
        {EntityType::Spiny,           "spiny",            "Spiny",             C::Enemy,      &make<Spiny>},
        {EntityType::Boo,             "boo",              "Boo",               C::Enemy,      &make<Boo>},
        {EntityType::PiranhaPlant,    "piranha_plant",    "Piranha Plant",     C::Enemy,      &make<PiranhaPlant>},
        {EntityType::BulletBill,      "bullet_bill",      "Bullet Bill",       C::Enemy,      &make<BulletBill>},
        {EntityType::HammerBro,       "hammer_bro",       "Hammer Bro",        C::Enemy,      &make<HammerBro>},
        {EntityType::Thwomp,          "thwomp",           "Thwomp",            C::Enemy,      &make<Thwomp>},
        {EntityType::ChainChomp,      "chain_chomp",      "Chain Chomp",       C::Enemy,      &make<ChainChomp>},
        {EntityType::Lakitu,          "lakitu",           "Lakitu",            C::Enemy,      &make<Lakitu>},
        {EntityType::BoomBoom,        "boom_boom",        "Boss: Boom Boom",   C::Enemy,      &make<BoomBoom>},
        {EntityType::Bowser,          "bowser",           "Boss: Bowser",      C::Enemy,      &make<Bowser>},

        // --- Items -----------------------------------------------------------
        {EntityType::Coin,            "coin",             "Coin",              C::Item,       &make<Coin>},
        {EntityType::StarCoin,        "star_coin",        "Star Coin",         C::Item,       &make<StarCoin>},
        {EntityType::Mushroom,        "mushroom",         "Super Mushroom",    C::Item,       &make<Mushroom>},
        {EntityType::FireFlower,      "fire_flower",      "Fire Flower",       C::Item,       &make<FireFlower>},
        {EntityType::CapeFeather,     "cape_feather",     "Cape Feather",      C::Item,       &make<CapeFeather>},
        {EntityType::Star,            "star",             "Star (invincible)", C::Item,       &make<Star>},
        {EntityType::OneUpMushroom,   "oneup_mushroom",   "1-Up Mushroom",     C::Item,       &make<OneUpMushroom>},
        {EntityType::MegaMushroom,    "mega_mushroom",    "Mega Mushroom",     C::Item,       &make<MegaMushroom>},
        {EntityType::MiniMushroom,    "mini_mushroom",    "Mini Mushroom",     C::Item,       &make<MiniMushroom>},
        {EntityType::POWBlock,        "pow_block",        "POW Block",         C::Item,       &make<POWBlock>},
        {EntityType::PSwitch,         "pswitch",          "P-Switch",          C::Item,       &make<PSwitch>},
        {EntityType::Trampoline,      "trampoline",       "Trampoline",        C::Item,       &make<Trampoline>},
        {EntityType::BridgeAxe,       "bridge_axe",       "Bridge Axe",        C::Item,       &make<BridgeAxe>},

        // --- Blocks ----------------------------------------------------------
        {EntityType::BrickBlock,      "brick_block",      "Brick Block",       C::Block,      &make<BrickBlock>},
        {EntityType::QuestionBlock,   "question_block",   "Question Block",    C::Block,      &make<QuestionBlock>},
        {EntityType::HiddenBlock,     "hidden_block",     "Hidden Block",      C::Block,      &make<HiddenBlock>},
        {EntityType::IceBlock,        "ice_block",        "Ice Block",         C::Block,      &make<IceBlock>},
        {EntityType::Pipe,            "pipe",             "Pipe",              C::Block,      &make<Pipe>},
        // A zero travel range makes m_rangeLen 0, so update() takes the
        // stationary branch and the platform never moves — every platform
        // placed from a level file or the generator was static (audit B-5).
        // Default to a 4-tile horizontal patrol; level data can override it
        // once the schema carries a range.
        {EntityType::MovingPlatform,  "moving_platform",  "Moving Platform",   C::Block,
         [](sf::Vector2f p) -> std::unique_ptr<Entity> {
             return std::make_unique<MovingPlatform>(
                 p, sf::Vector2f(4.0f * Constants::TILE_SIZE, 0.0f));
         }},
        {EntityType::FallingPlatform, "falling_platform", "Falling Platform",  C::Block,      &make<FallingPlatform>},
        {EntityType::ConveyorBelt,    "conveyor_belt",    "Conveyor Belt",     C::Block,      &make<ConveyorBelt>},

        // --- Scenery ---------------------------------------------------------
        {EntityType::Flagpole,        "flagpole",         "Goal Flagpole",     C::Scenery,    &make<Flagpole>},
        {EntityType::Castle,          "castle",           "Castle (level end)",C::Scenery,    &make<Castle>},

        // --- Projectiles: spawned at runtime, never authored ------------------
        // Both take a launch velocity their thrower decides. The registry only
        // promises "an entity of this type, here", so they are built at rest
        // and the spawn listener sets the velocity afterwards — exactly what
        // the factory's switch did. PlayingState::spawnProjectile does not come
        // through here at all: it acquires these two from their object pools
        // and passes the velocity to the constructor directly.
        {EntityType::Hammer,          "hammer",           "Hammer",            C::Projectile,
         [](sf::Vector2f p) -> std::unique_ptr<Entity> {
             return std::make_unique<Hammer>(p, sf::Vector2f(0.0f, 0.0f));
         }},
        {EntityType::BossFireball,    "boss_fireball",    "Bowser Fireball",   C::Projectile,
         [](sf::Vector2f p) -> std::unique_ptr<Entity> {
             return std::make_unique<BossFireball>(p, sf::Vector2f(0.0f, 0.0f));
         }},
    };
    return kEntries;
}

std::vector<const Entry*> inCategory(Category category) {
    std::vector<const Entry*> found;
    for (const Entry& entry : all()) {
        if (entry.category == category) found.push_back(&entry);
    }
    return found;
}

const Entry* findByName(const std::string& name) {
    // Indexed rather than scanned because these two lookups are now on the
    // construction path: every entity in a level file resolves a name here, and
    // every EntityFactory::create() resolves a type. Both maps are function-
    // local statics for the same initialisation-order reason as all() itself,
    // and they hold pointers INTO all()'s vector — which is const and never
    // resized after construction, so the pointers stay valid for the run.
    static const std::unordered_map<std::string, const Entry*> kByName = [] {
        std::unordered_map<std::string, const Entry*> index;
        for (const Entry& entry : all()) index.emplace(entry.name, &entry);
        return index;
    }();

    auto it = kByName.find(name);
    return it == kByName.end() ? nullptr : it->second;
}

const Entry* findByType(EntityType type) {
    static const std::unordered_map<int, const Entry*> kByType = [] {
        std::unordered_map<int, const Entry*> index;
        for (const Entry& entry : all()) {
            index.emplace(static_cast<int>(entry.type), &entry);
        }
        return index;
    }();

    auto it = kByType.find(static_cast<int>(type));
    return it == kByType.end() ? nullptr : it->second;
}

const char* categoryLabel(Category category) {
    switch (category) {
        case Category::Player:     return "Players";
        case Category::Enemy:      return "Enemies";
        case Category::Item:       return "Items & Power-ups";
        case Category::Block:      return "Blocks & Platforms";
        case Category::Scenery:    return "Scenery & Goal";
        case Category::Projectile: return "Projectiles (runtime only)";
    }
    return "?";
}

const std::vector<Category>& placeableCategories() {
    // Players are deliberately absent.
    //
    // A level file does not contain a player: LevelLoader::saveLevel turns any
    // Mario/Luigi/Toad/Peach in the entity list into the level's "spawnPoint"
    // and writes no entity for it. Offering one in the palette therefore
    // promised something the format cannot keep — and worse, dropping a second
    // Player into a running PlayingState produced an entity that
    // adoptPlayer() had never seen, so the physics engine and the input manager
    // disagreed about which body was the player. The Spawn Point tool is how a
    // start position is authored.
    static const std::vector<Category> kCategories = {
        Category::Enemy, Category::Item,
        Category::Block, Category::Scenery
    };
    return kCategories;
}

} // namespace EntityCatalogue
