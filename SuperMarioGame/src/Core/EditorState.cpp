#include "Core/EditorState.hpp"

#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/ResourceManager.hpp"
#include "Entities/Entity.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/LevelLoader.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace {

const char* const kThemeNames[] = {"overworld", "underground", "castle", "ice"};
constexpr int kThemeCount = 4;

int themeIndexOf(const std::string& theme) {
    for (int i = 0; i < kThemeCount; ++i) {
        if (theme == kThemeNames[i]) return i;
    }
    return 0;
}

void copyInto(char* buffer, std::size_t size, const std::string& text) {
    std::snprintf(buffer, size, "%s", text.c_str());
}

} // namespace

EditorState::EditorState(std::string openPath)
    : m_pendingOpenPath(std::move(openPath)) {}

EditorState::~EditorState() = default;

sf::FloatRect EditorState::canvasRect() const {
    return sf::FloatRect(
        {LEFT_W, MENU_BAR_H},
        {static_cast<float>(Constants::WINDOW_WIDTH) - LEFT_W - RIGHT_W,
         static_cast<float>(Constants::WINDOW_HEIGHT) - MENU_BAR_H - BOTTOM_H});
}

void EditorState::enter() {
    std::cout << "Entering EditorState" << std::endl;

    m_playerSheet  = SpriteSheet::loadAtlas("player");
    m_enemySheet   = SpriteSheet::loadAtlas("enemy_projectile");
    m_itemSheet    = SpriteSheet::loadAtlas("item");
    m_scenerySheet = SpriteSheet::loadAtlas("world_scenery_item");

    m_artBinder.setSheets(m_playerSheet.get(), m_enemySheet.get(),
                          m_itemSheet.get(), m_scenerySheet.get());
    m_background.setSpriteSheet(m_scenerySheet.get());
    m_tileMapRenderer.setSpriteSheet(m_scenerySheet.get());

    // The world is drawn into the canvas rectangle rather than the whole window,
    // so the panels do not cover a level the author is trying to see. Setting
    // both size and viewport keeps one world pixel equal to one screen pixel;
    // leaving the size at 1280x720 would squeeze the level into the smaller rect
    // and every "click that tile" would land somewhere else.
    const sf::FloatRect canvas = canvasRect();
    sf::View& view = m_camera.getView();
    view.setSize(canvas.size);
    view.setViewport(sf::FloatRect(
        {canvas.position.x / static_cast<float>(Constants::WINDOW_WIDTH),
         canvas.position.y / static_cast<float>(Constants::WINDOW_HEIGHT)},
        {canvas.size.x / static_cast<float>(Constants::WINDOW_WIDTH),
         canvas.size.y / static_cast<float>(Constants::WINDOW_HEIGHT)}));

    m_mapEditor.setEntityAdmitter(&m_bridge);
    m_mapEditor.setViewportRect(canvas);
    m_mapEditor.setActive(true);

    // Custom levels are re-scanned whenever the editor opens: a level saved in a
    // previous session must be in the Open dialog without a restart.
    LevelCatalog::refreshCustomLevels();

    if (!m_pendingOpenPath.empty()) {
        openLevel(m_pendingOpenPath);
        m_pendingOpenPath.clear();
    } else {
        newLevel(m_newWidth, m_newHeight, kThemeNames[0], "Untitled Level");
        setStatus("New blank level. File > Open to edit an existing map.");
    }
}

void EditorState::exit() {
    m_mapEditor.setEntityAdmitter(nullptr);
    m_mapEditor.clearSelection();
    m_entities.clear();
}

void EditorState::admitEntity(Entity* entity) {
    // No difficulty scaling here, unlike PlayingState::admitEntity: the editor
    // shows the level as authored, not as the current difficulty would play it.
    // A Goomba drawn at Hard speed would be the same Goomba on disk.
    m_artBinder.bind(entity);
    m_document.dirty = true;
}

void EditorState::forgetEntity(Entity* entity) {
    if (m_mapEditor.selectedEntity() == entity) m_mapEditor.clearSelection();
    m_document.dirty = true;
}

