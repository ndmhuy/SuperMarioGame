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
#include "Graphics/LightingRenderer.hpp"
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

    // tests/verify_r21_debug_cheats.cpp reaches the void plane, the level clock,
    // the camera and rescueDestination() to prove Debug > Cheats' IMMORTAL
    // rescues a player out of a pit instead of killing them — the same tradeoff
    // as the friend above, and for the same reason: none of it is worth a public
    // getter that only a test would ever call.
    friend class DebugCheatsTestHooks;

    // tests/verify_r21_versus_and_axes.cpp reaches m_camera, m_player/m_player2,
    // m_hud and the axe counters to prove three things a public API could not
    // show: that the camera follows the survivor of a two-player match, that
    // the eliminated player's badge says so, and that the bridge waits for the
    // last axe. Same tradeoff as the two friends above.
    friend class VersusAndAxesTestHooks;

    // tests/verify_r21o_offcamera_gate.cpp reaches the camera, the entity list
    // and the two census counters to prove BOTH halves of the off-camera update
    // gate on a live instance: that an entity left far behind stops thinking,
    // and that a projectile, a platform and a shadow at the same distance do
    // not. A guard that only checked the first half would license the very bug
    // it exists to prevent. Same tradeoff as the three friends above.
    friend class OffCameraGateTestHooks;

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

    // --- Going down a pipe ---------------------------------------------------
    //
    // A pipe entry is a short scripted slide INTO the mouth before the world
    // changes, so a warp reads as going somewhere rather than as the screen
    // being swapped underneath a standing player.
    //
    // It is scripted motion, not physics: the slide deliberately carries the
    // player inside a solid block, so the frames it runs for skip both the
    // input poll and the physics pass (see the early return in update()) —
    // otherwise collision resolution would shove them straight back out of the
    // pipe they are entering, and the movement keys would let them walk away
    // mid-animation.
    //
    // Holds no Pipe pointer on purpose. Completing the entry replaces
    // m_entities wholesale, and the pipe that started it is one of the things
    // destroyed; everything the completion needs is copied out up front.
    struct PipeEntry {
        bool active = false;
        float elapsed = 0.0f;
        float duration = 0.0f;
        sf::Vector2f from{0.0f, 0.0f};
        sf::Vector2f to{0.0f, 0.0f};
        std::string targetLevel;
        sf::Vector2f exit{0.0f, 0.0f};
    };
    PipeEntry m_pipeEntry;

    // Starts the slide. `mouthCenter` and `approachX` come from the Pipe, which
    // is still alive at the call site and will not be a frame later.
    void beginPipeEntry(sf::Vector2f mouthCenter, float approachX,
                        const std::string& targetLevel, sf::Vector2f exit);
    // Advances it, and performs the warp on the frame it finishes.
    void updatePipeEntry(float dt);

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

        // Who this was, and what their run finished on.
        //
        // Elimination calls destroy() on the Player and forgetEntity() nulls
        // m_player / m_player2, so by the time the HUD next syncs there is no
        // object left to ask "which character was this, and how many coins did
        // they have?". Player 1's badge therefore froze on whatever it last
        // read and Player 2's vanished. Recorded here as an explicit fact at
        // the moment of elimination; inferring it from a pointer that no longer
        // exists only moves the staleness somewhere else.
        std::string characterName;
        std::string badgeLabel;   // "P2" or "CPU"; Player 1's badge has no label
        int finalCoins = 0;
        int finalScore = 0;
    };
    DeathState m_death;    // Player 1
    DeathState m_death2;   // Player 2

    // Records `who`'s identity into their death record, so the HUD can keep
    // showing whose badge it is after the object behind it is gone.
    void rememberEliminatedIdentity(const Player* who, DeathState& death) const;
    // Overwrites the badge fields of a player who is out with the facts kept
    // above, and marks them eliminated. Runs after the two live-player sync
    // blocks in update() precisely so it can override what they left behind:
    // Player 1's block falls through to mock test-scene values once
    // Game::getPlayer() is null, and Player 2's block does not run at all.
    void applyEliminatedBadges(HudData& hudData) const;

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
    // Both participants are in the level. The ONLY case updateVersusCamera()
    // can frame — it needs two positions to take a midpoint of.
    bool bothPlayersPresent() const { return m_player && m_player2; }
    // The participant that single-player logic must act on: Player 1 while they
    // are still in the level, otherwise Player 2, otherwise nothing.
    //
    // Elimination destroys the loser and forgetEntity() nulls its pointer, so
    // "m_player is null" stopped meaning "there is nobody to follow" the moment
    // versus mode existed — it can equally mean "Player 1 is out and Player 2
    // is still playing". update()'s camera dispatch read m_player2 to decide
    // whether this was a two-player frame, so after Player 1 was eliminated it
    // still chose updateVersusCamera(), which returns immediately without both
    // players: the view stopped dead and the survivor ran straight out of it.
    // The view tether and the boss arena clamp had the mirror-image bug, both
    // being written against m_player alone.
    Player* activeParticipant() const;
    // Both players' lives are spent before the run is over.
    bool allPlayersOut() const;
    // The second participant's status line, drawn beside the single-player HUD:
    // scores and the lead in versus, a shared pool in co-op, the delay gauge in
    // Shadow Chase.
    void renderMatchHud(sf::RenderTarget& target) const;
    // Bonus D. Collects this frame's lamps -- the players, every live fireball --
    // and hands them to the shader pass. Called between the world and the HUD so
    // an underground level goes dark without the score bar going with it.
    void renderLightPass(sf::RenderTarget& target);
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    int m_selectedLevelIndex = 0; // 0: Level 1, 1: Level 2, 2: Level 3, 3: Bonus 1
    Camera m_camera;
    TimeRewindManager m_rewindManager;


    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = Constants::LEVEL_TIME;
    // Simulated seconds since the level began, independent of the countdown.
    // Endless Mode and the FREEZE TIMER cheat both stop m_levelTimer without
    // stopping the run, so anything that needs "how long has this been going"
    // — Shadow Mario's replay clock — has to ask this instead.
    float m_runElapsed = 0.0f;
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

    // --- Debug > Cheats: IMMORTAL --------------------------------------------
    //
    // What killPlayer() does instead when the run may not end. Suppressing the
    // kill on its own would be WORSE than the death it replaces: the player
    // drops through the floor and keeps falling forever with nothing to catch
    // them. So the lethal event is converted into a rescue.
    //
    // This is the same shape commit 95521a8 gave bosses for the identical
    // problem — Entity::onLeftLevel() -> Boss::returnToArenaSpawn() exists
    // because a boss that left the level fell forever. The player simply never
    // had the equivalent.
    void rescuePlayer(Player* who, const char* reason);

    // Where a rescue puts `who`, in preference order: solid ground in the column
    // they fell from (so they keep their place in the level, which is what
    // matters mid-take), then the last checkpoint, then the level spawn. Only
    // positions isSpawnUsable() accepts are offered.
    sf::Vector2f rescueDestination(const Player* who) const;

    // --- Debug > Cheats: FREE CAMERA -----------------------------------------
    //
    // Pans the detached camera with WASD/arrows, the same keys and speed
    // MapEditor::handlePanning() uses — the editor's free camera is the
    // behaviour a recorder already knows.
    void updateFreeCamera(float dt);

    // Debug > Cheats' "Kill all enemies on screen" button. Routed through the
    // same onStomped() the POW block uses, so the enemies play their real defeat
    // rather than blinking out of existence.
    void clearOnScreenEnemies();
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
    // Every fifth chunk — roughly every 500 tiles — carries a real Bowser
    // encounter, and no other chunk carries a boss at all. Five because it is
    // far enough apart to be a milestone the run builds towards and near enough
    // that a good run meets several.
    static constexpr int ENDLESS_BOSS_CHUNK_INTERVAL = 5;
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

    // Bonus D -- the GLSL darkness pass (SPEC §19.4). Owns its shader and
    // latches "this machine has no GLSL" so a driver without it costs one
    // stderr line, not one per frame. The theme it darkens for comes from
    // m_background, so the level file's "theme" field drives both.
    LightingRenderer m_lighting;

    // Bonus D's light-pass numbers, live-tunable from Debug > Lighting.
    //
    // They shipped as three constexpr locals inside renderLightPass() plus two
    // literals, so every guess at "is a 9-tile lamp the right size for a cave"
    // cost a rebuild and a fresh run to the cave. That is the same argument the
    // Match panel's sliders already won for the shadow's delay and the CPU's
    // reaction time, and the reason gravity, walk speed and jump height are
    // ImGui-tunable in the first place.
    //
    // Owned HERE rather than in DevPanel because renderLightPass() is the only
    // reader and PlayingState owns the light pass; a static inside the panel
    // would outlive the level it was tuned for. Values are world px unless
    // stated, and every default below is the constant this replaced.
    struct LightingTunables {
        float playerRadius       = 290.0f;   // ~9 tiles: a cave stays readable
        float fireballRadius     = 210.0f;   // a thrown ember, not a torch
        float freeCameraRadius   = 460.0f;   // wider, so F9 panning stays usable
        float playerBreathe      = 0.05f;    // fraction of radius, sinusoidal
        float fireballIntensity  = 0.85f;    // short of 1 on purpose
        float playerShadowTint[3] = {58.0f / 255.0f, 56.0f / 255.0f, 48.0f / 255.0f};

        // Scrubbing the day/night clock rather than exposing DAY_NIGHT_PERIOD.
        //
        // The period only changes how LONG you wait to see a given darkness;
        // what a level actually has to be checked at is the darkness itself, and
        // holding the cycle at a chosen phase gets there in one drag instead of
        // waiting up to 100 s per sample. NIGHT_DARKNESS and DAY_NIGHT_PERIOD
        // themselves stay compile-time constants on LightingRenderer (they are
        // a cross-file contract with radial_light.frag and with
        // tests/verify_r21_lighting.cpp); phase 0.5 reaches the full
        // NIGHT_DARKNESS, so the whole observable range is still covered.
        bool  clockFrozen = false;
        float frozenPhase = 0.5f;            // [0,1): 0 noon, 0.5 midnight
    };
    LightingTunables m_lightingTunables;

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

    // --- How many axes, and why the count is the difficulty knob -------------
    //
    // EASY 1, NORMAL 2 (left + right), HARD 3 (left + middle + right). More
    // axes is HARDER, not easier: every axe has to be reached before the bridge
    // drops, so the route that skips the fight costs one traverse of a lava
    // bridge under Bowser's fire per axe instead of one per fight. The user
    // chose this reading explicitly over "more axes = more chances".
    //
    // Read from Game::difficulty()'s persisted id, which is the only source of
    // truth for the tier — IDifficultyStrategy exposes getId() plus the four
    // scaling factors, and deliberately has no "tier" enum to switch on.
    static int axeQuotaForDifficulty();

    // Sizes the level's axe roster to that quota and arms the counter below.
    //
    // Hung off findActiveBoss() rather than called from each of the five level
    // load paths: the roster belongs to the fight the level has, every path
    // that discovers the boss already calls that one method, and a sixth load
    // path added later therefore cannot forget to size it.
    void configureBridgeAxes();
    // How many the fight started with, and how many are still to be reached.
    // m_axesTotal == 0 means this fight has no axe route at all, and a chop
    // request is then honoured immediately — which is what an axe dropped into
    // a running level by the map editor relies on.
    int m_axesTotal = 0;
    int m_axesRemaining = 0;

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

    // May a flagpole touch finish the level right now? False while a boss is
    // still alive. Shared by the LevelComplete handler and by the gate installed
    // on every Flagpole, so the rule exists once.
    bool levelMayComplete() const;

    // Moves everything queued by a spawn handler this frame into m_entities.
    // Called once per frame, after every loop that walks m_entities has ended.
    void flushPendingSpawns();

    // --- Off-camera update gate ------------------------------------------
    //
    // Endless Mode never discards a chunk it has appended, so m_entities only
    // ever grows: extendEndlessLevelIfNeeded() splices a hundred fresh tiles of
    // content in every time the player gets within 40 tiles of the end, and
    // everything it spliced keeps thinking for the rest of the run. The gate
    // below stops paying for the chunks the player has walked past.
    //
    // Margin is HALF THE LIVE VIEW in each axis rather than a tile constant, so
    // nothing within two viewport widths of the camera centre is ever frozen.
    // At the default 1280x720 view that is 20 tiles horizontally and 11
    // vertically; the versus/co-op camera zooms out to fit both players, and
    // deriving the margin from the view means that wider camera automatically
    // gets a wider safe band instead of silently eating into it.
    AABB thinkingRegion() const;

    // Whether `entity` must keep updating even when it is far outside
    // thinkingRegion(). This is a WHITELIST of what may be frozen, not a
    // blacklist of what may not: a class added later keeps its old behaviour
    // until someone deliberately opts it in.
    bool freezableOffCamera(const Entity& entity) const;

    // Census for the Endless append line. Written by update()'s entity pass;
    // `m_entitiesThought + m_entitiesFrozen` is what the ungated loop used to
    // update every frame, which is what makes the gate's effect measurable
    // from a single run rather than from two builds.
    //
    // m_entitiesExempt is the honest part: entities that were out of the
    // thinking region and kept thinking anyway because freezableOffCamera()
    // said no. `frozen + exempt` is everything out of region, so this counter
    // is the gate's REMAINING HEADROOM — without it the two numbers above
    // cannot distinguish "the gate is working" from "nothing was far away".
    std::size_t m_entitiesThought = 0;
    std::size_t m_entitiesFrozen  = 0;
    std::size_t m_entitiesExempt  = 0;

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

