#pragma once

#include "Utils/TileMap.hpp"
#include <memory>
#include <vector>
#include <string>

class Entity;

class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class PlaceTileCommand : public IEditorCommand {
public:
    PlaceTileCommand(TileMap& tileMap, int gx, int gy, TileType newType);
    void execute() override;
    void undo() override;

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

private:
    TileMap& m_tileMap;
    int m_gx, m_gy;
    TileType m_oldType;
};

class PlaceEntityCommand : public IEditorCommand {
public:
    PlaceEntityCommand(std::vector<std::unique_ptr<Entity>>& entities, const std::string& type, float wx, float wy);
    void execute() override;
    void undo() override;

private:
    std::vector<std::unique_ptr<Entity>>& m_entities;
    std::string m_type;
    float m_wx, m_wy;
    Entity* m_placedEntity = nullptr; // Track the placed raw pointer so we can identify and delete it in undo
};

class EraseEntityCommand : public IEditorCommand {
public:
    EraseEntityCommand(std::vector<std::unique_ptr<Entity>>& entities, Entity* target);
    void execute() override;
    void undo() override;

private:
    std::vector<std::unique_ptr<Entity>>& m_entities;
    Entity* m_target;
    std::unique_ptr<Entity> m_backup; // Backup for undo rollback
    size_t m_oldIndex = 0;
};
