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
#include "Entities/HiddenBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"
#include "Entities/IceBlock.hpp"
#include "Entities/ConveyorBelt.hpp"

std::unique_ptr<Entity> EntityFactory::create(EntityType type, sf::Vector2f position) {
    switch (type) {
        // --- PLAYERS ---
        case EntityType::Mario: {
            auto player = std::make_unique<Mario>();
            player->setPosition(position);
            return player;
        }
        case EntityType::Luigi: {
            auto player = std::make_unique<Luigi>();
            player->setPosition(position);
            return player;
        }
        case EntityType::Toad: {
            auto player = std::make_unique<Toad>();
            player->setPosition(position);
            return player;
        }
        case EntityType::Peach: {
            auto player = std::make_unique<Peach>();
            player->setPosition(position);
            return player;
        }

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
        case EntityType::Mushroom: {
            auto item = std::make_unique<Mushroom>();
            item->setPosition(position);
            return item;
        }
        case EntityType::FireFlower: {
            auto item = std::make_unique<FireFlower>();
            item->setPosition(position);
            return item;
        }
        case EntityType::Coin: {
            auto item = std::make_unique<Coin>();
            item->setPosition(position);
            return item;
        }
        case EntityType::Star: {
            auto item = std::make_unique<Star>();
            item->setPosition(position);
            return item;
        }
        case EntityType::OneUpMushroom: {
            auto item = std::make_unique<OneUpMushroom>();
            item->setPosition(position);
            return item;
        }
        case EntityType::CapeFeather: {
            auto item = std::make_unique<CapeFeather>();
            item->setPosition(position);
            return item;
        }
        case EntityType::MegaMushroom: {
            auto item = std::make_unique<MegaMushroom>();
            item->setPosition(position);
            return item;
        }
        case EntityType::MiniMushroom: {
            auto item = std::make_unique<MiniMushroom>();
            item->setPosition(position);
            return item;
        }
        case EntityType::POWBlock: {
            auto item = std::make_unique<POWBlock>();
            item->setPosition(position);
            return item;
        }
        case EntityType::PSwitch: {
            auto item = std::make_unique<PSwitch>();
            item->setPosition(position);
            return item;
        }
        case EntityType::Trampoline: {
            auto item = std::make_unique<Trampoline>();
            item->setPosition(position);
            return item;
        }
        case EntityType::StarCoin: {
            auto item = std::make_unique<StarCoin>();
            item->setPosition(position);
            return item;
        }

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