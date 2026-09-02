#pragma once

#include "Core/IGameState.hpp"
#include "Core/EventBus.hpp"
#include "Core/GameMode.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/MapEditor.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/LevelLoader.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/Minimap.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Graphics/TileMapRenderer.hpp"
#include "Graphics/EntityArtBinder.hpp"
#include "Core/TimeRewindManager.hpp"
#include "Core/DevPanel.hpp"
#include "Utils/Constants.hpp"
#include <string>
#include <vector>
#include <memory>

#include "Utils/MapGenerator.hpp"
#include "Core/GameOverState.hpp"
#include "Utils/ObjectPool.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Hammer.hpp"
#include "Entities/BossFireball.hpp"

class Boss;

class Entity;
class Player;
class ShadowMario;
class AIController;

class PlayingState : public IGameState {
public:
    // `characterIndex` and `levelIndex` let the front-end states rebuild an exact
    // run: character select picks the first, and Game Over's "Retry Level" needs
    // both to restart what the player just lost.
    //
    // `match` replaces what used to be a trailing `bool twoPlayer`. A boolean
    // could say "there are two bodies on screen" but not which of versus,
    // co-op, a CPU opponent or a Shadow Mario put them there — and those four
    // want different cameras, different collision rules and a different HUD.
    // `isEndless` builds on the procedural generator rather than replacing it:
    // the first chunk is generated exactly like "Generate & Play", then
    // extendEndlessLevelIfNeeded() appends another chunk of rising difficulty
    // each time the player nears the current far edge, forever (or until they
    // die) — no flagpole, no fixed width, distance travelled is the score.
    // `pendingLoadSlot` is how MenuState's LOAD GAME picker reaches
    // loadFromSlot(): that method is private, called until now only by
    // DevPanel on a PlayingState that is already running. The menu has no such
    // instance, so it builds one specifically to load into, and enter() calls
    // loadFromSlot(pendingLoadSlot) on itself once the level is otherwise set
    // up. 0 (the default) means "no pending load, play normally".
    // `isAttractDemo` is F5 attract mode (SPEC 10.2): instead of the usual
    // "always recording" behaviour (setupTestScene()), this instance loads the
    // bundled demo replay and plays it back — see the comment at that call
    // site. It is always constructed with plain defaults for everything else
    // (SinglePlayer, not endless) by MenuState's idle timer, so it can never be
    // an Endless or versus run.
    // `customLevelPath` plays a level that is not part of the campaign — one
    // authored in the editor, listed by LevelCatalog::customLevels(). When it is
    // set, setupTestScene() loads THAT file instead of pathFor(levelIndex), and
    // advanceToNextLevel() treats the run as a one-off rather than walking into
    // World 1-2. Without it an authored level could be saved and never played,
    // because PlayingState only ever knew how to load a campaign index.
    // `isPlaytest` says this run was PUSHED over an editor rather than started
    // from a menu, so leaving it must pop back to the editor. Quitting with
    // changeState() would replace this state and leave the editor stranded
    // underneath a main menu with the author's unsaved level still in it.
    explicit PlayingState(bool startInEditor = false, bool isProcedural = false,
                          const MapGeneratorConfig& genConfig = MapGeneratorConfig(),
                          int characterIndex = 0, int levelIndex = 0,
                          MatchConfig match = MatchConfig{}, bool isEndless = false,
                          int pendingLoadSlot = 0, bool isAttractDemo = false,
                          std::string customLevelPath = std::string(),
                          bool isPlaytest = false);
    ~PlayingState() override;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    // Something has been pushed over the level (pause, victory, options). The
    // level keeps rendering — it is what the overlay sits on — but its music
    // stops and its ImGui dev surface goes quiet, because an ImGui window drawn
    // underneath an overlay would still take the mouse.
    void onSuspend() override;
    void onResume() override;

private:
    // DevPanel is an extension of this state's debug surface rather than an
    // outside collaborator: it reads the same members render() does and queues
    // mutations back. Friendship here is narrower than exposing setters for the
    // entity list, tilemap and generator config to everyone.
    friend class DevPanel;

