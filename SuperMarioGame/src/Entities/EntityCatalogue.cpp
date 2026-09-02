#include "Entities/EntityCatalogue.hpp"

#include <algorithm>

namespace EntityCatalogue {

const std::vector<Entry>& all() {
    using C = Category;
    // Names here are the canonical ones. verify_regressions checks each against
    // the class's own getTypeName(), so a mismatch is a failing test rather than
    // a level file that loads as a Goomba.
    static const std::vector<Entry> kEntries = {
        // --- Players ---------------------------------------------------------
        {EntityType::Mario,           "mario",            "Mario",             C::Player},
        {EntityType::Luigi,           "luigi",            "Luigi",             C::Player},
        {EntityType::Toad,            "toad",             "Toad",              C::Player},
        {EntityType::Peach,           "peach",            "Peach",             C::Player},

        // --- Enemies ---------------------------------------------------------
        {EntityType::Goomba,          "goomba",           "Goomba",            C::Enemy},
        {EntityType::KoopaTroopa,     "koopa_troopa",     "Koopa Troopa",      C::Enemy},
        {EntityType::KoopaParatroopa, "koopa_paratroopa", "Koopa Paratroopa",  C::Enemy},
        {EntityType::Spiny,           "spiny",            "Spiny",             C::Enemy},
        {EntityType::Boo,             "boo",              "Boo",               C::Enemy},
        {EntityType::PiranhaPlant,    "piranha_plant",    "Piranha Plant",     C::Enemy},
        {EntityType::BulletBill,      "bullet_bill",      "Bullet Bill",       C::Enemy},
        {EntityType::HammerBro,       "hammer_bro",       "Hammer Bro",        C::Enemy},
        {EntityType::Thwomp,          "thwomp",           "Thwomp",            C::Enemy},
        {EntityType::ChainChomp,      "chain_chomp",      "Chain Chomp",       C::Enemy},
        {EntityType::Lakitu,          "lakitu",           "Lakitu",            C::Enemy},
        {EntityType::BoomBoom,        "boom_boom",        "Boss: Boom Boom",   C::Enemy},
        {EntityType::Bowser,          "bowser",           "Boss: Bowser",      C::Enemy},

        // --- Items -----------------------------------------------------------
        {EntityType::Coin,            "coin",             "Coin",              C::Item},
        {EntityType::StarCoin,        "star_coin",        "Star Coin",         C::Item},
        {EntityType::Mushroom,        "mushroom",         "Super Mushroom",    C::Item},
        {EntityType::FireFlower,      "fire_flower",      "Fire Flower",       C::Item},
        {EntityType::CapeFeather,     "cape_feather",     "Cape Feather",      C::Item},
        {EntityType::Star,            "star",             "Star (invincible)", C::Item},
        {EntityType::OneUpMushroom,   "oneup_mushroom",   "1-Up Mushroom",     C::Item},
        {EntityType::MegaMushroom,    "mega_mushroom",    "Mega Mushroom",     C::Item},
        {EntityType::MiniMushroom,    "mini_mushroom",    "Mini Mushroom",     C::Item},
        {EntityType::POWBlock,        "pow_block",        "POW Block",         C::Item},
        {EntityType::PSwitch,         "pswitch",          "P-Switch",          C::Item},
        {EntityType::Trampoline,      "trampoline",       "Trampoline",        C::Item},
        {EntityType::BridgeAxe,       "bridge_axe",       "Bridge Axe",        C::Item},

        // --- Blocks ----------------------------------------------------------
        {EntityType::BrickBlock,      "brick_block",      "Brick Block",       C::Block},
        {EntityType::QuestionBlock,   "question_block",   "Question Block",    C::Block},
        {EntityType::HiddenBlock,     "hidden_block",     "Hidden Block",      C::Block},
        {EntityType::IceBlock,        "ice_block",        "Ice Block",         C::Block},
        {EntityType::Pipe,            "pipe",             "Pipe",              C::Block},
        {EntityType::MovingPlatform,  "moving_platform",  "Moving Platform",   C::Block},
        {EntityType::FallingPlatform, "falling_platform", "Falling Platform",  C::Block},
        {EntityType::ConveyorBelt,    "conveyor_belt",    "Conveyor Belt",     C::Block},

        // --- Scenery ---------------------------------------------------------
        {EntityType::Flagpole,        "flagpole",         "Goal Flagpole",     C::Scenery},
        {EntityType::Castle,          "castle",           "Castle (level end)",C::Scenery},

        // --- Projectiles: spawned at runtime, never authored ------------------
        {EntityType::Hammer,          "hammer",           "Hammer",            C::Projectile},
        {EntityType::BossFireball,    "boss_fireball",    "Bowser Fireball",   C::Projectile},
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
    for (const Entry& entry : all()) {
        if (entry.name == name) return &entry;
    }
    return nullptr;
}

const Entry* findByType(EntityType type) {
    for (const Entry& entry : all()) {
        if (entry.type == type) return &entry;
    }
    return nullptr;
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
