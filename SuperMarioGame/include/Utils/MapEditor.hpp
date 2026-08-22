#pragma once

#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class Entity;
class Camera;

class MapEditor {
public:
    MapEditor();
    ~MapEditor() = default;

    void update(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const sf::Vector2f& mouseWorldPos, float dt, Camera* camera = nullptr);
    void render(sf::RenderTarget& target, const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities, Camera* camera = nullptr) const;
    void renderImGui(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities);

    void toggleActive();
    bool isActive() const { return m_active; }

    void executeCommand(std::unique_ptr<IEditorCommand> cmd);
    void undo();
    void redo();
    void clearHistory();

private:
    void saveLevel(const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities);
    void loadLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities);

    bool m_active = false;
    bool m_showGrid = true;

    // Palette states
    bool m_isTileMode = true;
    TileType m_selectedTileType = TileType::Ground;
    std::string m_selectedEntityType = "coin";
    // Free-text filter over the entity palette. With every type in the game now
    // listed rather than a hand-picked sixteen, scrolling is no longer the
    // fastest way to find one.
    char m_entityFilter[64] = "";

    // Undo/Redo stacks
    std::vector<std::unique_ptr<IEditorCommand>> m_undoStack;
    std::vector<std::unique_ptr<IEditorCommand>> m_redoStack;

    // File name
    char m_levelNameInput[64] = "custom_level";
};
