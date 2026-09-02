#include "Core/InputManager.hpp"
#include "Utils/MapEditor.hpp"
#include "Utils/Constants.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Graphics/Camera.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <iostream>

namespace {

struct TileChoice { TileType type; const char* label; };

// Every TileType the game has, including the two the old palette left out —
// Lava, which 1-3 is built on and which no one could place, and Used, so a spent
// question block can be authored directly.
const TileChoice kTiles[] = {
    {TileType::Empty,    "Empty (Erase)"},
    {TileType::Ground,   "Ground"},
    {TileType::Brick,    "Brick"},
    {TileType::Question, "Question Block"},
    {TileType::Used,     "Used Block"},
    {TileType::Pipe,     "Pipe"},
    {TileType::Ice,      "Ice"},
    {TileType::Conveyor, "Conveyor Belt"},
    {TileType::Water,    "Water"},
    {TileType::Lava,     "Lava"},
    {TileType::Coin,     "Coin Tile"},
};
constexpr int kTileCount = static_cast<int>(sizeof(kTiles) / sizeof(kTiles[0]));

const char* toolLabel(MapEditor::Tool tool) {
    switch (tool) {
        case MapEditor::Tool::Paint:      return "Paint";
        case MapEditor::Tool::Erase:      return "Erase";
        case MapEditor::Tool::RectFill:   return "Rect Fill";
        case MapEditor::Tool::Eyedropper: return "Eyedropper";
        case MapEditor::Tool::Select:     return "Select / Move";
        case MapEditor::Tool::SpawnPoint: return "Spawn Point";
    }
    return "?";
}

const char* toolHint(MapEditor::Tool tool) {
    switch (tool) {
        case MapEditor::Tool::Paint:      return "Left-drag paints tiles; one click drops one entity.";
        case MapEditor::Tool::Erase:      return "Left-drag erases. Right-click always erases, whatever the tool.";
        case MapEditor::Tool::RectFill:   return "Tiles only. Drag a rectangle; it fills on release, as ONE undo step.";
        case MapEditor::Tool::Eyedropper: return "Click to adopt the tile or entity under the cursor, then switch to Paint.";
        case MapEditor::Tool::Select:     return "Click an entity to inspect it; drag to move it. Delete removes it.";
        case MapEditor::Tool::SpawnPoint: return "Click where the player should start. Saved as the level's spawnPoint.";
    }
    return "";
}

// The item ids a question block can hold, in QuestionBlock::Content order.
const char* const kQuestionContents[] = {
    "Coin", "Super Mushroom", "Fire Flower", "Cape Feather",
    "Star", "Mini Mushroom", "Mega Mushroom", "1-Up Mushroom"
};

} // namespace

MapEditor::MapEditor() = default;

void MapEditor::toggleActive() {
    m_active = !m_active;
}

void MapEditor::setTool(Tool tool) {
    m_tool = tool;
    // A tool switch cancels whatever gesture was half-finished; leaving a
    // rectangle anchor behind made the next click fill from wherever the
    // previous drag started.
    m_rectDragging = false;
    m_movingSelection = false;

    // Rect fill and the spawn tool are tile-layer operations; Select is an
    // entity-layer one. Flipping the palette with the tool means the panel
    // always shows what the tool will actually use.
    if (tool == Tool::RectFill) m_isTileMode = true;
    if (tool == Tool::Select) m_isTileMode = false;
}

void MapEditor::cyclePalette(int delta) {
    if (delta == 0) return;

    if (m_isTileMode) {
        int index = 0;
        for (int i = 0; i < kTileCount; ++i) {
            if (kTiles[i].type == m_selectedTileType) { index = i; break; }
        }
        index = ((index + delta) % kTileCount + kTileCount) % kTileCount;
        m_selectedTileType = kTiles[index].type;
        return;
    }

    // Flattened in the same order the palette draws, so stepping with the
    // keyboard and reading down the list agree.
    std::vector<EntityType> order;
    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        for (const EntityCatalogue::Entry* entry : EntityCatalogue::inCategory(category)) {
            order.push_back(entry->type);
        }
    }
    if (order.empty()) return;

    int index = 0;
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] == m_selectedEntityType) { index = static_cast<int>(i); break; }
    }
    const int count = static_cast<int>(order.size());
    index = ((index + delta) % count + count) % count;
    m_selectedEntityType = order[static_cast<std::size_t>(index)];
}