void EditorState::showLevelOrigin() {
    // The level's bottom-left corner, filling the canvas.
    //
    // Column 0 is at the canvas's left edge, so "x pixels right of the canvas
    // edge" is "x pixels into the level" and clicks land where they look. The
    // camera cannot be clamped to do this — MapEditor turns bounds off every
    // frame so the author can pan past the edge to extend the map — so a camera
    // centred on the spawn point instead put a third of the canvas into NEGATIVE
    // world space, where every click was silently discarded.
    //
    // The BOTTOM rather than the top because a level is 22-23 tiles tall and the
    // canvas holds 16: framed on row 0 the author opens World 1-1 and sees
    // nothing but sky, with the ground and every entity below the viewport.
    const sf::FloatRect canvas = canvasRect();
    const float levelHeightPx = m_tileMap.getHeight() * Constants::TILE_SIZE;
    m_camera.setBoundsEnabled(false);
    m_camera.snapTo({canvas.size.x * 0.5f,
                     std::max(canvas.size.y * 0.5f, levelHeightPx - canvas.size.y * 0.5f)});
}

void EditorState::syncCameraBounds() {
    m_camera.setBounds(AABB{0.0f, 0.0f,
                            m_tileMap.getWidth() * Constants::TILE_SIZE,
                            m_tileMap.getHeight() * Constants::TILE_SIZE});
}

void EditorState::setStatus(std::string message) {
    m_status = std::move(message);
    m_statusTimer = 8.0f;
    std::cout << "[Editor] " << m_status << std::endl;
}

void EditorState::newLevel(int width, int height, const std::string& theme,
                           const std::string& name) {
    for (auto& entity : m_entities) {
        if (entity) forgetEntity(entity.get());
    }
    m_entities.clear();
    m_tileMap.initialize(std::max(8, width), std::max(8, height));

    m_document = Document{};
    m_document.name = name;
    m_document.theme = theme;
    m_document.dirty = false;

    m_background.setTheme(theme);
    m_tileMapRenderer.setTheme(m_background.getTheme());
    m_mapEditor.clearHistory();
    m_mapEditor.clearSelection();
    m_mapEditor.setSpawnPoint({2.0f * Constants::TILE_SIZE,
                               std::max(0.0f, (height - 4.0f)) * Constants::TILE_SIZE});
    syncCameraBounds();
    showLevelOrigin();

    copyInto(m_nameBuffer, sizeof(m_nameBuffer), m_document.name);
    copyInto(m_saveAsBuffer, sizeof(m_saveAsBuffer), LevelCatalog::toFileStem(m_document.name));
    m_newThemeIndex = themeIndexOf(theme);
}

bool EditorState::openLevel(const std::string& path) {
    LevelLoader loader;
    LevelData data;
    TileMap loaded;
    if (!loader.loadLevel(path, loaded, data)) {
        setStatus("Could not open " + path);
        return false;
    }

    for (auto& entity : m_entities) {
        if (entity) forgetEntity(entity.get());
    }
    m_entities = std::move(data.entities);
    m_tileMap = std::move(loaded);
    for (auto& entity : m_entities) {
        if (entity) m_artBinder.bind(entity.get());
    }

    m_document.path = ResourceManager::resolvePath(path);
    m_document.name = data.name;
    m_document.theme = data.theme;
    m_document.builtIn = LevelCatalog::isBuiltIn(path);
    m_document.dirty = false;

    m_background.setTheme(data.theme);
    m_tileMapRenderer.setTheme(m_background.getTheme());
    m_mapEditor.clearHistory();
    m_mapEditor.clearSelection();
    m_mapEditor.setSpawnPoint(data.spawnPoint);
    syncCameraBounds();
    showLevelOrigin();

    copyInto(m_nameBuffer, sizeof(m_nameBuffer), m_document.name);
    copyInto(m_saveAsBuffer, sizeof(m_saveAsBuffer),
             m_document.builtIn
                 // A copy by default, so the obvious next keystroke cannot
                 // overwrite a level that ships with the game.
                 ? LevelCatalog::toFileStem(m_document.name) + "_copy"
                 : LevelCatalog::toFileStem(m_document.name));
    m_newThemeIndex = themeIndexOf(m_document.theme);

    setStatus((m_document.builtIn ? "Opened BUILT-IN level " : "Opened custom level ") +
              m_document.path);
    return true;
}