    // tests/verify_frontend_states.cpp's LevelCompletionCameraTestHooks reaches
    // m_activeBoss, m_camera and m_selectedLevelIndex to regression-test two
    // defects that only show up across a level transition on a live instance
    // (the boss-alive completion gate and the camera carrying stale arena state
    // into the next level) — narrower than adding getters nothing else would
    // ever call, the same tradeoff DevPanel's friendship above already makes.
    friend class LevelCompletionCameraTestHooks;

    // Operations the dev panels request. Defined here (not in the panel) so the
    // state stays in charge of its own invariants.
    void regenerateProceduralLevel();
    void saveToSlot(int slot);
    void loadFromSlot(int slot);

    PhysicsEngine m_physicsEngine;
    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;

    // Entities created *during* a frame wait here until the frame is at a point
    // where m_entities may safely grow.
    //
    // Every spawn in this game arrives through a synchronous EventBus handler:
    // a question block publishes PowerUpRequested from inside the collision
    // pass, Bowser publishes EntitySpawnRequested from inside his own update().
    // Those handlers used to push_back straight onto m_entities — the very
    // vector the range-for in update() and the physics engine are iterating. A
    // push_back that reallocates leaves that loop holding a pointer into freed
    // storage, which is undefined behaviour: it survived on macOS whenever the
    // vector happened to have spare capacity, and crashed on Windows in 1-3,
    // where Bowser spawns a fireball every 1.2-2.2s for the whole fight. That
    // is why deleting Bowser "fixed" the level.
    std::vector<std::unique_ptr<Entity>> m_pendingSpawns;
    MapEditor m_mapEditor;
    // RAII subscription tokens (audit X-7): each unsubscribes itself in its own
    // destructor, so there is no sentinel value to check and no way to forget
    // one. exit() still resets them explicitly rather than waiting on
    // ~PlayingState(), since these lambdas capture `this` and the intent is
    // "dead the moment the state exits", not merely "eventually, when this
    // object is destroyed".
    EventBus::ScopedSubscription m_checkpointSub;
    EventBus::ScopedSubscription m_playerDiedSub;
    EventBus::ScopedSubscription m_powerUpSub;
    EventBus::ScopedSubscription m_levelCompleteSub;
    EventBus::ScopedSubscription m_entitySpawnSub;
    EventBus::ScopedSubscription m_fireballSub;
    // One press of the crouch key is one warp, not one per frame.
    float m_warpCooldown = 0.0f;

    // --- Dying ---------------------------------------------------------------
    //
    // Death is a sequence, not an instant. killPlayer() starts it; the player
    // pops up and falls off the bottom of the screen; only then does the run
    // either continue at the checkpoint or end.
    //
    // The phase is also the re-entry guard. killPlayer() used to run in full
    // every frame the player was still out of bounds, restarting the game-over
    // fade each time — so the fade never finished, its callback never fired, and
    // falling into the void hung the game instead of showing the end screen.
    enum class DeathPhase { None, Falling, GameOver };
    static constexpr float DEATH_FALL_SECONDS = 1.6f;
    // The corpse leaving the screen can end the fall early, but never before
    // this. Without a floor, a death low in the view cleared the bottom edge in a
    // handful of frames and the whole death — animation, jingle, pause — was over
    // in well under half a second.
    static constexpr float DEATH_FALL_MINIMUM = 0.9f;

    // Death is per participant, not per level.
    //
    // There was one set of these fields, and killPlayer() always acted on
    // m_player. The PlayerDied event has always carried the dying Player* — see
    // Player::powerDown() — but the subscription discarded it, so an enemy
    // hitting Player 2 ran the death sequence, the life deduction and the
    // game-over test on PLAYER 1, who was standing somewhere else entirely
    // unharmed. A single shared phase also meant a genuine second death was
    // swallowed while the first player's 1.6s fall was still running.
    struct DeathState {
        DeathPhase phase = DeathPhase::None;
        float timer = 0.0f;
        std::string reason;
        // Out of lives. Kept so the hazard checks stop re-killing a corpse: an
        // eliminated Player 1 sits below the level, the void check fires again
        // every frame, and the match becomes a permanent loop of death jingles
        // while Player 2 plays on.
        bool eliminated = false;
    };
    DeathState m_death;    // Player 1
    DeathState m_death2;   // Player 2