void MapEditor::setViewportRect(const sf::FloatRect& rect) {
    m_viewportRect = rect;
}

void MapEditor::clearViewportRect() {
    m_viewportRect = sf::FloatRect{{0.0f, 0.0f}, {0.0f, 0.0f}};
}

void MapEditor::executeCommand(std::unique_ptr<IEditorCommand> cmd) {
    if (!cmd) return;
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear(); // A new action invalidates the redo branch

    // Trimmed from the FRONT, which is also what keeps the raw Entity* inside a
    // MoveEntityCommand valid: a move is always recorded before the erase that
    // could destroy what it points at, so the older command is always the one
    // that goes first.
    if (m_undoStack.size() > MAX_HISTORY) {
        m_undoStack.erase(m_undoStack.begin(),
                          m_undoStack.begin() +
                              static_cast<long>(m_undoStack.size() - MAX_HISTORY));
    }
}

void MapEditor::undo() {
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
}

void MapEditor::redo() {
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
}

void MapEditor::clearHistory() {
    m_undoStack.clear();
    m_redoStack.clear();
}

std::vector<std::string> MapEditor::historyLabels() const {
    std::vector<std::string> labels;
    labels.reserve(m_undoStack.size());
    for (auto it = m_undoStack.rbegin(); it != m_undoStack.rend(); ++it) {
        labels.push_back((*it)->describe());
    }
    return labels;
}

void MapEditor::deleteSelection(std::vector<std::unique_ptr<Entity>>& entities) {
    if (!m_selectedEntity) return;
    Entity* victim = m_selectedEntity;
    m_selectedEntity = nullptr;
    m_targetLevelSyncedFor = nullptr;
    m_movingSelection = false;
    executeCommand(std::make_unique<EraseEntityCommand>(entities, victim, m_admitter));
}

void MapEditor::saveLevel(const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities) {
    LevelLoader loader;
    // Into assets/levels/custom/, beside the levels that ship with the game -
    // NOT saves/, which is per-player progress. Writing there is why a level
    // authored in this editor could never be loaded into a game: LevelCatalog
    // only ever looked under assets/levels.
    LevelData meta;
    meta.name = m_levelNameInput;
    meta.spawnPoint = m_spawnPoint;
    const std::string path = LevelCatalog::customPathFor(m_levelNameInput);
    if (path.empty()) {
        std::cerr << "[Editor] No writable custom level directory." << std::endl;
        return;
    }
    if (loader.saveLevel(path, tileMap, entities, meta)) {
        LevelCatalog::refreshCustomLevels();
        std::cout << "[Editor] Exported level successfully to: " << path << std::endl;
    } else {
        std::cerr << "[Editor] Failed to export level to: " << path << std::endl;
    }
}

void MapEditor::loadLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities) {
    LevelLoader loader;
    LevelData data;
    const std::string path = LevelCatalog::customPathFor(m_levelNameInput);
    if (path.empty()) return;
    if (loader.loadLevel(path, tileMap, data)) {
        // Every raw pointer the host holds into the old contents dies here.
        // Before this the editor simply move-assigned over the vector, which
        // left PlayingState::m_player, InputManager and Game pointing at freed
        // Players (audit A-3, which the editor was bypassing).
        if (m_admitter) {
            for (auto& existing : entities) {
                if (existing) m_admitter->release(existing.get());
            }
        }
        entities = std::move(data.entities);
        if (m_admitter) {
            for (auto& fresh : entities) {
                if (fresh) m_admitter->admit(fresh.get());
            }
        }
        m_spawnPoint = data.spawnPoint;
        m_selectedEntity = nullptr;
        clearHistory();
        std::cout << "[Editor] Imported level successfully from: " << path << std::endl;
    } else {
        std::cerr << "[Editor] Failed to import level from: " << path << std::endl;
    }
}

bool MapEditor::pointerIsOverCanvas(const sf::Vector2f& mouseScreenPos) const {
    if (m_viewportRect.size.x > 0.0f && m_viewportRect.size.y > 0.0f) {
        // An explicit rect beats io.WantCaptureMouse, which Game.cpp reads a
        // frame late — fatal for a drag, whose first frame decides the gesture.
        return m_viewportRect.contains(mouseScreenPos);
    }
    return !ImGui::GetIO().WantCaptureMouse;
}