bool EditorState::saveTo(const std::string& path) {
    if (path.empty()) {
        setStatus("No writable location for the level file.");
        return false;
    }

    LevelData meta;
    meta.name = m_document.name;
    meta.theme = m_document.theme;
    meta.spawnPoint = m_mapEditor.spawnPoint();

    LevelLoader loader;
    if (!loader.saveLevel(path, m_tileMap, m_entities, meta)) {
        setStatus("FAILED to write " + path);
        return false;
    }

    std::error_code ec;
    // Absolute, because "assets/levels/custom/x.json" is only an answer if you
    // already know which directory the game was launched from — and that
    // ambiguity is half of what "where is it" meant.
    const std::string absolute = std::filesystem::absolute(path, ec).string();
    m_lastSavedPath = ec ? path : absolute;

    m_document.path = path;
    m_document.builtIn = LevelCatalog::isBuiltIn(path);
    m_document.dirty = false;
    LevelCatalog::refreshCustomLevels();
    setStatus("Saved to " + m_lastSavedPath);
    return true;
}

void EditorState::save() {
    if (m_document.path.empty() || m_document.builtIn) {
        // Never silently over a shipped level: the whole campaign is four files
        // and one careless Ctrl+S would replace one of them with a work in
        // progress. Save As is offered instead, pre-filled with a copy name.
        m_showSaveAs = true;
        if (m_document.builtIn) m_showBuiltInWarning = true;
        return;
    }
    saveTo(m_document.path);
}

void EditorState::saveAsCurrentName() {
    m_document.name = m_nameBuffer;
    saveTo(LevelCatalog::customPathFor(m_saveAsBuffer));
}

void EditorState::playtest() {
    // Round-tripped through a scratch file rather than handed over in memory:
    // playtesting the file is what actually proves the level will play when it
    // is loaded again, and a level that plays in the editor and not from disk is
    // the bug this whole change exists to close. The name starts with "__" so
    // refreshCustomLevels() keeps it out of the author's own list.
    const std::string dir = LevelCatalog::resolvedCustomDirectory();
    if (dir.empty()) {
        setStatus("No writable custom level directory; cannot playtest.");
        return;
    }
    const std::string scratch = (std::filesystem::path(dir) / "__playtest.json").string();

    LevelData meta;
    meta.name = m_document.name;
    meta.theme = m_document.theme;
    meta.spawnPoint = m_mapEditor.spawnPoint();

    LevelLoader loader;
    if (!loader.saveLevel(scratch, m_tileMap, m_entities, meta)) {
        setStatus("Could not write the playtest file.");
        return;
    }

    setStatus("Playtesting. Esc > QUIT returns here.");
    Game::getInstance().pushState(std::make_unique<PlayingState>(
        /*startInEditor=*/false, /*isProcedural=*/false, MapGeneratorConfig(),
        /*characterIndex=*/0, /*levelIndex=*/0, MatchConfig{}, /*isEndless=*/false,
        /*pendingLoadSlot=*/0, /*isAttractDemo=*/false, scratch, /*isPlaytest=*/true));
}

void EditorState::leaveToMenu() {
    // Two presses when there is unsaved work. Not a modal: the editor is
    // keyboard-driven and a level lost to a stray Escape is an hour lost.
    if (m_document.dirty && !m_leaveArmed) {
        m_leaveArmed = true;
        setStatus("Unsaved changes. Ctrl+S to save, or press Esc again to discard.");
        return;
    }
    Game::getInstance().changeState(std::make_unique<MenuState>());
}

bool EditorState::handleDialogKey(sf::Keyboard::Key key) {
    const bool anyDialog = m_openDialogUp || m_newDialogUp || m_saveAsDialogUp;
    if (!anyDialog) return false;

    if (key == sf::Keyboard::Key::Escape) { m_dialogCancel = true; return true; }
    if (key == sf::Keyboard::Key::Enter)  { m_dialogConfirm = true; return true; }

    if (m_openDialogUp) {
        const int rows = LevelCatalog::count() +
                         static_cast<int>(LevelCatalog::customLevels().size());
        if (rows > 0) {
            if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S) {
                m_openSelected = (m_openSelected + 1) % rows;
                return true;
            }
            if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W) {
                m_openSelected = (m_openSelected - 1 + rows) % rows;
                return true;
            }
        }
    }
    // Everything else is swallowed: a tool shortcut typed at a modal would edit
    // the level behind it.
    return true;
}