    // Which record belongs to `who`, or null if it is not a participant (the
    // shadow, for instance, cannot die).
    DeathState* deathStateFor(const Player* who);
    // True while any participant is mid-death — the guard the update loop needs
    // before running hazard checks.
    bool anyDeathInProgress() const;

    void updateDeathSequence(float dt);
    // Put `who` back at the checkpoint. Player 1 additionally resets the level
    // clock and the music; Player 2 just returns to play.
    void respawnPlayer(Player* who);

    Player* m_player = nullptr;

    // --- Two-player versus (task 11.1) -----------------------------------
    // Player 2, or null in a single-player run. m_player stays Player 1
    // throughout, so every existing single-player path keeps working unchanged
    // and only the places that genuinely need both were touched.
    //
    // Player 2 is the same kind of object whether a keyboard or an AIController
    // drives it — which is the whole point of Player exposing verbs rather than
    // reading input itself. Nothing below this line needs to know which.
    Player* m_player2 = nullptr;

    // --- CPU opponent (Bonus: AI multiplayer) ----------------------------
    // Non-null exactly when Player 2 is machine-driven. Owned here rather than
    // by the Player it drives: a controller is not part of what a character is,
    // and the entity list is rebuilt on every level load while the match's
    // configuration outlives it.
    std::unique_ptr<AIController> m_aiController;

    // --- Shadow Mario (Bonus C) ------------------------------------------
    // The player's own path, three seconds late. An observer pointer into
    // m_entities, like m_player and m_player2; cleared by forgetEntity().
    //
    // Deliberately NOT m_player2: a shadow has no lives, no score and cannot
    // win, so treating it as a participant would put it in the versus camera's
    // midpoint, the versus HUD's score line and allPlayersOut()'s verdict.
    ShadowMario* m_shadow = nullptr;

    // What match this is. Read by the camera, the HUD and the end screens.
    MatchConfig m_match;

    // Spawn the second participant the mode calls for — a human Player 2, a
    // CPU-driven Player 2, or a Shadow Mario. Called from setupTestScene() once
    // Player 1 exists, since all three spawn relative to it.
    void spawnMatchParticipants();
    // Feed the shadow this frame's input sample and let it replay a due one.
    void updateShadow(float dt);
    // Seconds of grace left before the shadow reaches the player, drawn as a
    // gauge. Returns -1 when there is no shadow.
    float shadowProximitySeconds() const;

    // Frames both players into one view and shoves whoever falls behind along,
    // so neither can drag the other off-screen. Shared camera rather than split
    // screen: every screen-space overlay the game has — HUD, minimap, pause,
    // victory — would otherwise have to learn about viewports.
    void updateVersusCamera(float dt);
    // Both players' lives are spent before the run is over.
    bool allPlayersOut() const;
    // The second participant's status line, drawn beside the single-player HUD:
    // scores and the lead in versus, a shared pool in co-op, the delay gauge in
    // Shadow Chase.
    void renderMatchHud(sf::RenderTarget& target) const;
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    int m_selectedLevelIndex = 0; // 0: Level 1, 1: Level 2, 2: Level 3, 3: Bonus 1
    Camera m_camera;
    TimeRewindManager m_rewindManager;


    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = Constants::LEVEL_TIME;
    bool  m_timeWarningFired = false;

    // Level completion (flagpole -> walk to the castle door -> short celebration -> advance)
    bool  m_levelComplete = false;
    float m_levelCompleteTimer = 0.0f;
    // Where the castle door is, so the player visibly walks up to it instead of
    // just standing at the flagpole until the summary screen cuts in. Captured
    // once, from the Castle entity found when LevelComplete fires.
    bool  m_hasLevelCompleteCastle = false;
    sf::Vector2f m_levelCompleteCastleTarget{0.0f, 0.0f};

