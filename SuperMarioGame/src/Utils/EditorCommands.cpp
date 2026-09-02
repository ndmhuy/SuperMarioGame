#include "Utils/EditorCommands.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Utils/Constants.hpp"
#include <algorithm>

namespace {

// Human-readable label for a type, for the History panel. Falls back to the
// serialised name and then to the raw enum value, so a type added to
// EntityFactory but not to EntityCatalogue still describes itself as something.
std::string labelFor(EntityType type) {
    if (const EntityCatalogue::Entry* entry = EntityCatalogue::findByType(type)) {
        return entry->label;
    }
    return "entity #" + std::to_string(static_cast<int>(type));
}

std::string tileLabel(TileType type) {
    return TileMap::getInfo(type).name;
}

} // namespace

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

std::string PlaceTileCommand::describe() const {
    return "Paint " + tileLabel(m_newType) + " (" + std::to_string(m_gx) + "," +
           std::to_string(m_gy) + ")";
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

std::string EraseTileCommand::describe() const {
    return "Erase tile (" + std::to_string(m_gx) + "," + std::to_string(m_gy) + ")";
}

// FillRectCommand Implementation
FillRectCommand::FillRectCommand(TileMap& tileMap, int gx0, int gy0, int gx1, int gy1,
                                 TileType newType)
    : m_tileMap(tileMap), m_newType(newType) {
    const int x0 = std::max(0, std::min(gx0, gx1));
    const int x1 = std::max(gx0, gx1);
    const int y0 = std::max(0, std::min(gy0, gy1));
    const int y1 = std::max(gy0, gy1);

    // The grid is grown once, here, rather than by setTile()'s own auto-expand
    // in execute(). setTile expands by gx+10 (TileMap.cpp), so filling right to
    // the edge through it would add ten spurious columns to the saved width.
    m_tileMap.expandToFit(x1 + 1, y1 + 1);

    m_cells.reserve(static_cast<std::size_t>((x1 - x0 + 1) * (y1 - y0 + 1)));
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            m_cells.push_back(Cell{x, y, m_tileMap.getTileType(x, y)});
        }
    }
}

void FillRectCommand::execute() {
    for (const Cell& cell : m_cells) {
        m_tileMap.setTile(cell.x, cell.y, m_newType);
    }
}

void FillRectCommand::undo() {
    for (const Cell& cell : m_cells) {
        m_tileMap.setTile(cell.x, cell.y, cell.previous);
    }
}

std::string FillRectCommand::describe() const {
    return "Fill " + std::to_string(m_cells.size()) + " x " + tileLabel(m_newType);
}

// PlaceEntityCommand Implementation
PlaceEntityCommand::PlaceEntityCommand(std::vector<std::unique_ptr<Entity>>& entities,
                                       EntityType type, float wx, float wy,
                                       IEntityAdmitter* admitter)
    : m_entities(entities), m_type(type), m_wx(wx), m_wy(wy), m_admitter(admitter) {}

void PlaceEntityCommand::execute() {
    std::unique_ptr<Entity> entity;
    if (m_parked) {
        // A redo. Put back the very object the first execute() built, so any
        // command stacked above this one still refers to something alive.
        entity = std::move(m_parked);
    } else {
        entity = EntityFactory::create(m_type, sf::Vector2f(m_wx, m_wy));
    }
    if (!entity) return;

    m_placed = entity.get();
    m_entities.push_back(std::move(entity));
    // After insertion: the admitter is the owning state, and a state that wants
    // to look the entity up in its own vector must be able to find it.
    if (m_admitter) m_admitter->admit(m_placed);
}

void PlaceEntityCommand::undo() {
    if (!m_placed) return;
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [this](const std::unique_ptr<Entity>& e) { return e.get() == m_placed; });
    if (it == m_entities.end()) {
        m_placed = nullptr;
        return;
    }
    if (m_admitter) m_admitter->release(m_placed);
    m_parked = std::move(*it);
    m_entities.erase(it);
    m_placed = nullptr;
}

std::string PlaceEntityCommand::describe() const {
    return "Place " + labelFor(m_type);
}

// EraseEntityCommand Implementation
EraseEntityCommand::EraseEntityCommand(std::vector<std::unique_ptr<Entity>>& entities,
                                       Entity* target, IEntityAdmitter* admitter)
    : m_entities(entities), m_target(target), m_admitter(admitter) {}

void EraseEntityCommand::execute() {
    if (!m_target) return;
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [this](const std::unique_ptr<Entity>& e) { return e.get() == m_target; });
    if (it != m_entities.end()) {
        // Before the vector stops holding it: the owning state may have raw
        // pointers into this object (m_player, InputManager, Game), and an
        // undo that never comes destroys it with this command.
        if (m_admitter) m_admitter->release(m_target);
        m_oldIndex = std::distance(m_entities.begin(), it);
        m_backup = std::move(*it);
        m_entities.erase(it);
    }
}

void EraseEntityCommand::undo() {
    if (m_backup) {
        m_target = m_backup.get();
        Entity* restored = m_target;
        if (m_oldIndex >= m_entities.size()) {
            m_entities.push_back(std::move(m_backup));
        } else {
            m_entities.insert(m_entities.begin() + m_oldIndex, std::move(m_backup));
        }
        if (m_admitter) m_admitter->admit(restored);
    }
}