void EditorState::openSelectedFromDialog() {
    const int builtIns = LevelCatalog::count();
    if (m_openSelected < builtIns) {
        openLevel(LevelCatalog::pathFor(m_openSelected));
        return;
    }
    const auto& custom = LevelCatalog::customLevels();
    const std::size_t index = static_cast<std::size_t>(m_openSelected - builtIns);
    if (index < custom.size()) openLevel(custom[index].path);
}

void EditorState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    // Before the WantCaptureKeyboard guard: a modal's own text field owns the
    // ImGui keyboard, so a dialog that waited for that guard could never be
    // confirmed or dismissed from the keyboard at all.
    if (handleDialogKey(keyPressed->code)) return;

    // A text field owns the keyboard while it is being typed into: Ctrl+Z in
    // the level-name box means "undo my typing", not "undo my last brush
    // stroke". Undo and redo were ImGui buttons only before this — Ctrl+Z did
    // nothing at all in the editor.
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    const bool ctrl = keyPressed->control || keyPressed->system;
    const bool shift = keyPressed->shift;
    // Anything other than a second Escape cancels the pending discard.
    if (keyPressed->code != sf::Keyboard::Key::Escape) m_leaveArmed = false;

    if (ctrl && keyPressed->code == sf::Keyboard::Key::Z) {
        if (shift) m_mapEditor.redo();
        else m_mapEditor.undo();
        m_document.dirty = true;
        return;
    }
    if (ctrl && keyPressed->code == sf::Keyboard::Key::Y) {
        m_mapEditor.redo();
        m_document.dirty = true;
        return;
    }
    if (ctrl && keyPressed->code == sf::Keyboard::Key::S) {
        if (shift) m_showSaveAs = true;
        else save();
        return;
    }
    if (ctrl && keyPressed->code == sf::Keyboard::Key::O) {
        LevelCatalog::refreshCustomLevels();
        m_showOpen = true;
        return;
    }
    if (ctrl && keyPressed->code == sf::Keyboard::Key::N) {
        m_showNew = true;
        return;
    }

    switch (keyPressed->code) {
        case sf::Keyboard::Key::Delete:
        case sf::Keyboard::Key::Backspace:
            m_mapEditor.deleteSelection(m_entities);
            break;
        // Single-key tool shortcuts, in the order the tool row shows them.
        case sf::Keyboard::Key::B: m_mapEditor.setTool(MapEditor::Tool::Paint); break;
        case sf::Keyboard::Key::E: m_mapEditor.setTool(MapEditor::Tool::Erase); break;
        case sf::Keyboard::Key::R: m_mapEditor.setTool(MapEditor::Tool::RectFill); break;
        case sf::Keyboard::Key::I: m_mapEditor.setTool(MapEditor::Tool::Eyedropper); break;
        case sf::Keyboard::Key::V: m_mapEditor.setTool(MapEditor::Tool::Select); break;
        case sf::Keyboard::Key::P: m_mapEditor.setTool(MapEditor::Tool::SpawnPoint); break;
        // Palette stepping, so every brush is reachable without the mouse.
        case sf::Keyboard::Key::Q: m_mapEditor.cyclePalette(-1); break;
        case sf::Keyboard::Key::T: m_mapEditor.cyclePalette(1); break;
        case sf::Keyboard::Key::F5: playtest(); break;
        case sf::Keyboard::Key::Escape:
            leaveToMenu();
            break;
        default:
            break;
    }
}

void EditorState::update(float dt) {
    if (m_suspended) return;

    m_animTimer += dt;
    if (m_statusTimer > 0.0f) m_statusTimer -= dt;

    const std::size_t depthBefore = m_mapEditor.historyDepth();

    m_mapEditor.update(m_tileMap, m_entities,
                       Game::getInstance().getMouseWorldPosition(m_camera.getView()),
                       Game::getInstance().getMousePixelPosition(), dt, &m_camera);
    if (m_mapEditor.historyDepth() != depthBefore) m_document.dirty = true;

    m_camera.update(dt);
    m_background.update(dt);
}