    // Where this level says the player starts — the respawn fallback before any
    // checkpoint is reached.
    sf::Vector2f m_levelSpawnPoint{96.0f, 64.0f};
    // Last checkpoint reached, used by respawn instead of a hardcoded corner.
    sf::Vector2f m_checkpointPosition{96.0f, 64.0f};
    bool m_hasCheckpoint = false;

    // Advance to the next campaign level, or back to the menu after the last.
    void advanceToNextLevel();
    // Reload the current level from scratch, keeping character and level index.
    void restartLevel();
    // Hand the flagpole result to VictoryState and award the time bonus.
    void presentLevelSummary();
    // Build the snapshot GameOverState needs to rebuild this run.
    RunSummary buildRunSummary() const;
    // Kill the player: lose a life and respawn, or end the run.
    // Kill a specific participant. `who` must be Player 1 or Player 2; passing
    // null is treated as Player 1 so the debug key and the older callers keep
    // meaning what they always meant.
    void killPlayer(Player* who, const char* reason);
    // Clear enemies sitting on a respawn point and grant landing invincibility,
    // so coming back to life is not immediately fatal.
    void makeSpawnSafe(Player* who, sf::Vector2f respawn);

    // Minimap overlay (toggled with M via EventType::MinimapToggled)
    std::unique_ptr<Minimap> m_minimap;

    // Combat/impact particle bursts, driven from EventBus
    ParticleEmitter m_particleEmitter;
    EventBus::ScopedSubscription m_enemyDefeatedSub;
    EventBus::ScopedSubscription m_blockBrokenSub;
    EventBus::ScopedSubscription m_coinParticleSub;
    EventBus::ScopedSubscription m_playerDamagedSub;
    // Combo milestone burst (ParticleType::Combo) — the fourth of
    // ParticleEmitter's declared-but-unused types (R7 audit); the other three
    // below are timer-gated per-frame checks rather than events.
    EventBus::ScopedSubscription m_comboParticleSub;
    // Ambient zone particles (water/lava) and wall-slide dust: ParticleEmitter
    // has burst types for all three but nothing called burst() with them.
    // Timer-gated so standing in water for ten seconds does not queue ten
    // seconds of bubbles in one frame's worth of draw calls.
    float m_ambientParticleTimer = 0.0f;
    float m_wallDustTimer = 0.0f;

    // Footstep cadence (SPEC 11.4). Two timers because two players can be on
    // different surfaces at different speeds at once.
    float m_footstepTimer = 0.0f;
    float m_footstepTimer2 = 0.0f;
    void updateFootstep(Player* who, float& timer, float dt);

    // --- Boss fights ---------------------------------------------------
    // The boss in this level, or null. Found once when the level loads rather
    // than searched for every frame; cleared when it is removed.
    Boss* m_activeBoss = nullptr;
    // True once the player has crossed into the boss arena and the camera is
    // locked to it.
    bool m_arenaLocked = false;
    // The camera bounds to put back when the fight ends.
    AABB m_preArenaCameraBounds;

    // Puts the world back to a recorded Memento. Shared by the time rewind and
    // by replay playback — they differ in where the snapshot comes from, not in
    // what applying it means (task 10.3).
    void applySnapshot(const GameSnapshot& snapshot);

    // Latch onto whatever boss the freshly loaded level contains.
    // Drops every raw pointer this state holds into an entity that is about to
    // be destroyed. Called from the prune, before the unique_ptr is released.
    void forgetEntity(Entity* entity);
    void releaseBossArena();
    void findActiveBoss();
    // Lock on arena entry, keep the player inside, release when the boss dies.
    void updateBossArena();
    // Fill the HUD's boss fields, which have had no producer until now.
    void syncBossHud(HudData& hudData) const;

    // True while an overlay state sits above this one.
    bool m_suspended = false;
    // Set once the victory screen has been pushed for this level, so the
    // celebration timer cannot push a second one.
    bool m_summaryShown = false;

    bool m_startInEditor = false;
    bool m_isProcedural = false;
    // Set only for a one-off custom level; empty for every campaign run.
    std::string m_customLevelPath;
    // True when an EditorState pushed this run; see the constructor's doc.
    bool m_isPlaytest = false;

