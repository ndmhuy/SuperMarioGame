#pragma once

#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class Entity;

class MapEditor {
public:
    MapEditor();
    ~MapEditor() = default;

    void update(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const sf::Vector2f& mouseWorldPos, float dt);
    void render(sf::RenderTarget& target, const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities) const;
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

    // Undo/Redo stacks
    std::vector<std::unique_ptr<IEditorCommand>> m_undoStack;
    std::vector<std::unique_ptr<IEditorCommand>> m_redoStack;

    // File name
    char m_levelNameInput[64] = "custom_level";
};
