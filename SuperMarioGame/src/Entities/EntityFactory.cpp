#include "Entities/EntityFactory.hpp"

// Include respective concrete entity headers (only those currently implemented in the codebase)
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
// Note: Bowser.hpp and BoomBoom.hpp are not yet created in the project, so they are not included here.

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

#include "Entities/BrickBlock.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"
#include "Entities/IceBlock.hpp"
#include "Entities/ConveyorBelt.hpp"

std::unique_ptr<Entity> EntityFactory::create(EntityType type, sf::Vector2f position) {
    switch (type) {
        // --- PLAYERS ---
        case EntityType::Mario:
            return std::make_unique<Mario>(position);
        case EntityType::Luigi:
            return std::make_unique<Luigi>(position);
        case EntityType::Toad:
            return std::make_unique<Toad>(position);
        case EntityType::Peach:
            return std::make_unique<Peach>(position);

        // --- ENEMIES ---
        case EntityType::Goomba:
            return std::make_unique<Goomba>(position, false);
        case EntityType::KoopaTroopa:
            return std::make_unique<KoopaTroopa>(position, false);
        case EntityType::KoopaParatroopa:
            return std::make_unique<KoopaParatroopa>(position, false);
        case EntityType::Boo:
            return std::make_unique<Boo>(position);
        case EntityType::PiranhaPlant:
            return std::make_unique<PiranhaPlant>(position);
        case EntityType::BulletBill:
            return std::make_unique<BulletBill>(position);
        case EntityType::HammerBro:
            return std::make_unique<HammerBro>(position);
        case EntityType::Thwomp:
            return std::make_unique<Thwomp>(position);
        case EntityType::ChainChomp:
            return std::make_unique<ChainChomp>(position);
        case EntityType::Lakitu:
            return std::make_unique<Lakitu>(position);
        case EntityType::Spiny:
            return std::make_unique<Spiny>(position);
        case EntityType::Bowser:
        case EntityType::BoomBoom:
            // Bowser and BoomBoom are not yet implemented in the codebase
            return nullptr;

        // --- ITEMS ---
        case EntityType::Mushroom:
            return std::make_unique<Mushroom>(position);
        case EntityType::FireFlower:
            return std::make_unique<FireFlower>(position);
        case EntityType::Coin:
            return std::make_unique<Coin>(position);
        case EntityType::Star:
            return std::make_unique<Star>(position);
        case EntityType::OneUpMushroom:
            return std::make_unique<OneUpMushroom>(position);
        case EntityType::CapeFeather:
            return std::make_unique<CapeFeather>(position);
        case EntityType::MegaMushroom:
            return std::make_unique<MegaMushroom>(position);
        case EntityType::MiniMushroom:
            return std::make_unique<MiniMushroom>(position);
        case EntityType::POWBlock:
            return std::make_unique<POWBlock>(position);
        case EntityType::PSwitch:
            return std::make_unique<PSwitch>(position);
        case EntityType::Trampoline:
            return std::make_unique<Trampoline>(position);
        case EntityType::StarCoin:
            return std::make_unique<StarCoin>(position);

        // --- BLOCKS ---
        case EntityType::BrickBlock:
            return std::make_unique<BrickBlock>(position);
        case EntityType::QuestionBlock:
            return std::make_unique<QuestionBlock>(position);
        case EntityType::Pipe:
            return std::make_unique<Pipe>(position);
        case EntityType::Flagpole:
            return std::make_unique<Flagpole>(position);
        case EntityType::HiddenBlock:
            return std::make_unique<HiddenBlock>(position);
        case EntityType::MovingPlatform:
            return std::make_unique<MovingPlatform>(position, sf::Vector2f(0.f, 0.f));
        case EntityType::FallingPlatform:
            return std::make_unique<FallingPlatform>(position);
        case EntityType::IceBlock:
            return std::make_unique<IceBlock>(position);
        case EntityType::ConveyorBelt:
            return std::make_unique<ConveyorBelt>(position);

        default:
            return nullptr;
    }
}

std::unique_ptr<Entity> EntityFactory::createFireball(sf::Vector2f position, sf::Vector2f velocity) {
    return std::make_unique<Fireball>(position, velocity);
}