void MapEditor::handlePanning(Camera* camera, float dt) {
    if (!camera) return;
    camera->setBoundsEnabled(false); // The host re-enables on exit

    const float panSpeed = 650.0f;
    InputManager& input = InputManager::getInstance();
    // Not while a text field has the keyboard: typing "Sand Castle" into the
    // level name used to walk the camera four screens down and right.
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (input.isHeld(sf::Keyboard::Key::A) || input.isHeld(sf::Keyboard::Key::Left)) {
            camera->move(sf::Vector2f(-panSpeed * dt, 0.0f));
        }
        if (input.isHeld(sf::Keyboard::Key::D) || input.isHeld(sf::Keyboard::Key::Right)) {
            camera->move(sf::Vector2f(panSpeed * dt, 0.0f));
        }
        if (input.isHeld(sf::Keyboard::Key::W) || input.isHeld(sf::Keyboard::Key::Up)) {
            camera->move(sf::Vector2f(0.0f, -panSpeed * dt));
        }
        if (input.isHeld(sf::Keyboard::Key::S) || input.isHeld(sf::Keyboard::Key::Down)) {
            camera->move(sf::Vector2f(0.0f, panSpeed * dt));
        }
    }

    // Middle-drag panning. The previous position has to be sampled every frame,
    // not only while the button is down, or the first frame of a drag jumps by
    // however far the mouse travelled since the last drag ended.
    static sf::Vector2i lastMousePos;
    if (Game::getInstance().isMouseButtonDown(sf::Mouse::Button::Middle)) {
        const sf::Vector2i currentMousePos = sf::Mouse::getPosition();
        camera->move(sf::Vector2f(lastMousePos - currentMousePos));
    }
    lastMousePos = sf::Mouse::getPosition();
}

Entity* MapEditor::entityAt(const std::vector<std::unique_ptr<Entity>>& entities,
                            const sf::Vector2f& worldPos) {
    // Reverse order: the most recently placed entity is on top of the pile and
    // is the one the pointer means.
    for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
        if (!*it) continue;
        const AABB box = (*it)->getBoundingBox();
        if (worldPos.x >= box.x && worldPos.x <= box.x + box.width &&
            worldPos.y >= box.y && worldPos.y <= box.y + box.height) {
            return it->get();
        }
    }
    return nullptr;
}

void MapEditor::placeSelectedEntity(TileMap& tileMap,
                                    std::vector<std::unique_ptr<Entity>>& entities,
                                    const Cell& cell) {
    const float wx = cell.gx * Constants::TILE_SIZE;
    const float wy = cell.gy * Constants::TILE_SIZE;

    if (cell.gx >= tileMap.getWidth() || cell.gy >= tileMap.getHeight()) {
        tileMap.expandToFit(cell.gx + 1, cell.gy + 1);
    }

    for (const auto& e : entities) {
        if (e && std::abs(e->getPosition().x - wx) < 8.0f &&
            std::abs(e->getPosition().y - wy) < 8.0f) {
            return; // Already something here
        }
    }

    auto command = std::make_unique<PlaceEntityCommand>(entities, m_selectedEntityType,
                                                        wx, wy, m_admitter);
    PlaceEntityCommand* raw = command.get();
    executeCommand(std::move(command));
    // Select what was just dropped, so the Inspector opens on it: placing a pipe
    // and immediately wiring its destination is the common case.
    m_selectedEntity = raw->placedEntity();
}