void EditorState::render(sf::RenderTarget& target) {
    // Backdrop in screen space, clipped to the canvas by the same viewport the
    // world uses — otherwise the sky paints over the panels.
    const sf::View screenView = target.getView();
    sf::View canvasScreenView = m_camera.getView();
    canvasScreenView.setCenter(canvasScreenView.getSize() * 0.5f);
    target.setView(canvasScreenView);
    m_background.render(target, m_camera.getVisibleBounds());

    target.setView(m_camera.getView());
    m_tileMapRenderer.setSpriteSheet(m_scenerySheet.get());
    m_tileMapRenderer.setTheme(m_background.getTheme());
    m_tileMapRenderer.render(target, m_tileMap, m_camera.getVisibleBounds(), m_animTimer);

    for (auto& entity : m_entities) {
        if (entity) entity->render(target);
    }

    m_mapEditor.render(target, m_tileMap, m_entities, &m_camera);

    target.setView(screenView);

    // ImGui from render() only. Game::run() updates the state machine BEFORE
    // ImGui::SFML::Update(), so a window built from update() would be issued
    // against the previous frame's ImGui context.
    renderMenuBar();
    renderLeftPanel();
    renderRightPanel();
    renderBottomPanel();
    renderDialogs();
}

void EditorState::onSuspend() {
    m_suspended = true;
}

void EditorState::onResume() {
    m_suspended = false;
    // A playtest may have written the scratch file and a Save may have happened
    // in between; either way the list on screen must be current.
    LevelCatalog::refreshCustomLevels();
}

// --- ImGui panels ---------------------------------------------------------

void EditorState::renderMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Level...", "Ctrl+N")) m_showNew = true;
        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
            LevelCatalog::refreshCustomLevels();
            m_showOpen = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", "Ctrl+S")) save();
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) m_showSaveAs = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Playtest", "F5")) playtest();
        ImGui::Separator();
        if (ImGui::MenuItem("Back to Main Menu", "Esc")) leaveToMenu();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_mapEditor.canUndo())) m_mapEditor.undo();
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, m_mapEditor.canRedo())) m_mapEditor.redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Delete selected", "Del", false,
                            m_mapEditor.selectedEntity() != nullptr)) {
            m_mapEditor.deleteSelection(m_entities);
        }
        ImGui::EndMenu();
    }

    // The title bar answers "which map am I editing" at all times, which is the
    // question that had no answer at all before: the editor opened on a silently
    // loaded copy of World 1-1 and said nothing.
    ImGui::Separator();
    if (m_document.builtIn) {
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f), "BUILT-IN:");
    } else if (m_document.path.empty()) {
        ImGui::TextColored(ImVec4(0.65f, 0.85f, 1.0f, 1.0f), "NEW:");
    } else {
        ImGui::TextColored(ImVec4(0.55f, 0.95f, 0.65f, 1.0f), "CUSTOM:");
    }
    ImGui::Text("%s%s", m_document.name.c_str(), m_document.dirty ? " *" : "");
    if (!m_document.path.empty()) {
        ImGui::TextDisabled("(%s)", m_document.path.c_str());
    } else {
        ImGui::TextDisabled("(never saved)");
    }

    ImGui::EndMainMenuBar();
}

