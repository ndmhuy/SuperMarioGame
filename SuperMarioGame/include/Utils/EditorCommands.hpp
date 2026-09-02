#pragma once

#include "Utils/TileMap.hpp"
#include "Utils/IEntityAdmitter.hpp"
#include "Entities/EntityFactory.hpp"
#include <SFML/System/Vector2.hpp>
#include <memory>
#include <vector>
#include <string>

class Entity;

class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;

    // One line for the History panel. Present tense, as the action reads on the
    // stack ("Place Goomba"), not as a report of something already done.
    virtual std::string describe() const = 0;
};

class PlaceTileCommand : public IEditorCommand {
public:
    PlaceTileCommand(TileMap& tileMap, int gx, int gy, TileType newType);
    void execute() override;
    void undo() override;
    std::string describe() const override;

private:
    TileMap& m_tileMap;
    int m_gx, m_gy;
    TileType m_newType;
    TileType m_oldType;
};

class EraseTileCommand : public IEditorCommand {
public:
    EraseTileCommand(TileMap& tileMap, int gx, int gy);
    void execute() override;
    void undo() override;
    std::string describe() const override;

private:
    TileMap& m_tileMap;
    int m_gx, m_gy;
    TileType m_oldType;
};

// Paints one rectangle of tiles in a single undoable step.
//
// Painting a 20x10 region a tile at a time put 200 entries on the undo stack,
// so undoing a rectangle meant pressing Ctrl+Z two hundred times.
class FillRectCommand : public IEditorCommand {
public:
    FillRectCommand(TileMap& tileMap, int gx0, int gy0, int gx1, int gy1, TileType newType);
    void execute() override;
    void undo() override;
    std::string describe() const override;

private:
    struct Cell {
        int x = 0;
        int y = 0;
        TileType previous = TileType::Empty;
    };

    TileMap& m_tileMap;
    TileType m_newType;
    std::vector<Cell> m_cells;
};

// Places one entity, built by EntityFactory.
//
// It used to build the entity from an if/else-if chain over sixteen hardcoded
// type-name strings and never touched the factory at all. The palette offers
// forty types, so twenty-four of them — every enemy, every block, the pipe, the
// flagpole — left `entity` null and the command silently did nothing. That is
// the "I can't drop any entity" defect. Holding an EntityType rather than a
// string makes the whole class of failure unrepresentable: there is no name to
// spell wrong and no branch to forget.
class PlaceEntityCommand : public IEditorCommand {
public:
    // `admitter` may be null (a headless test); when it is not, it is the owning
    // state's entity door — see IEntityAdmitter for why skipping it produced a
    // yellow rectangle and a dangling player pointer.
    PlaceEntityCommand(std::vector<std::unique_ptr<Entity>>& entities,
                       EntityType type, float wx, float wy,
                       IEntityAdmitter* admitter = nullptr);
    void execute() override;
    void undo() override;
    std::string describe() const override;

    // The entity this command currently has in the world, or null before the
    // first execute() and between undo() and redo. The editor selects what it
    // just placed with this.
    Entity* placedEntity() const { return m_placed; }

private:
    std::vector<std::unique_ptr<Entity>>& m_entities;
    EntityType m_type;
    float m_wx, m_wy;
    IEntityAdmitter* m_admitter;
    Entity* m_placed = nullptr;
    // undo() parks the entity here instead of destroying it, so the raw pointer
    // stays valid for the whole life of this command. A MoveEntityCommand
    // stacked on top of a Place would otherwise hold a dangling pointer the
    // moment the Place was undone and redone — redo would have constructed a
    // DIFFERENT object at the same conceptual place.
    std::unique_ptr<Entity> m_parked;
};

class EraseEntityCommand : public IEditorCommand {
public:
    EraseEntityCommand(std::vector<std::unique_ptr<Entity>>& entities, Entity* target,
                       IEntityAdmitter* admitter = nullptr);
    void execute() override;
    void undo() override;
    std::string describe() const override;

private:
    std::vector<std::unique_ptr<Entity>>& m_entities;
    Entity* m_target;
    IEntityAdmitter* m_admitter;
    std::unique_ptr<Entity> m_backup; // Keeps the object (and m_target) alive while erased
    size_t m_oldIndex = 0;
};

// Drags an existing entity to a new position.
//
// The pointer is an observer into the entity vector. It stays valid because
// every command in this family that removes an entity parks it rather than
// destroying it, and the history is cleared whenever the whole document is
// replaced (load, new, reset).
class MoveEntityCommand : public IEditorCommand {
public:
    MoveEntityCommand(Entity* target, sf::Vector2f from, sf::Vector2f to);
    void execute() override;
    void undo() override;
    std::string describe() const override;

private:
    Entity* m_target;
    sf::Vector2f m_from;
    sf::Vector2f m_to;
};

// Edits one authorable field of one entity.
//
// question_block.itemType, pipe.pipeId/isEntrance/targetLevel/exit, boss
// arenaX/arenaW have all been in the level schema since it was written and none
// of them were authorable: the only way to set one was to hand-edit the JSON.
// The property is named rather than typed so this one command covers every
// field without a subclass each; the entity-side write lives in apply(), which
// is the single place that knows how a name maps to a setter.
class SetEntityPropertyCommand : public IEditorCommand {
public:
    // Fields the inspector can write. Kept as an enum rather than a string so a
    // typo is a compile error.
    enum class Property {
        QuestionBlockItem,
        PipeId,
        PipeIsEntrance,
        PipeTargetLevel,
        PipeExitX,
        PipeExitY,
        BossArenaX,
        BossArenaW
    };

    // `value` is the numeric form for every property except PipeTargetLevel,
    // which uses `text`. Booleans use 0/1.
    SetEntityPropertyCommand(Entity* target, Property property,
                             float newValue, std::string newText = std::string());
    void execute() override;
    void undo() override;
    std::string describe() const override;

    // Reads the current value of `property` off `entity`, or 0 when the entity
    // does not carry it. Used to seed the command with the value it replaces.
    static float readValue(const Entity* entity, Property property);
    static std::string readText(const Entity* entity, Property property);
    static const char* propertyLabel(Property property);

private:
    void apply(float value, const std::string& text) const;

    Entity* m_target;
    Property m_property;
    float m_newValue;
    std::string m_newText;
    float m_oldValue = 0.0f;
    std::string m_oldText;
};