void MapEditor::handleLeftButton(bool pressedNow, bool wasPressed, const Cell& cell,
                                 const sf::Vector2f& mouseWorldPos, TileMap& tileMap,
                                 std::vector<std::unique_ptr<Entity>>& entities) {
    const bool justPressed = pressedNow && !wasPressed;
    const bool justReleased = !pressedNow && wasPressed;

    switch (m_tool) {
        case Tool::Paint: {
            if (!cell.valid) break;
            if (m_isTileMode) {
                if (!pressedNow) break;
                if (cell.gx >= tileMap.getWidth() || cell.gy >= tileMap.getHeight()) {
                    tileMap.expandToFit(cell.gx + 1, cell.gy + 1);
                }
                if (tileMap.getTileType(cell.gx, cell.gy) != m_selectedTileType) {
                    executeCommand(std::make_unique<PlaceTileCommand>(
                        tileMap, cell.gx, cell.gy, m_selectedTileType));
                }
            } else if (justPressed) {
                // One entity per click, not one per frame the button is held: a
                // held button used to drop one Goomba per tile the mouse crossed.
                placeSelectedEntity(tileMap, entities, cell);
            }
            break;
        }

        case Tool::Erase: {
            if (!pressedNow || !cell.valid) break;
            if (m_isTileMode) {
                if (cell.gx < tileMap.getWidth() && cell.gy < tileMap.getHeight() &&
                    tileMap.getTileType(cell.gx, cell.gy) != TileType::Empty) {
                    executeCommand(std::make_unique<EraseTileCommand>(tileMap, cell.gx, cell.gy));
                }
            } else if (justPressed) {
                if (Entity* target = entityAt(entities, mouseWorldPos)) {
                    if (target == m_selectedEntity) m_selectedEntity = nullptr;
                    executeCommand(std::make_unique<EraseEntityCommand>(entities, target, m_admitter));
                }
            }
            break;
        }

        case Tool::RectFill: {
            if (justPressed && cell.valid) {
                m_rectDragging = true;
                m_rectAnchor = cell;
                m_rectCursor = cell;
            } else if (pressedNow && m_rectDragging && cell.valid) {
                m_rectCursor = cell;
            } else if (justReleased && m_rectDragging) {
                m_rectDragging = false;
                executeCommand(std::make_unique<FillRectCommand>(
                    tileMap, m_rectAnchor.gx, m_rectAnchor.gy,
                    m_rectCursor.gx, m_rectCursor.gy, m_selectedTileType));
            }
            break;
        }

        case Tool::Eyedropper: {
            if (!justPressed || !cell.valid) break;
            if (Entity* target = entityAt(entities, mouseWorldPos)) {
                if (const EntityCatalogue::Entry* entry =
                        EntityCatalogue::findByName(target->getTypeName())) {
                    m_selectedEntityType = entry->type;
                    m_isTileMode = false;
                    m_tool = Tool::Paint;
                    break;
                }
            }
            const TileType picked = tileMap.getTileType(cell.gx, cell.gy);
            if (picked != TileType::Empty) {
                m_selectedTileType = picked;
                m_isTileMode = true;
                m_tool = Tool::Paint;
            }
            break;
        }

        case Tool::Select: {
            if (justPressed) {
                m_selectedEntity = entityAt(entities, mouseWorldPos);
                if (m_selectedEntity) {
                    m_movingSelection = true;
                    m_dragOrigin = m_selectedEntity->getPosition();
                    m_dragGrabOffset = mouseWorldPos - m_dragOrigin;
                }
            } else if (pressedNow && m_movingSelection && m_selectedEntity) {
                // Snapped to the grid: level geometry is tile-aligned, and a
                // pipe half a tile off its column is a bug you find by playing.
                const sf::Vector2f target = mouseWorldPos - m_dragGrabOffset;
                m_selectedEntity->setPosition(sf::Vector2f(
                    std::round(target.x / Constants::TILE_SIZE) * Constants::TILE_SIZE,
                    std::round(target.y / Constants::TILE_SIZE) * Constants::TILE_SIZE));
            } else if (justReleased && m_movingSelection && m_selectedEntity) {
                m_movingSelection = false;
                const sf::Vector2f settled = m_selectedEntity->getPosition();
                if (settled != m_dragOrigin) {
                    // Put it back first: the command's execute() is what performs
                    // the move, so the history holds one entry for the whole drag
                    // rather than one per frame.
                    m_selectedEntity->setPosition(m_dragOrigin);
                    executeCommand(std::make_unique<MoveEntityCommand>(
                        m_selectedEntity, m_dragOrigin, settled));
                }
            }
            break;
        }

        case Tool::SpawnPoint: {
            if (!justPressed || !cell.valid) break;
            m_spawnPoint = sf::Vector2f(cell.gx * Constants::TILE_SIZE,
                                        cell.gy * Constants::TILE_SIZE);
            break;
        }
    }
}