    // Leave this run the way it was entered: pop back to the editor for a
    // playtest, replace with the main menu otherwise.
    void leaveToCallingScreen();
    MapGeneratorConfig m_genConfig;

    // Guards exit() against running twice per transition: GameStateManager
    // calls it explicitly before pop_back() destroys this object, and
    // ~PlayingState() also calls it as a safety net. See exit()'s own comment.
    bool m_hasExited = false;

    // Slot to call loadFromSlot() with at the end of this run's first enter(),
    // or 0 for none. Consumed (reset to 0) once acted on, so a later level
    // transition that reuses this same instance never re-triggers it.
    int m_pendingLoadSlot = 0;

    // F5 attract mode (SPEC 10.2). setupTestScene() checks this to load and
    // play the bundled demo replay instead of recording a new one; update()
    // returns to MenuState once the replay runs out; handleInput() returns to
    // MenuState on the first key of any kind rather than running its usual
    // per-key logic (see the top of handleInput()).
    bool m_isAttractDemo = false;

    // Replay playback pacing (update()'s ReplayRecorder::isPlaying() branch).
    // ReplayRecorder::record() keeps only 1 real simulation frame in every
    // kFrameInterval, so advance() must not be called once per update() tick —
    // that would play the whole recording back kFrameInterval times faster
    // than it was captured (a ~28s attract-mode demo finished in well under
    // 5 seconds — caught by actually watching it, not by the existing
    // ReplayRecorder-only regression test, which never drives it through
    // PlayingState's update loop). m_replayHoldTicks holds the last applied
    // snapshot for the frames in between; m_replayPlaybackActive detects a
    // fresh isPlaying()==true edge (the console's "replay play" can start a
    // new playback in an instance that already ran one) so a stale hold count
    // never carries over into it.
    int m_replayHoldTicks = 0;
    bool m_replayPlaybackActive = false;

    // Set when MapGenerator::generateSolvable() exhausted every reseed attempt
    // and kept the last (unverified) layout anyway — surfaced in the dev
    // panel rather than silently trusted (audit D3).
    bool m_lastLevelUnverified = false;

    // --- Endless Mode ----------------------------------------------------
    //
    // Not just a very wide procedural level: the tilemap starts at one chunk
    // and grows for as long as the player keeps walking, so there is no width
    // to size up front and no flagpole to reach. See
    // extendEndlessLevelIfNeeded() in the .cpp for why a chunk is generated in
    // an isolated TileMap/entity list rather than directly into the live one.
    bool m_isEndless = false;
    int m_endlessChunkIndex = 0;          // chunks appended so far; drives difficulty
    float m_endlessBestDistanceTiles = 0.0f;
    static constexpr int ENDLESS_CHUNK_TILES = 100;
    static constexpr int ENDLESS_LOOKAHEAD_TILES = 40;
    void extendEndlessLevelIfNeeded();

    // Sprite Sheet Atlases (owned by PlayingState)
    std::unique_ptr<SpriteSheet> m_playerSheet;
    std::unique_ptr<SpriteSheet> m_enemySheet;
    std::unique_ptr<SpriteSheet> m_itemSheet;
    std::unique_ptr<SpriteSheet> m_scenerySheet;
    std::unique_ptr<SpriteSheet> m_blocksSheet;

    // Dev/debug ImGui surface. Draws on the render path but performs no game
    // mutation itself — see DevPanel.hpp.
    DevPanel m_devPanel;

    // Tilemap animation timer for animated tiles (coin, question, water, etc.)
    float m_tileAnimTimer = 0.0f;

    // Parallax backdrop (task 5.5). Before this the window was cleared to a flat
    // cornflower blue and every level looked identical behind the geometry.
    BackgroundRenderer m_background;

    // How a TileType becomes a sprite. Mutable because render() is where the
    // sheet and theme are pushed into it and render() is const-correct about
    // nothing else it touches either; keeping it a member rather than a local
    // avoids rebuilding it 60 times a second.
    mutable TileMapRenderer m_tileMapRenderer;

