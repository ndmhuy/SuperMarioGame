#pragma once

#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include "Utils/IEntityAdmitter.hpp"
#include "Entities/EntityFactory.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class Entity;
class Camera;

// The level-editing engine behind both the in-game F1 overlay (PlayingState)
// and the full-screen editor screen (EditorState).
//
// It owns the tool state, the current selection and the undo history, and it
// mutates the level only through IEditorCommand — which is what makes undo
// total rather than best-effort. It deliberately does NOT own the level: the
// TileMap and the entity vector belong to whichever state is hosting it and are
// passed in per call, so the same editor can drive a live game world and a
// blank canvas without knowing the difference.
//
// It draws no chrome of its own beyond renderImGui(). A host that lays out its
// own panels calls the render*Panel() members individually and places them where
// it likes.
class MapEditor {
public:
    // What a click does. Paint/Erase act on the tile or entity layer according
    // to the palette mode; the rest are layer-specific and say so.
    enum class Tool {
        Paint,       // tiles: paint on drag. entities: place one per click
        Erase,       // remove the tile or entity under the cursor
        RectFill,    // tiles only: drag a rectangle, fill it on release
        Eyedropper,  // adopt the tile or entity type under the cursor
        Select,      // pick an entity, drag to move it, Inspector edits it
        SpawnPoint   // set where the player starts
    };

    MapEditor();
    ~MapEditor() = default;

    // One frame of pointer and camera handling. `mouseScreenPos` is in window
    // pixels and is what the viewport test uses; pass (-1,-1) when there is no
    // window (headless).
    void update(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities,
                const sf::Vector2f& mouseWorldPos, const sf::Vector2f& mouseScreenPos,
                float dt, Camera* camera = nullptr);

    // World-space overlay: the grid, the spawn marker, the selection highlight
    // and the rectangle-fill preview.
    void render(sf::RenderTarget& target, const TileMap& tileMap,
                const std::vector<std::unique_ptr<Entity>>& entities,
                Camera* camera = nullptr) const;

    // The self-contained floating panel, for the in-game F1 overlay. A host with
    // its own layout should call the individual panels below instead.
    void renderImGui(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities);

    // --- Individual panels, for a host that owns its own layout --------------
    void renderToolPanel();
    void renderPalettePanel();
    void renderInspectorPanel(std::vector<std::unique_ptr<Entity>>& entities);
    void renderHistoryPanel();

    void toggleActive();
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    void executeCommand(std::unique_ptr<IEditorCommand> cmd);
    void undo();
    void redo();
    void clearHistory();
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    // The owning state's entity door. Null means "nobody is watching", which is
    // only correct for a headless harness — a live state MUST set this or a
    // placed entity draws as a coloured rectangle and an erased player leaves
    // three dangling pointers behind. See IEntityAdmitter.
    void setEntityAdmitter(IEntityAdmitter* admitter) { m_admitter = admitter; }

    // Confine pointer editing to this window-pixel rectangle.
    //
    // ImGui's io.WantCaptureMouse is read one frame stale in the main loop
    // (Game.cpp updates the state machine before ImGui::SFML::Update), which is
    // survivable for a click and not survivable for a drag: the frame a drag
    // starts over a panel is the frame that decides the whole gesture. A host
    // that knows where its viewport is says so instead of asking ImGui.
    void setViewportRect(const sf::FloatRect& rect);
    void clearViewportRect();

    Tool tool() const { return m_tool; }
    void setTool(Tool tool);

    // Step the palette selection by `delta` entries, wrapping.
    //
    // The palette is otherwise mouse-only, which makes it unreachable both for a
    // keyboard-only user and for the --script verification runs directive 10
    // asks for (scripted events never reach ImGui, only the state stack). Tile
    // mode walks the tile list; entity mode walks the placeable catalogue in
    // palette order.
    void cyclePalette(int delta);

    bool isTileMode() const { return m_isTileMode; }
    TileType selectedTileType() const { return m_selectedTileType; }
    EntityType selectedEntityType() const { return m_selectedEntityType; }

    bool showGrid() const { return m_showGrid; }
    void setShowGrid(bool show) { m_showGrid = show; }

    // Where the player starts, in world pixels. The editor keeps its own copy
    // because a level's spawn point is level data, not an entity: LevelLoader
    // writes it as "spawnPoint" and never as an entity in the list.
    sf::Vector2f spawnPoint() const { return m_spawnPoint; }
    void setSpawnPoint(sf::Vector2f worldPos) { m_spawnPoint = worldPos; }