void MapEditor::handleRightButton(bool pressedNow, const Cell& cell, TileMap& tileMap,
                                  std::vector<std::unique_ptr<Entity>>& entities) {
    if (!pressedNow || !cell.valid) return;
    if (cell.gx >= tileMap.getWidth() || cell.gy >= tileMap.getHeight()) return;

    if (m_isTileMode) {
        if (tileMap.getTileType(cell.gx, cell.gy) != TileType::Empty) {
            executeCommand(std::make_unique<EraseTileCommand>(tileMap, cell.gx, cell.gy));
        }
        return;
    }

    if (m_rightWasDown) return; // One entity per click

    const float wx = cell.gx * Constants::TILE_SIZE;
    const float wy = cell.gy * Constants::TILE_SIZE;
    Entity* target = nullptr;
    for (const auto& e : entities) {
        if (e && std::abs(e->getPosition().x - wx) < Constants::TILE_SIZE &&
            std::abs(e->getPosition().y - wy) < Constants::TILE_SIZE) {
            target = e.get();
            break;
        }
    }
    if (!target) return;
    if (target == m_selectedEntity) m_selectedEntity = nullptr;
    executeCommand(std::make_unique<EraseEntityCommand>(entities, target, m_admitter));
}

void MapEditor::update(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities,
                       const sf::Vector2f& mouseWorldPos, const sf::Vector2f& mouseScreenPos,
                       float dt, Camera* camera) {
    if (!m_active) return;

    handlePanning(camera, dt);

    // Through Game, not sf::Mouse: a --script verification run drives a
    // synthetic pointer, and the editor is the one screen in the game that
    // cannot be exercised by the keyboard alone (see Game::isMouseButtonDown).
    Game& game = Game::getInstance();
    const bool leftDown = game.isMouseButtonDown(sf::Mouse::Button::Left);
    const bool rightDown = game.isMouseButtonDown(sf::Mouse::Button::Right);

    if (!pointerIsOverCanvas(mouseScreenPos)) {
        // A drag that leaves the canvas must still finish, or a rectangle
        // released over a panel is silently abandoned and a moved entity is
        // left mid-drag with no undo entry.
        if (m_rectDragging && !leftDown) {
            m_rectDragging = false;
            executeCommand(std::make_unique<FillRectCommand>(
                tileMap, m_rectAnchor.gx, m_rectAnchor.gy,
                m_rectCursor.gx, m_rectCursor.gy, m_selectedTileType));
        }
        if (m_movingSelection && !leftDown && m_selectedEntity) {
            m_movingSelection = false;
            const sf::Vector2f settled = m_selectedEntity->getPosition();
            if (settled != m_dragOrigin) {
                m_selectedEntity->setPosition(m_dragOrigin);
                executeCommand(std::make_unique<MoveEntityCommand>(
                    m_selectedEntity, m_dragOrigin, settled));
            }
        }
        m_leftWasDown = leftDown;
        m_rightWasDown = rightDown;
        return;
    }

    Cell cell;
    cell.gx = static_cast<int>(std::floor(mouseWorldPos.x / Constants::TILE_SIZE));
    cell.gy = static_cast<int>(std::floor(mouseWorldPos.y / Constants::TILE_SIZE));
    cell.valid = (cell.gx >= 0 && cell.gy >= 0);

    handleLeftButton(leftDown, m_leftWasDown, cell, mouseWorldPos, tileMap, entities);
    handleRightButton(rightDown, cell, tileMap, entities);

    m_leftWasDown = leftDown;
    m_rightWasDown = rightDown;
}