    // Which atlas each entity class draws from. Shared with EditorState.
    mutable EntityArtBinder m_artBinder;

    std::array<bool, 3> m_starCoinsCollected = {false, false, false};
    EventBus::ScopedSubscription m_starCoinSub;

    // Tell the parallax backdrop where this level's floor is, so the layers that
    // are meant to stand on the ground actually do.
    void syncBackdropGround();

    // World y at which a falling player is counted as having left the level.
    //
    // Recomputed per level by syncVoidPlane(), one tile under the deepest floor
    // rather than under the whole tilemap — see the comment at its use site in
    // update(). Defaults to something safely far down until a level is loaded.
    void syncVoidPlane();
    float m_voidPlaneY = 100000.0f;

    // What the HUD's WORLD field should read for the level now loaded.
    //
    // Recomputed at every load rather than derived from m_selectedLevelIndex at
    // read time, because a warp pipe loads a sub-level by PATH and leaves the
    // index pointing at the parent — so the index alone cannot tell 1-1 from
    // the room underneath it.
    void refreshWorldLabel(const std::string& levelPath);
    std::string m_worldLabel = "WORLD 1-1";

    // Stand the end-of-level scenery on the floor.
    //
    // The flagpole is a 24x168 sprite whose position is its TOP-left corner, and
    // every level file names a tile row for it directly — 12 in all four shipped
    // levels, which puts its foot at world y 552 against a ground surface at
    // 672. The flag therefore hung 120px (nearly four tiles) in mid-air in every
    // level in the game, and MapGenerator's arithmetic left it floating too.
    //
    // Rather than correct four numbers in four files and the generator, and then
    // correct them again for every level anyone authors afterwards, the pole is
    // dropped onto whatever floor is beneath it at load time. The level file
    // says WHERE the flag is; the geometry says how high. Same for the castle,
    // whose door has to meet the same floor.
    void settleEndOfLevelScenery();

    // The world y of the first solid tile's top surface at or below `from`, or a
    // negative value when the column is empty all the way down.
    float floorBelow(float worldX, float fromWorldY) const;

    // Whether a player can be put down at `topLeft` without being buried.
    //
    // Requires all three of: inside the map; the 32x32 box a small player
    // occupies clear of solid tiles; and a floor somewhere beneath that box.
    bool isSpawnUsable(sf::Vector2f topLeft) const;

    // `desired` if isSpawnUsable() accepts it, otherwise the nearest position on
    // the same row that it does accept, otherwise `desired` unchanged (with a
    // complaint on stderr — there is nothing better to offer).
    //
    // Every level transition goes through this: pipe warps, the flagpole
    // advancing to the next level, level select, and LOAD GAME.
    sf::Vector2f usableSpawnNear(sf::Vector2f desired, const std::string& levelPath) const;


    // Somewhere the given player can come back that is on screen and on solid
    // ground.
    //
    // Respawning used to be unconditional: back to the checkpoint, or the level
    // spawn. In one-player that is fine — the camera follows the only player, so
    // wherever they land is what you are looking at. In two-player the camera
    // frames the MIDPOINT of both players, so sending the dead one back to the
    // level start while their partner is 150 tiles away either drags the frame
    // backwards across the level or leaves the respawned player off screen
    // entirely, being shoved along by the versus tether they cannot see. And
    // neither mode checked that the destination had a floor at all, so a
    // checkpoint taken over a pit respawned the player straight back into it.
    //
    // Returns a point above solid ground inside the current view where one
    // exists, and the fallback otherwise.
    sf::Vector2f findSafeRespawn(sf::Vector2f preferred, const Player* nextTo) const;

    // --- P-Switch: bricks become coins for its duration, then change back ----
    //
    // PSwitchActivated has been published since the switch was written and the
    // HUD has always had a pSwitchActive field, but nothing between the two
    // existed: pressing the switch played a sound and changed nothing in the
    // world. These do the swap.
    void beginPSwitch(float seconds);
    void updatePSwitch(float dt);
    void endPSwitch();