void EditorState::renderLeftPanel() {
    const float height = static_cast<float>(Constants::WINDOW_HEIGHT) - MENU_BAR_H;
    ImGui::SetNextWindowPos(ImVec2(0.0f, MENU_BAR_H), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(LEFT_W, height), ImGuiCond_Always);
    ImGui::Begin("Tools & Palette", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    m_mapEditor.renderToolPanel();
    ImGui::Separator();
    m_mapEditor.renderPalettePanel();

    ImGui::End();
}

void EditorState::renderRightPanel() {
    const float x = static_cast<float>(Constants::WINDOW_WIDTH) - RIGHT_W;
    const float height = static_cast<float>(Constants::WINDOW_HEIGHT) - MENU_BAR_H;
    ImGui::SetNextWindowPos(ImVec2(x, MENU_BAR_H), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(RIGHT_W, height), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    m_mapEditor.renderInspectorPanel(m_entities);

    ImGui::End();
}

void EditorState::renderBottomPanel() {
    const float y = static_cast<float>(Constants::WINDOW_HEIGHT) - BOTTOM_H;
    const float width = static_cast<float>(Constants::WINDOW_WIDTH) - LEFT_W - RIGHT_W;
    ImGui::SetNextWindowPos(ImVec2(LEFT_W, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, BOTTOM_H), ImGuiCond_Always);
    ImGui::Begin("Level", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (ImGui::BeginTabBar("##BottomTabs")) {
        if (ImGui::BeginTabItem("Properties")) {
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::InputText("Level name", m_nameBuffer, sizeof(m_nameBuffer))) {
                m_document.name = m_nameBuffer;
                m_document.dirty = true;
            }

            int themeIndex = themeIndexOf(m_document.theme);
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::Combo("Theme", &themeIndex, kThemeNames, kThemeCount)) {
                m_document.theme = kThemeNames[themeIndex];
                m_background.setTheme(m_document.theme);
                m_tileMapRenderer.setTheme(m_background.getTheme());
                m_document.dirty = true;
            }

            int width = m_tileMap.getWidth();
            int height = m_tileMap.getHeight();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputInt("Width (tiles)", &width)) {
                // expandToFit only grows; shrinking would throw away tiles the
                // author cannot get back with undo, since this is not a command.
                m_tileMap.expandToFit(std::max(width, m_tileMap.getWidth()), m_tileMap.getHeight());
                syncCameraBounds();
                m_document.dirty = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputInt("Height (tiles)", &height)) {
                m_tileMap.expandToFit(m_tileMap.getWidth(), std::max(height, m_tileMap.getHeight()));
                syncCameraBounds();
                m_document.dirty = true;
            }

            const sf::Vector2f spawn = m_mapEditor.spawnPoint();
            ImGui::Text("Spawn point: tile (%d, %d)",
                        static_cast<int>(spawn.x / Constants::TILE_SIZE),
                        static_cast<int>(spawn.y / Constants::TILE_SIZE));
            ImGui::SameLine();
            if (ImGui::SmallButton("Set with tool")) {
                m_mapEditor.setTool(MapEditor::Tool::SpawnPoint);
            }
            ImGui::Text("%d entities placed", static_cast<int>(m_entities.size()));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("History")) {
            m_mapEditor.renderHistoryPanel();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Where is my level?")) {
            // The literal answer to the user's second message, kept on a tab of
            // its own so it is never more than one click away.
            ImGui::TextWrapped("Custom levels are written to:");
            ImGui::TextWrapped("%s", LevelCatalog::resolvedCustomDirectory().c_str());
            ImGui::Separator();
            if (m_lastSavedPath.empty()) {
                ImGui::TextDisabled("Nothing saved yet this session.");
            } else {
                ImGui::TextWrapped("Last save: %s", m_lastSavedPath.c_str());
            }
            ImGui::Separator();
            ImGui::TextWrapped("To play it: Main Menu > CUSTOM LEVELS, or press "
                               "F5 here to playtest without leaving the editor.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (m_statusTimer > 0.0f && !m_status.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", m_status.c_str());
    }

    ImGui::End();
}

void EditorState::renderDialogs() {
    if (m_showOpen) {
        ImGui::OpenPopup("Open Level");
        m_openSelected = 0;
        m_showOpen = false;
    }
    if (m_showNew) {
        ImGui::OpenPopup("New Level");
        m_showNew = false;
    }
    if (m_showSaveAs) {
        ImGui::OpenPopup("Save Level As");
        m_showSaveAs = false;
    }

    ImGui::SetNextWindowSize(ImVec2(620.0f, 430.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Open Level", nullptr, ImGuiWindowFlags_NoResize)) {
        m_openDialogUp = true;
        if (m_dialogCancel) {
            m_dialogCancel = false;
            m_openDialogUp = false;
            ImGui::CloseCurrentPopup();
        } else if (m_dialogConfirm) {
            m_dialogConfirm = false;
            m_openDialogUp = false;
            openSelectedFromDialog();
            ImGui::CloseCurrentPopup();
        } else {
            ImGui::TextWrapped("Up/Down to choose, Enter to open, Esc to cancel.");
            ImGui::Separator();
            ImGui::TextWrapped("BUILT-IN - these are the game's REAL maps. Editing one "
                               "is allowed; Save will offer Save As rather than "
                               "overwriting it.");
            ImGui::BeginChild("##BuiltIn", ImVec2(0.0f, 120.0f), true);
            for (int i = 0; i < LevelCatalog::count(); ++i) {
                const bool selected = (m_openSelected == i);
                const std::string label = LevelCatalog::longNameFor(i) + "   [" +
                                          LevelCatalog::pathFor(i) + "]";
                if (ImGui::Selectable(label.c_str(), selected)) {
                    m_openSelected = i;
                    openSelectedFromDialog();
                    m_openDialogUp = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextWrapped("YOUR LEVELS, from %s:",
                               LevelCatalog::resolvedCustomDirectory().c_str());
            ImGui::BeginChild("##Custom", ImVec2(0.0f, 140.0f), true);
            const auto& custom = LevelCatalog::customLevels();
            if (custom.empty()) {
                ImGui::TextDisabled("None yet. File > Save As writes one here.");
            }
            for (std::size_t i = 0; i < custom.size(); ++i) {
                const int row = LevelCatalog::count() + static_cast<int>(i);
                if (ImGui::Selectable(custom[i].longName.c_str(), m_openSelected == row)) {
                    m_openSelected = row;
                    openSelectedFromDialog();
                    m_openDialogUp = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Rescan")) LevelCatalog::refreshCustomLevels();
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                m_openDialogUp = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    } else {
        m_openDialogUp = false;
    }

    if (ImGui::BeginPopupModal("New Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        m_newDialogUp = true;
        if (m_dialogCancel) {
            m_dialogCancel = false;
            m_newDialogUp = false;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        if (m_dialogConfirm) {
            m_dialogConfirm = false;
            m_newDialogUp = false;
            newLevel(m_newWidth, m_newHeight, kThemeNames[m_newThemeIndex], m_nameBuffer);
            setStatus("New level created. It has no file until you Save As.");
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        ImGui::InputText("Name", m_nameBuffer, sizeof(m_nameBuffer));
        ImGui::InputInt("Width (tiles)", &m_newWidth);
        ImGui::InputInt("Height (tiles)", &m_newHeight);
        ImGui::Combo("Theme", &m_newThemeIndex, kThemeNames, kThemeCount);
        ImGui::TextDisabled("A blank canvas - no tiles, no entities.");
        ImGui::TextDisabled("Enter creates, Esc cancels.");
        if (ImGui::Button("Create")) {
            newLevel(m_newWidth, m_newHeight, kThemeNames[m_newThemeIndex], m_nameBuffer);
            setStatus("New level created. It has no file until you Save As.");
            m_newDialogUp = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_newDialogUp = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    } else {
        m_newDialogUp = false;
    }

    if (ImGui::BeginPopupModal("Save Level As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        m_saveAsDialogUp = true;
        if (m_dialogCancel) {
            m_dialogCancel = false;
            m_saveAsDialogUp = false;
            m_showBuiltInWarning = false;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        if (m_dialogConfirm) {
            m_dialogConfirm = false;
            m_saveAsDialogUp = false;
            m_showBuiltInWarning = false;
            saveAsCurrentName();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        if (m_showBuiltInWarning) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                               "%s ships with the game and will not be overwritten.",
                               m_document.name.c_str());
            ImGui::TextWrapped("Your edits are saved as a copy under "
                               "assets/levels/custom/ instead.");
            ImGui::Separator();
        }
        ImGui::InputText("Level name", m_nameBuffer, sizeof(m_nameBuffer));
        ImGui::InputText("File name", m_saveAsBuffer, sizeof(m_saveAsBuffer));
        // The exact path, before the save rather than after: guessing where a
        // file went is the complaint this answers.
        // The absolute path, before the save rather than after: guessing where a
        // file went is the complaint this answers.
        ImGui::TextWrapped("Will write: %s",
                           LevelCatalog::customPathFor(m_saveAsBuffer).c_str());
        ImGui::TextDisabled("Enter saves, Esc cancels.");
        if (ImGui::Button("Save")) {
            m_showBuiltInWarning = false;
            m_saveAsDialogUp = false;
            saveAsCurrentName();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_showBuiltInWarning = false;
            m_saveAsDialogUp = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    } else {
        m_saveAsDialogUp = false;
    }
}