void MapEditor::render(sf::RenderTarget& target, const TileMap& tileMap,
                       const std::vector<std::unique_ptr<Entity>>& entities,
                       Camera* camera) const {
    if (!m_active) return;

    const float size = Constants::TILE_SIZE;

    if (m_showGrid) {
        int renderWidth = tileMap.getWidth();
        if (camera) {
            const float cameraRightX = camera->getView().getCenter().x + Constants::WINDOW_WIDTH * 0.5f;
            const int cameraRightTile = static_cast<int>(std::floor(cameraRightX / size)) + 10;
            renderWidth = std::max(renderWidth, cameraRightTile);
        }
        const int height = tileMap.getHeight();

        const sf::Color gridColor(255, 255, 255, 45);
        const sf::Color outOfBoundsGridColor(0, 200, 255, 60);

        for (int x = 0; x <= renderWidth; ++x) {
            const sf::Color col = (x <= tileMap.getWidth()) ? gridColor : outOfBoundsGridColor;
            sf::Vertex line[] = {
                sf::Vertex{sf::Vector2f(x * size, 0.0f), col},
                sf::Vertex{sf::Vector2f(x * size, height * size), col}
            };
            target.draw(line, 2, sf::PrimitiveType::Lines);
        }
        for (int y = 0; y <= height; ++y) {
            sf::Vertex line[] = {
                sf::Vertex{sf::Vector2f(0.0f, y * size), gridColor},
                sf::Vertex{sf::Vector2f(renderWidth * size, y * size), gridColor}
            };
            target.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }

    // The rectangle being dragged, so the fill is not a blind gesture.
    if (m_rectDragging) {
        const float x0 = std::min(m_rectAnchor.gx, m_rectCursor.gx) * size;
        const float y0 = std::min(m_rectAnchor.gy, m_rectCursor.gy) * size;
        const float w = (std::abs(m_rectAnchor.gx - m_rectCursor.gx) + 1) * size;
        const float h = (std::abs(m_rectAnchor.gy - m_rectCursor.gy) + 1) * size;
        sf::RectangleShape preview({w, h});
        preview.setPosition({x0, y0});
        preview.setFillColor(sf::Color(80, 200, 255, 60));
        preview.setOutlineColor(sf::Color(120, 220, 255, 220));
        preview.setOutlineThickness(2.0f);
        target.draw(preview);
    }

    // The selection, so the Inspector is visibly about something.
    if (m_selectedEntity) {
        const AABB box = m_selectedEntity->getBoundingBox();
        sf::RectangleShape highlight({box.width, box.height});
        highlight.setPosition({box.x, box.y});
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineColor(sf::Color(255, 220, 60, 230));
        highlight.setOutlineThickness(2.0f);
        target.draw(highlight);
    }
    (void)entities;

    // The spawn point. A level whose spawn is authorable needs it drawn, or the
    // only way to find out where the player starts is to play the level.
    sf::RectangleShape spawnMarker({size, size});
    spawnMarker.setPosition(m_spawnPoint);
    spawnMarker.setFillColor(sf::Color(60, 220, 120, 70));
    spawnMarker.setOutlineColor(sf::Color(60, 255, 140, 240));
    spawnMarker.setOutlineThickness(2.0f);
    target.draw(spawnMarker);
}

// --- ImGui panels ---------------------------------------------------------

void MapEditor::renderToolPanel() {
    ImGui::Checkbox("Show Grid", &m_showGrid);
    ImGui::Separator();

    static const Tool kTools[] = {Tool::Paint, Tool::Erase, Tool::RectFill,
                                  Tool::Eyedropper, Tool::Select, Tool::SpawnPoint};
    for (int i = 0; i < 6; ++i) {
        if (i % 2 != 0) ImGui::SameLine();
        const bool selected = (m_tool == kTools[i]);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.85f, 1.0f));
        if (ImGui::Button(toolLabel(kTools[i]), ImVec2(118.0f, 0.0f))) {
            setTool(kTools[i]);
        }
        if (selected) ImGui::PopStyleColor();
    }
    ImGui::TextWrapped("%s", toolHint(m_tool));
    ImGui::TextDisabled("Keys: B paint  E erase  R rect  I pick  V select  P spawn\n"
                        "      Q / T step the palette below");
}

