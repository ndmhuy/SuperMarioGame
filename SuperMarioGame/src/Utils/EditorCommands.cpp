#include "Utils/EditorCommands.hpp"
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
#include <algorithm>

// PlaceTileCommand Implementation
PlaceTileCommand::PlaceTileCommand(TileMap& tileMap, int gx, int gy, TileType newType)
    : m_tileMap(tileMap), m_gx(gx), m_gy(gy), m_newType(newType) {
    m_oldType = m_tileMap.getTileType(gx, gy);
}

void PlaceTileCommand::execute() {
    m_tileMap.setTile(m_gx, m_gy, m_newType);
}

void PlaceTileCommand::undo() {
    m_tileMap.setTile(m_gx, m_gy, m_oldType);
}

// EraseTileCommand Implementation
EraseTileCommand::EraseTileCommand(TileMap& tileMap, int gx, int gy)
    : m_tileMap(tileMap), m_gx(gx), m_gy(gy) {
    m_oldType = m_tileMap.getTileType(gx, gy);
}

void EraseTileCommand::execute() {
    m_tileMap.setTile(m_gx, m_gy, TileType::Empty);
}

void EraseTileCommand::undo() {
    m_tileMap.setTile(m_gx, m_gy, m_oldType);
}

// PlaceEntityCommand Implementation
PlaceEntityCommand::PlaceEntityCommand(std::vector<std::unique_ptr<Entity>>& entities, const std::string& type, float wx, float wy)
    : m_entities(entities), m_type(type), m_wx(wx), m_wy(wy) {}

void PlaceEntityCommand::execute() {
    sf::Vector2f pos(m_wx, m_wy);
    std::unique_ptr<Entity> entity;

    if (m_type == "mario") entity = std::make_unique<Mario>(pos);
    else if (m_type == "luigi") entity = std::make_unique<Luigi>(pos);
    else if (m_type == "toad") entity = std::make_unique<Toad>(pos);
    else if (m_type == "peach") entity = std::make_unique<Peach>(pos);
    else if (m_type == "mushroom") entity = std::make_unique<Mushroom>(pos);
    else if (m_type == "oneup_mushroom") entity = std::make_unique<OneUpMushroom>(pos);
    else if (m_type == "mini_mushroom") entity = std::make_unique<MiniMushroom>(pos);
    else if (m_type == "mega_mushroom") entity = std::make_unique<MegaMushroom>(pos);
    else if (m_type == "cape_feather") entity = std::make_unique<CapeFeather>(pos);
    else if (m_type == "fire_flower") entity = std::make_unique<FireFlower>(pos);
    else if (m_type == "star") entity = std::make_unique<Star>(pos);
    else if (m_type == "coin") entity = std::make_unique<Coin>(pos);
    else if (m_type == "star_coin") entity = std::make_unique<StarCoin>(pos);
    else if (m_type == "pswitch") entity = std::make_unique<PSwitch>(pos);
    else if (m_type == "pow_block") entity = std::make_unique<POWBlock>(pos);
    else if (m_type == "trampoline") entity = std::make_unique<Trampoline>(pos);

    if (entity) {
        m_placedEntity = entity.get();
        m_entities.push_back(std::move(entity));
    }
}

void PlaceEntityCommand::undo() {
    if (!m_placedEntity) return;
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [this](const std::unique_ptr<Entity>& e) { return e.get() == m_placedEntity; });
    if (it != m_entities.end()) {
        m_entities.erase(it);
        m_placedEntity = nullptr;
    }
}

// EraseEntityCommand Implementation
EraseEntityCommand::EraseEntityCommand(std::vector<std::unique_ptr<Entity>>& entities, Entity* target)
    : m_entities(entities), m_target(target) {}

void EraseEntityCommand::execute() {
    if (!m_target) return;
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [this](const std::unique_ptr<Entity>& e) { return e.get() == m_target; });
    if (it != m_entities.end()) {
        m_oldIndex = std::distance(m_entities.begin(), it);
        m_backup = std::move(*it);
        m_entities.erase(it);
    }
}

void EraseEntityCommand::undo() {
    if (m_backup) {
        m_target = m_backup.get();
        if (m_oldIndex >= m_entities.size()) {
            m_entities.push_back(std::move(m_backup));
        } else {
            m_entities.insert(m_entities.begin() + m_oldIndex, std::move(m_backup));
        }
    }
}
