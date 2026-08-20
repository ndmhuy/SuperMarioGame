#include "Core/InputManager.hpp"
#include "Utils/MapEditor.hpp"
#include "Utils/Constants.hpp"
#include "Entities/Entity.hpp"
#include "Utils/LevelLoader.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>

MapEditor::MapEditor() {
    m_active = false;
    m_showGrid = true;
    m_isTileMode = true;
    m_selectedTileType = TileType::Ground;
    m_selectedEntityType = "coin";
}

void MapEditor::toggleActive() {
    m_active = !m_active;
}

void MapEditor::executeCommand(std::unique_ptr<IEditorCommand> cmd) {
    if (!cmd) return;
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear(); // Clear redo stack on new action
}

void MapEditor::undo() {
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->undo();
    m_redoStack.push_back(std::move(cmd));
    std::cout << "[Editor] Undo action performed." << std::endl;
}

void MapEditor::redo() {
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->execute();
    m_undoStack.push_back(std::move(cmd));
    std::cout << "[Editor] Redo action performed." << std::endl;
}

void MapEditor::clearHistory() {
    m_undoStack.clear();
    m_redoStack.clear();
}

void MapEditor::saveLevel(const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities) {
    LevelLoader loader;
    std::string path = "saves/" + std::string(m_levelNameInput) + ".json";
    bool success = loader.saveLevel(path, tileMap, entities, m_levelNameInput);
    if (success) {
        std::cout << "[Editor] Exported level successfully to: " << path << std::endl;
    } else {
        std::cerr << "[Editor] Failed to export level to: " << path << std::endl;
    }
}

void MapEditor::loadLevel(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities) {
    LevelLoader loader;
    LevelData data;
    std::string path = "saves/" + std::string(m_levelNameInput) + ".json";
    bool success = loader.loadLevel(path, tileMap, data);
    if (success) {
        entities = std::move(data.entities);
        clearHistory();
        std::cout << "[Editor] Imported level successfully from: " << path << std::endl;
    } else {
        std::cerr << "[Editor] Failed to import level from: " << path << std::endl;
    }
}


#include "Graphics/Camera.hpp"

void MapEditor::update(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities, const sf::Vector2f& mouseWorldPos, float dt, Camera* camera) {
    if (!m_active) return;

    // 1. Free Camera Panning Controls (A / D / W / S or Arrow Keys or Middle Mouse Drag)
    if (camera) {
        camera->setBoundsEnabled(false); // PlayingState re-enables on exit
        float panSpeed = 650.0f;
        if (InputManager::getInstance().isHeld(sf::Keyboard::Key::A) || InputManager::getInstance().isHeld(sf::Keyboard::Key::Left)) {
            camera->move(sf::Vector2f(-panSpeed * dt, 0.0f));
        }
        if (InputManager::getInstance().isHeld(sf::Keyboard::Key::D) || InputManager::getInstance().isHeld(sf::Keyboard::Key::Right)) {
            camera->move(sf::Vector2f(panSpeed * dt, 0.0f));
        }
        if (InputManager::getInstance().isHeld(sf::Keyboard::Key::W) || InputManager::getInstance().isHeld(sf::Keyboard::Key::Up)) {
            camera->move(sf::Vector2f(0.0f, -panSpeed * dt));
        }
        if (InputManager::getInstance().isHeld(sf::Keyboard::Key::S) || InputManager::getInstance().isHeld(sf::Keyboard::Key::Down)) {
            camera->move(sf::Vector2f(0.0f, panSpeed * dt));
        }

        // Mouse Drag Panning (Middle Mouse Button)
        static sf::Vector2i lastMousePos;
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle)) {
            sf::Vector2i currentMousePos = sf::Mouse::getPosition();
            sf::Vector2f delta = sf::Vector2f(lastMousePos - currentMousePos);
            camera->move(delta);
        }
        lastMousePos = sf::Mouse::getPosition();
    }

    // Trigger placement or deletion via mouse clicks when not interacting with ImGui panels
    if (ImGui::GetIO().WantCaptureMouse) return;

    int gx = static_cast<int>(std::floor(mouseWorldPos.x / Constants::TILE_SIZE));
    int gy = static_cast<int>(std::floor(mouseWorldPos.y / Constants::TILE_SIZE));

    // Auto-expansion and free placement beyond current map bounds
    if (gx >= 0 && gy >= 0) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            if (m_isTileMode) {
                if (gx >= tileMap.getWidth() || gy >= tileMap.getHeight()) {
                    tileMap.expandToFit(gx + 10, gy + 1);
                }
                if (tileMap.getTileType(gx, gy) != m_selectedTileType) {
                    executeCommand(std::make_unique<PlaceTileCommand>(tileMap, gx, gy, m_selectedTileType));
                }
            } else {
                if (gx >= tileMap.getWidth() || gy >= tileMap.getHeight()) {
                    tileMap.expandToFit(gx + 10, gy + 1);
                }
                float wx = gx * Constants::TILE_SIZE;
                float wy = gy * Constants::TILE_SIZE;
                bool occupied = false;
                for (const auto& e : entities) {
                    if (e && std::abs(e->getPosition().x - wx) < 8.0f && std::abs(e->getPosition().y - wy) < 8.0f) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied) {
                    executeCommand(std::make_unique<PlaceEntityCommand>(entities, m_selectedEntityType, wx, wy));
                }
            }
        } else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
            if (gx < tileMap.getWidth() && gy < tileMap.getHeight()) {
                if (m_isTileMode) {
                    if (tileMap.getTileType(gx, gy) != TileType::Empty) {
                        executeCommand(std::make_unique<EraseTileCommand>(tileMap, gx, gy));
                    }
                } else {
                    float wx = gx * Constants::TILE_SIZE;
                    float wy = gy * Constants::TILE_SIZE;
                    Entity* target = nullptr;
                    for (const auto& e : entities) {
                        if (e && std::abs(e->getPosition().x - wx) < Constants::TILE_SIZE && std::abs(e->getPosition().y - wy) < Constants::TILE_SIZE) {
                            target = e.get();
                            break;
                        }
                    }
                    if (target) {
                        executeCommand(std::make_unique<EraseEntityCommand>(entities, target));
                    }
                }
            }
        }
    }
}

