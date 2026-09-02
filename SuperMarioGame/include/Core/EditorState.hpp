#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/EntityArtBinder.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/TileMapRenderer.hpp"
#include "Utils/IEntityAdmitter.hpp"
#include "Utils/MapEditor.hpp"
#include "Utils/TileMap.hpp"

#include <memory>
#include <string>
#include <vector>

class Entity;

// The level editor as a screen of its own.
//
// It used to be one ImGui window floating over a PlayingState that had quietly
// loaded World 1-1 — MenuState's "MAP EDITOR" row constructed
// PlayingState(true, false), so the editor opened on top of a live game with a
// live player in it and no indication that the level on screen was a real
// campaign map. That arrangement caused the two things the user reported: there
// was no way to tell what you were editing, and there was no route from an
// authored level back into a game.
//
// EditorState owns a level instead of borrowing one: its own TileMap, entity
// vector, camera, atlases and backdrop, plus the MapEditor that does the actual
// editing. It is a full-screen state (isOverlay() is false) reached from the
// same menu row, and it pushes a PlayingState to playtest.
//
// ImGui ordering constraint: Game::run() calls GameStateManager::update()
// BEFORE ImGui::SFML::Update(), and builds windows during render(). Every
// ImGui:: call in this class is therefore issued from render(); update() only
// moves the camera and drives MapEditor's pointer handling.
class EditorState : public IGameState {
public:
    // `openPath` is a level file to open on entry — a campaign level, or one of
    // the author's own. Empty starts a blank canvas.
    explicit EditorState(std::string openPath = std::string());
    ~EditorState() override;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void onSuspend() override;
    void onResume() override;

private:
    // The panel layout, in window pixels. Fixed rather than docked: this build
    // of ImGui (1.91.8) is the non-docking branch, so a dockspace is not
    // available — and a fixed layout gives the one thing the dockspace was
    // wanted for anyway, an explicit canvas rectangle that mouse handling can
    // test against instead of guessing from a frame-stale WantCaptureMouse.
    static constexpr float MENU_BAR_H = 24.0f;
    static constexpr float LEFT_W = 300.0f;
    static constexpr float RIGHT_W = 300.0f;
    static constexpr float BOTTOM_H = 160.0f;

    // What is open in the editor, and where it came from.
    //
    // The user's complaint was exactly this: "we are not able to know that we
    // are editing our real maps". Every field here exists to be shown.
    struct Document {
        // Resolved filesystem path, or empty for a level that has never been
        // saved. Shown verbatim in the chrome after every save.
        std::string path;
        std::string name = "Untitled Level";
        std::string theme = "overworld";
        // True when `path` is one of the files that ship with the game. Save is
        // refused on one of these without an explicit confirmation.
        bool builtIn = false;
        bool dirty = false;
    };

    // MapEditor mutates m_entities directly and needs a way back into this
    // state's bookkeeping — see IEntityAdmitter. A nested adapter, so
    // admitEntity()/forgetEntity() stay private.
    class EditorBridge : public IEntityAdmitter {
    public:
        explicit EditorBridge(EditorState& owner) : m_owner(owner) {}
        void admit(Entity* entity) override { m_owner.admitEntity(entity); }
        void release(Entity* entity) override { m_owner.forgetEntity(entity); }
    private:
        EditorState& m_owner;
    };

    void admitEntity(Entity* entity);
    void forgetEntity(Entity* entity);

    // --- Document operations ---------------------------------------------
    void newLevel(int width, int height, const std::string& theme, const std::string& name);
    bool openLevel(const std::string& path);
    // Writes to `path`. Returns false and leaves the document alone on failure.
    bool saveTo(const std::string& path);
    // Save over the open file, or fall through to Save As when there is none or
    // when the open file ships with the game.
    void save();
    void saveAsCurrentName();
    // Save to a scratch file and push a PlayingState on it.
    void playtest();
    // Back to the main menu, asking once when the level has unsaved edits.
    void leaveToMenu();

    void syncCameraBounds();
    // Frame world (0,0) at the canvas corner. See the definition for why the
    // spawn point is the wrong thing to centre on here.
    void showLevelOrigin();
    void setStatus(std::string message);

    // --- ImGui, all issued from render() ---------------------------------
    void renderMenuBar();
    void renderLeftPanel();
    void renderRightPanel();
    void renderBottomPanel();
    void renderDialogs();

    sf::FloatRect canvasRect() const;

    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;
    Camera m_camera;
    MapEditor m_mapEditor;
    EditorBridge m_bridge{*this};

    std::unique_ptr<SpriteSheet> m_playerSheet;
    std::unique_ptr<SpriteSheet> m_enemySheet;
    std::unique_ptr<SpriteSheet> m_itemSheet;
    std::unique_ptr<SpriteSheet> m_scenerySheet;

    BackgroundRenderer m_background;
    TileMapRenderer m_tileMapRenderer;
    EntityArtBinder m_artBinder;

    Document m_document;
    // Path handed to the constructor, opened by enter(). enter() is where it
    // happens because the atlases have to exist first or the level opens with
    // every entity unbound.
    std::string m_pendingOpenPath;

    float m_animTimer = 0.0f;
    bool m_suspended = false;

    // Dialog visibility. ImGui popups are keyed by name and opened from the
    // frame that requests them, so the request has to survive as a flag.
    bool m_showOpen = false;
    bool m_showNew = false;
    bool m_showSaveAs = false;
    bool m_showBuiltInWarning = false;

    // Dialogs are keyboard-operable, like every other screen in this game.
    //
    // Not a nicety: scripted events reach the state stack but never ImGui (see
    // Game::run), so a modal that only an ImGui button can dismiss cannot be
    // driven by a --script verification run at all — and an editor whose Save
    // could not be verified by running the game is exactly what directive 9
    // forbids reporting as complete. handleInput() acts on these before its
    // WantCaptureKeyboard guard; renderDialogs() consumes them.
    bool m_openDialogUp = false;
    bool m_newDialogUp = false;
    bool m_saveAsDialogUp = false;
    bool m_dialogConfirm = false;
    bool m_dialogCancel = false;
    // Row the Open dialog's keyboard cursor is on: built-in levels first, then
    // the author's own.
    int m_openSelected = 0;

    // Handled a key on behalf of an open dialog. Returns true when the key was
    // consumed and must not also reach the tools.
    bool handleDialogKey(sf::Keyboard::Key key);
    // Open the level the Open dialog's cursor is on.
    void openSelectedFromDialog();
    // One Escape on a dirty level warns; a second one discards.
    bool m_leaveArmed = false;

    char m_nameBuffer[64] = "Untitled Level";
    char m_saveAsBuffer[64] = "my_level";
    int m_newWidth = 60;
    int m_newHeight = 22;
    int m_newThemeIndex = 0;

    // The last path actually written, kept for the whole session so the answer
    // to "where did it go" stays on screen rather than flashing past.
    std::string m_lastSavedPath;
    std::string m_status;
    float m_statusTimer = 0.0f;
};