    // The entity the Inspector is editing, or null.
    Entity* selectedEntity() const { return m_selectedEntity; }
    void selectEntity(Entity* entity) { m_selectedEntity = entity; }
    // Forget a selection that is about to be destroyed by something other than
    // this editor (a level load, a reset).
    void clearSelection() {
        m_selectedEntity = nullptr;
        m_targetLevelSyncedFor = nullptr;
    }
    // Delete key / Inspector button. Routed through EraseEntityCommand, so it
    // is undoable and goes through the admitter like every other removal.
    void deleteSelection(std::vector<std::unique_ptr<Entity>>& entities);

    // Number of steps currently undoable, for the History panel.
    std::size_t historyDepth() const { return m_undoStack.size(); }
    // The undo stack top-down, most recent first, as describe() strings.
    std::vector<std::string> historyLabels() const;

private:
    // Editing is per grid cell. Both mouse handlers need the same three numbers.
    struct Cell {
        int gx = 0;
        int gy = 0;
        bool valid = false;
    };

    void saveLevel(const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities);
    void loadLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities);

    // True when the pointer is somewhere this editor is allowed to act on.
    bool pointerIsOverCanvas(const sf::Vector2f& mouseScreenPos) const;

    void handlePanning(Camera* camera, float dt);
    void handleLeftButton(bool pressedNow, bool wasPressed, const Cell& cell,
                          const sf::Vector2f& mouseWorldPos, TileMap& tileMap,
                          std::vector<std::unique_ptr<Entity>>& entities);
    void handleRightButton(bool pressedNow, const Cell& cell, TileMap& tileMap,
                           std::vector<std::unique_ptr<Entity>>& entities);

    // The entity whose bounding box contains `worldPos`, preferring the last one
    // in the vector so a thing placed on top of another is the one you grab.
    static Entity* entityAt(const std::vector<std::unique_ptr<Entity>>& entities,
                            const sf::Vector2f& worldPos);

    void placeSelectedEntity(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities,
                             const Cell& cell);

    bool m_active = false;
    bool m_showGrid = true;

    // Palette states
    bool m_isTileMode = true;
    Tool m_tool = Tool::Paint;
    TileType m_selectedTileType = TileType::Ground;
    // An EntityType, not a name. It was a std::string matched against sixteen
    // hardcoded branches in PlaceEntityCommand, so the other twenty-four types
    // in the palette placed nothing at all and said nothing about it.
    EntityType m_selectedEntityType = EntityType::Coin;
    // Free-text filter over the entity palette. With every type in the game now
    // listed rather than a hand-picked sixteen, scrolling is no longer the
    // fastest way to find one.
    char m_entityFilter[64] = "";

    // Undo/Redo stacks. Capped: the stack was unbounded, and every one of a
    // drag-paint's hundreds of PlaceTileCommands held a TileMap reference and
    // stayed there for the whole session.
    static constexpr std::size_t MAX_HISTORY = 256;
    std::vector<std::unique_ptr<IEditorCommand>> m_undoStack;
    std::vector<std::unique_ptr<IEditorCommand>> m_redoStack;

    IEntityAdmitter* m_admitter = nullptr;

    // Screen-pixel rect the pointer must be inside, or width<=0 for "anywhere
    // ImGui is not using".
    sf::FloatRect m_viewportRect{{0.0f, 0.0f}, {0.0f, 0.0f}};

    // Mouse edge detection. sf::Mouse::isButtonPressed is a level, and a
    // rectangle drag needs the press and the release.
    bool m_leftWasDown = false;
    bool m_rightWasDown = false;

    // RectFill drag anchor, valid while m_rectDragging.
    bool m_rectDragging = false;
    Cell m_rectAnchor;
    Cell m_rectCursor;

    // Select/Move drag. m_dragOrigin is where the entity was when the drag
    // began, so the whole move is one undoable command rather than one per frame.
    Entity* m_selectedEntity = nullptr;
    bool m_movingSelection = false;
    sf::Vector2f m_dragOrigin{0.0f, 0.0f};
    sf::Vector2f m_dragGrabOffset{0.0f, 0.0f};

    sf::Vector2f m_spawnPoint{64.0f, 640.0f};

    // The Inspector's "Target level" text field, and which entity it currently
    // holds the value of. Refilled only when the selection changes: refilling it
    // every frame from the pipe would overwrite each character as it was typed,
    // so the field could never be edited at all.
    char m_targetLevelInput[128] = "";
    const Entity* m_targetLevelSyncedFor = nullptr;

    // File name for the standalone F1 panel's export/import buttons.
    char m_levelNameInput[64] = "custom_level";
};