void MapEditor::renderPalettePanel() {
    if (ImGui::RadioButton("Tiles", m_isTileMode)) m_isTileMode = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Entities", !m_isTileMode)) m_isTileMode = false;
    ImGui::Separator();

    if (m_isTileMode) {
        for (int i = 0; i < kTileCount; ++i) {
            // Two to a row: eleven in a column pushed everything below the
            // panel's bottom edge.
            if (i % 2 != 0) ImGui::SameLine();
            const bool selected = (m_selectedTileType == kTiles[i].type);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.85f, 1.0f));
            if (ImGui::Button(kTiles[i].label, ImVec2(118.0f, 0.0f))) {
                m_selectedTileType = kTiles[i].type;
            }
            if (selected) ImGui::PopStyleColor();
        }
        return;
    }

    // The palette is generated from EntityCatalogue rather than written out
    // here. It used to be a hand-kept list of sixteen names that had fallen
    // behind the game: not one enemy, not one block, no pipe and no flagpole
    // could be placed in the level editor at all.
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##EntityFilter", "filter by name...",
                             m_entityFilter, sizeof(m_entityFilter));

    std::string needle = m_entityFilter;
    std::transform(needle.begin(), needle.end(), needle.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const EntityCatalogue::Entry* selectedEntry =
        EntityCatalogue::findByType(m_selectedEntityType);
    ImGui::TextDisabled("Placing: %s", selectedEntry ? selectedEntry->label.c_str() : "?");

    ImGui::BeginChild("##EntityPalette", ImVec2(0.0f, 0.0f), true);
    int shown = 0;
    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        std::vector<const EntityCatalogue::Entry*> entries =
            EntityCatalogue::inCategory(category);

        // Filter first, so an empty category header is not drawn over a search
        // that excluded all of it.
        std::vector<const EntityCatalogue::Entry*> matching;
        for (const EntityCatalogue::Entry* entry : entries) {
            if (needle.empty()) { matching.push_back(entry); continue; }
            std::string haystack = entry->label + " " + entry->name;
            std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (haystack.find(needle) != std::string::npos) matching.push_back(entry);
        }
        if (matching.empty()) continue;

        // Open by default while filtering: a search whose hits are inside
        // collapsed headers looks like a search that found nothing.
        ImGui::SetNextItemOpen(true, needle.empty() ? ImGuiCond_FirstUseEver : ImGuiCond_Always);
        if (!ImGui::CollapsingHeader(EntityCatalogue::categoryLabel(category))) continue;

        for (const EntityCatalogue::Entry* entry : matching) {
            ++shown;
            const bool selected = (m_selectedEntityType == entry->type);
            if (ImGui::Selectable(entry->label.c_str(), selected)) {
                m_selectedEntityType = entry->type;
                m_isTileMode = false;
                if (m_tool != Tool::Paint) setTool(Tool::Paint);
            }
            if (ImGui::IsItemHovered()) {
                // The serialised name, so a level file can be hand-checked
                // against what the editor wrote.
                ImGui::SetTooltip("%s", entry->name.c_str());
            }
        }
    }
    if (shown == 0) {
        ImGui::TextDisabled("No entity matches \"%s\".", m_entityFilter);
    }
    ImGui::EndChild();
}

void MapEditor::renderInspectorPanel(std::vector<std::unique_ptr<Entity>>& entities) {
    if (!m_selectedEntity) {
        ImGui::TextDisabled("Nothing selected.");
        ImGui::TextWrapped("Pick the Select tool and click an entity to edit "
                           "the fields the level file carries for it.");
        return;
    }

    Entity* entity = m_selectedEntity;
    ImGui::Text("%s", entity->getTypeName().c_str());
    const sf::Vector2f pos = entity->getPosition();
    ImGui::TextDisabled("tile (%d, %d)  world (%.0f, %.0f)",
                        static_cast<int>(pos.x / Constants::TILE_SIZE),
                        static_cast<int>(pos.y / Constants::TILE_SIZE), pos.x, pos.y);
    ImGui::Separator();

    using Property = SetEntityPropertyCommand::Property;
    auto issue = [this](Entity* target, Property property, float value,
                        std::string text = std::string()) {
        executeCommand(std::make_unique<SetEntityPropertyCommand>(
            target, property, value, std::move(text)));
    };

    if (auto* block = dynamic_cast<QuestionBlock*>(entity)) {
        int content = block->getContainedItemType();
        if (ImGui::Combo("Contains", &content, kQuestionContents,
                         IM_ARRAYSIZE(kQuestionContents))) {
            issue(entity, Property::QuestionBlockItem, static_cast<float>(content));
        }
        ImGui::TextDisabled("Written as \"itemType\" in the level JSON.");
    } else if (auto* pipe = dynamic_cast<Pipe*>(entity)) {
        int pipeId = pipe->getPipeId();
        if (ImGui::InputInt("Pipe ID", &pipeId)) {
            issue(entity, Property::PipeId, static_cast<float>(pipeId));
        }
        bool entrance = pipe->isEntrance();
        if (ImGui::Checkbox("Is entrance (warps)", &entrance)) {
            issue(entity, Property::PipeIsEntrance, entrance ? 1.0f : 0.0f);
        }

        if (m_targetLevelSyncedFor != entity) {
            std::snprintf(m_targetLevelInput, sizeof(m_targetLevelInput), "%s",
                          pipe->getTargetLevel().c_str());
            m_targetLevelSyncedFor = entity;
        }
        if (ImGui::InputText("Target level", m_targetLevelInput, sizeof(m_targetLevelInput),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            issue(entity, Property::PipeTargetLevel, 0.0f, std::string(m_targetLevelInput));
        }
        ImGui::TextDisabled("Empty = warp within this level. Press Enter to apply.");

        int exitX = static_cast<int>(pipe->getExitPosition().x / Constants::TILE_SIZE);
        int exitY = static_cast<int>(pipe->getExitPosition().y / Constants::TILE_SIZE);
        if (ImGui::InputInt("Exit X (tiles)", &exitX)) {
            issue(entity, Property::PipeExitX, static_cast<float>(exitX));
        }
        if (ImGui::InputInt("Exit Y (tiles)", &exitY)) {
            issue(entity, Property::PipeExitY, static_cast<float>(exitY));
        }
    } else if (auto* boss = dynamic_cast<Boss*>(entity)) {
        int arenaX = static_cast<int>(boss->getArena().x / Constants::TILE_SIZE);
        int arenaW = static_cast<int>(boss->getArena().width / Constants::TILE_SIZE);
        if (ImGui::InputInt("Arena X (tiles)", &arenaX)) {
            issue(entity, Property::BossArenaX, static_cast<float>(arenaX));
        }
        if (ImGui::InputInt("Arena width (tiles)", &arenaW)) {
            issue(entity, Property::BossArenaW, static_cast<float>(arenaW));
        }
        ImGui::TextDisabled("The room the fight is locked into.");
    } else {
        ImGui::TextDisabled("This type carries no authorable fields.");
    }

    ImGui::Separator();
    if (ImGui::Button("Delete selected (Del)")) {
        deleteSelection(entities);
    }
}

void MapEditor::renderHistoryPanel() {
    ImGui::BeginDisabled(!canUndo());
    if (ImGui::Button("Undo")) undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canRedo());
    if (ImGui::Button("Redo")) redo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Clear History")) clearHistory();

    ImGui::TextDisabled("%zu step(s), cap %zu", m_undoStack.size(), MAX_HISTORY);
    ImGui::BeginChild("##History", ImVec2(0.0f, 0.0f), true);
    for (const std::string& label : historyLabels()) {
        ImGui::BulletText("%s", label.c_str());
    }
    ImGui::EndChild();
}