    // --- The axe: the non-combat way past Bowser -----------------------------
    //
    // Drops every span of bridge that crosses lava inside the boss's arena, and
    // ends the fight with it. Bowser is immune to fire, carries i-frames that
    // make contact during them harmful, and attacks continuously — the series
    // has always paired that with a second solution, and this is it.
    void chopBridge();

    // --- POW block: clears the enemies that are standing on something --------
    //
    // Same gap: POWBlockHit only ever reached the camera shake and the sound.
    void detonatePOW();

    // Exactly the cells beginPSwitch() changed, so ending it restores the level
    // rather than guessing which bricks were always coins.
    struct SwappedTile {
        int x = 0;
        int y = 0;
        TileType original = TileType::Empty;
    };
    std::vector<SwappedTile> m_pSwitchSwaps;
    float m_pSwitchTimer = 0.0f;
    bool  m_pSwitchActive = false;
    EventBus::ScopedSubscription m_pSwitchSub;
    EventBus::ScopedSubscription m_powSub;
    EventBus::ScopedSubscription m_bridgeSub;

    void setupTestScene();
    void cleanupTestScene();
    void spawnSelectedPlayer(const sf::Vector2f& pos);

    // Takes ownership of `player`, installs it at m_entities[0], and refreshes every
    // observer that holds a raw Player* (m_player, InputManager, Game) plus its
    // animations. Every path that replaces the active player MUST go through this —
    // assigning m_entities[0] directly leaves m_player dangling (audit A-3).
    void adoptPlayer(std::unique_ptr<Player> player);
    bool loadLevelByPath(const std::string& jsonPath, sf::Vector2f spawnOverride = {0.0f, 0.0f});

    // The single door every entity comes through on its way into the world:
    // wires its animations and applies the difficulty modifiers.
    void admitEntity(Entity* entity);

    // Moves everything queued by a spawn handler this frame into m_entities.
    // Called once per frame, after every loop that walks m_entities has ended.
    void flushPendingSpawns();

    // The single door a mid-frame spawn goes through. Wires animations and
    // difficulty immediately — so the caller still gets a fully formed entity —
    // but defers the list insertion to flushPendingSpawns().
    void queueSpawn(std::unique_ptr<Entity> entity);

    // --- Object pooling (task 10.1) --------------------------------------
    // Projectiles are the churn: every shot was a heap allocation on spawn and
    // a free on despawn. ParticleSystem was already pooled by hand — it has
    // always kept a fixed slot array with an active flag — so it is deliberately
    // left alone; wrapping it in this template would add allocation, not remove
    // it.
    ObjectPool<Fireball> m_fireballPool;
    ObjectPool<Hammer> m_hammerPool;
    ObjectPool<BossFireball> m_bossFireballPool;

    // Builds a projectile from its pool, or from the factory for anything not
    // pooled. One entry point, so a caller never has to know which is which.
    std::unique_ptr<Entity> spawnProjectile(int entityType, sf::Vector2f position,
                                            sf::Vector2f velocity);
    // Offers a spent entity back to whichever pool owns its type. Anything else
    // is simply destroyed, exactly as before.
    void recycleEntity(std::unique_ptr<Entity> entity);

    // Polymorphic animation dispatcher: routes entity to its matching sprite sheet
    void wireEntityAnimations(Entity* entity);

    // The way MapEditor reaches this state's own bookkeeping.
    //
    // A nested adapter rather than making PlayingState itself an
    // IEntityAdmitter: admitEntity() and forgetEntity() stay private, and
    // nothing outside this class gains the ability to call them. The editor
    // mutates m_entities directly, so without this an entity it placed never
    // had setupAnimations() run (it drew as a flat placeholder box) and an
    // entity it erased left m_player, InputManager and Game holding freed
    // memory.
    class EditorBridge : public IEntityAdmitter {
    public:
        explicit EditorBridge(PlayingState& owner) : m_owner(owner) {}
        void admit(Entity* entity) override { m_owner.admitEntity(entity); }
        void release(Entity* entity) override { m_owner.forgetEntity(entity); }
    private:
        PlayingState& m_owner;
    };
    EditorBridge m_editorBridge{*this};
};