void MapEditor::render(sf::RenderTarget& target, const TileMap& tileMap, const std::vector<std::unique_ptr<Entity>>& entities, Camera* camera) const {
    if (!m_active || !m_showGrid) return;

    int renderWidth = tileMap.getWidth();
    if (camera) {
        float cameraRightX = camera->getView().getCenter().x + Constants::WINDOW_WIDTH * 0.5f;
        int cameraRightTile = static_cast<int>(std::floor(cameraRightX / Constants::TILE_SIZE)) + 10;
        renderWidth = std::max(renderWidth, cameraRightTile);
    }
    int height = tileMap.getHeight();
    float size = Constants::TILE_SIZE;

    sf::Color gridColor(255, 255, 255, 45); // Soft semi-transparent white lines
    sf::Color outOfBoundsGridColor(0, 200, 255, 60); // Cyan grid lines for out-of-bounds auto-expansion area

    // Vertical lines (extending to out-of-grid sight area)
    for (int x = 0; x <= renderWidth; ++x) {
        sf::Color col = (x <= tileMap.getWidth()) ? gridColor : outOfBoundsGridColor;
        sf::Vertex line[] = {
            sf::Vertex{sf::Vector2f(x * size, 0.0f), col},
            sf::Vertex{sf::Vector2f(x * size, height * size), col}
        };
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }

    // Horizontal lines
    for (int y = 0; y <= height; ++y) {
        sf::Vertex line[] = {
            sf::Vertex{sf::Vector2f(0.0f, y * size), gridColor},
            sf::Vertex{sf::Vector2f(renderWidth * size, y * size), gridColor}
        };
        target.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void MapEditor::renderImGui(TileMap& tileMap, std::vector<std::unique_ptr<Entity>>& entities) {
    if (!m_active) return;

    // Right-hand column, clear of the navigation and generator panels on the
    // left. With no position of its own this window opened at ImGui's default
    // spot and sat underneath "Gameplay Controls & Navigation".
    ImGui::SetNextWindowPos(ImVec2(912.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 600.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mario Maker - In-Game Level Editor (F1)");

    ImGui::Checkbox("Show Grid Overlay Layout", &m_showGrid);
    ImGui::Separator();

    // Mode Selector
    if (ImGui::RadioButton("Tile Palette", m_isTileMode)) {
        m_isTileMode = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Entity Palette", !m_isTileMode)) {
        m_isTileMode = false;
    }

    ImGui::Separator();

    if (m_isTileMode) {
        ImGui::Text("Select Tile Type to Place:");
        const char* tileNames[] = { "Empty (Erase)", "Ground", "Brick", "Question Block", "Pipe", "Ice Block", "Conveyor Belt", "Water", "Coin" };
        TileType tileTypes[] = { TileType::Empty, TileType::Ground, TileType::Brick, TileType::Question, TileType::Pipe, TileType::Ice, TileType::Conveyor, TileType::Water, TileType::Coin };
        
        int activeIdx = 1;
        for (int i = 0; i < 9; ++i) {
            if (m_selectedTileType == tileTypes[i]) activeIdx = i;
        }

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##TileBrush", &activeIdx, tileNames, 9)) {
            m_selectedTileType = tileTypes[activeIdx];
        }
    } else {
        ImGui::Text("Select Entity Type to Place:");
        const char* entityKeys[] = { "mario", "luigi", "toad", "peach", "mushroom", "oneup_mushroom", "mini_mushroom", "mega_mushroom", "cape_feather", "fire_flower", "star", "coin", "star_coin", "pswitch", "pow_block", "trampoline" };
        const char* entityLabels[] = { "Player: Mario", "Player: Luigi", "Player: Toad", "Player: Peach", "Item: Mushroom", "Item: 1-Up Mushroom", "Item: Mini Mushroom", "Item: Mega Mushroom", "Item: Cape Feather", "Item: Fire Flower", "Item: Star (Invincible)", "Item: Coin", "Item: Star Coin", "Item: P-Switch", "Item: POW Block", "Item: Trampoline" };
        
        int activeIdx = 11; // Defaults to coin
        for (int i = 0; i < 16; ++i) {
            if (m_selectedEntityType == entityKeys[i]) activeIdx = i;
        }

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##EntityBrush", &activeIdx, entityLabels, 16)) {
            m_selectedEntityType = entityKeys[activeIdx];
        }
    }

    ImGui::Separator();

    // History controls
    if (ImGui::Button("Undo")) {
        undo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) {
        redo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear History")) {
        clearHistory();
    }

    ImGui::Separator();

    // File Import / Export
    ImGui::Text("Map File Name (saves/<name>.json):");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##MapFileName", m_levelNameInput, sizeof(m_levelNameInput));
    
    if (ImGui::Button("Export Level JSON")) {
        saveLevel(tileMap, entities);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Level JSON")) {
        loadLevel(tileMap, entities);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset / Clear Entire Map")) {
        tileMap.initialize(tileMap.getWidth(), tileMap.getHeight());
        entities.clear();
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

    ImGui::TextDisabled("\nViewport Controls:\n"
                        "- Left Click: Place Tile/Entity\n"
                        "- Right Click: Erase Tile/Entity");

    ImGui::End();
}