std::string EraseEntityCommand::describe() const {
    return "Erase " + (m_target ? m_target->getTypeName()
                                : (m_backup ? m_backup->getTypeName() : std::string("entity")));
}

// MoveEntityCommand Implementation
MoveEntityCommand::MoveEntityCommand(Entity* target, sf::Vector2f from, sf::Vector2f to)
    : m_target(target), m_from(from), m_to(to) {}

void MoveEntityCommand::execute() {
    if (m_target) m_target->setPosition(m_to);
}

void MoveEntityCommand::undo() {
    if (m_target) m_target->setPosition(m_from);
}

std::string MoveEntityCommand::describe() const {
    return "Move " + (m_target ? m_target->getTypeName() : std::string("entity"));
}

// SetEntityPropertyCommand Implementation
SetEntityPropertyCommand::SetEntityPropertyCommand(Entity* target, Property property,
                                                   float newValue, std::string newText)
    : m_target(target), m_property(property), m_newValue(newValue),
      m_newText(std::move(newText)) {
    m_oldValue = readValue(target, property);
    m_oldText = readText(target, property);
}

void SetEntityPropertyCommand::execute() {
    apply(m_newValue, m_newText);
}

void SetEntityPropertyCommand::undo() {
    apply(m_oldValue, m_oldText);
}

std::string SetEntityPropertyCommand::describe() const {
    return std::string("Set ") + propertyLabel(m_property);
}

const char* SetEntityPropertyCommand::propertyLabel(Property property) {
    switch (property) {
        case Property::QuestionBlockItem: return "itemType";
        case Property::PipeId:            return "pipeId";
        case Property::PipeIsEntrance:    return "isEntrance";
        case Property::PipeTargetLevel:   return "targetLevel";
        case Property::PipeExitX:         return "exitX";
        case Property::PipeExitY:         return "exitY";
        case Property::BossArenaX:        return "arenaX";
        case Property::BossArenaW:        return "arenaW";
    }
    return "property";
}

float SetEntityPropertyCommand::readValue(const Entity* entity, Property property) {
    if (!entity) return 0.0f;

    if (const auto* block = dynamic_cast<const QuestionBlock*>(entity)) {
        if (property == Property::QuestionBlockItem) {
            return static_cast<float>(block->getContainedItemType());
        }
    }
    if (const auto* pipe = dynamic_cast<const Pipe*>(entity)) {
        switch (property) {
            case Property::PipeId:         return static_cast<float>(pipe->getPipeId());
            case Property::PipeIsEntrance: return pipe->isEntrance() ? 1.0f : 0.0f;
            // Exits are reported in TILES, which is the unit the level file and
            // the inspector both speak; the Pipe itself stores world pixels.
            case Property::PipeExitX:      return pipe->getExitPosition().x / Constants::TILE_SIZE;
            case Property::PipeExitY:      return pipe->getExitPosition().y / Constants::TILE_SIZE;
            default: break;
        }
    }
    if (const auto* boss = dynamic_cast<const Boss*>(entity)) {
        const AABB& arena = boss->getArena();
        switch (property) {
            case Property::BossArenaX: return arena.x / Constants::TILE_SIZE;
            case Property::BossArenaW: return arena.width / Constants::TILE_SIZE;
            default: break;
        }
    }
    return 0.0f;
}

std::string SetEntityPropertyCommand::readText(const Entity* entity, Property property) {
    if (!entity || property != Property::PipeTargetLevel) return std::string();
    if (const auto* pipe = dynamic_cast<const Pipe*>(entity)) return pipe->getTargetLevel();
    return std::string();
}

void SetEntityPropertyCommand::apply(float value, const std::string& text) const {
    if (!m_target) return;

    if (auto* block = dynamic_cast<QuestionBlock*>(m_target)) {
        if (m_property == Property::QuestionBlockItem) {
            block->setContents(static_cast<int>(value));
            return;
        }
    }
    if (auto* pipe = dynamic_cast<Pipe*>(m_target)) {
        // Pipe exposes its warp wiring as one operation rather than four
        // setters, so every field is written together with the current value of
        // the other three.
        int pipeId = pipe->getPipeId();
        bool isEntrance = pipe->isEntrance();
        std::string target = pipe->getTargetLevel();
        sf::Vector2f exit = pipe->getExitPosition();
        switch (m_property) {
            case Property::PipeId:         pipeId = static_cast<int>(value); break;
            case Property::PipeIsEntrance: isEntrance = (value != 0.0f); break;
            case Property::PipeTargetLevel: target = text; break;
            case Property::PipeExitX:      exit.x = value * Constants::TILE_SIZE; break;
            case Property::PipeExitY:      exit.y = value * Constants::TILE_SIZE; break;
            default: return;
        }
        pipe->configureWarp(pipeId, isEntrance, std::move(target), exit);
        return;
    }
    if (auto* boss = dynamic_cast<Boss*>(m_target)) {
        AABB arena = boss->getArena();
        switch (m_property) {
            case Property::BossArenaX: arena.x = value * Constants::TILE_SIZE; break;
            case Property::BossArenaW: arena.width = value * Constants::TILE_SIZE; break;
            default: return;
        }
        boss->setArena(arena);
    }
}