void MapEditor::renderImGui(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities) {
    if (!m_active) return;

    // Right-hand column, clear of the navigation and generator panels on the
    // left. With no position of its own this window opened at ImGui's default
    // spot and sat underneath "Gameplay Controls & Navigation".
    ImGui::SetNextWindowPos(ImVec2(912.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 700.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mario Maker - In-Game Level Editor (F1)");

    renderToolPanel();
    ImGui::Separator();

    if (ImGui::BeginTabBar("##F1Tabs")) {
        if (ImGui::BeginTabItem("Palette")) {
            ImGui::BeginChild("##F1Palette", ImVec2(0.0f, 240.0f));
            renderPalettePanel();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Inspector")) {
            ImGui::BeginChild("##F1Inspector", ImVec2(0.0f, 240.0f));
            renderInspectorPanel(entities);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("History")) {
            ImGui::BeginChild("##F1History", ImVec2(0.0f, 240.0f));
            renderHistoryPanel();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Text("Custom level name:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##MapFileName", m_levelNameInput, sizeof(m_levelNameInput));
    ImGui::TextDisabled("%s/%s.json", LevelCatalog::customDirectory().c_str(),
                        LevelCatalog::toFileStem(m_levelNameInput).c_str());

    if (ImGui::Button("Export Level JSON")) {
        saveLevel(tileMap, entities);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Level JSON")) {
        loadLevel(tileMap, entities);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset / Clear Entire Map")) {
        // Through the admitter, for the same reason loadLevel() is: this used to
        // be a bare entities.clear() that freed the live Player out from under
        // PlayingState::m_player, InputManager and Game.
        if (m_admitter) {
            for (auto& existing : entities) {
                if (existing) m_admitter->release(existing.get());
            }
        }
        tileMap.initialize(tileMap.getWidth(), tileMap.getHeight());
        entities.clear();
        m_selectedEntity = nullptr;
        clearHistory();
    }

    ImGui::Separator();
    if (ImGui::Button("Switch to Play Mode (F1)")) {
        toggleActive();
    }
    ImGui::SameLine();
    if (ImGui::Button("Return to Main Menu")) {
        toggleActive();
        Game::getInstance().changeState(std::make_unique<MenuState>());
    }

    ImGui::End();
}
