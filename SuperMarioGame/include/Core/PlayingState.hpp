#pragma once

#include "Core/IGameState.hpp"
#include "Core/EventBus.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/MapEditor.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/LevelLoader.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/Minimap.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include "Core/TimeRewindManager.hpp"
#include "Core/DevPanel.hpp"
#include "Utils/Constants.hpp"
#include <vector>
#include <memory>

#include "Utils/MapGenerator.hpp"
#include "Core/GameOverState.hpp"

class Boss;

class Entity;
class Player;

class PlayingState : public IGameState {
public:
    // `characterIndex` and `levelIndex` let the front-end states rebuild an exact
    // run: character select picks the first, and Game Over's "Retry Level" needs
    // both to restart what the player just lost.
    explicit PlayingState(bool startInEditor = false, bool isProcedural = false,
                          const MapGeneratorConfig& genConfig = MapGeneratorConfig(),
                          int characterIndex = 0, int levelIndex = 0);
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

    // Operations the dev panels request. Defined here (not in the panel) so the
    // state stays in charge of its own invariants.
    void regenerateProceduralLevel();
    void saveToSlot(int slot);
    void loadFromSlot(int slot);

    PhysicsEngine m_physicsEngine;
    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;
    MapEditor m_mapEditor;
    EventBus::SubscriptionId m_checkpointSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_powerUpSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_levelCompleteSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_entitySpawnSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_fireballSubId = static_cast<EventBus::SubscriptionId>(-1);
    Player* m_player = nullptr;
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    int m_selectedLevelIndex = 0; // 0: Level 1, 1: Level 2, 2: Level 3, 3: Bonus 1
    Camera m_camera;
    TimeRewindManager m_rewindManager;


    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = Constants::LEVEL_TIME;
    bool  m_timeWarningFired = false;

    // Level completion (flagpole -> short celebration -> advance)
    bool  m_levelComplete = false;
    float m_levelCompleteTimer = 0.0f;

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
    void killPlayer(const char* reason);

    // Minimap overlay (toggled with M via EventType::MinimapToggled)
    std::unique_ptr<Minimap> m_minimap;

    // Combat/impact particle bursts, driven from EventBus
    ParticleEmitter m_particleEmitter;
    EventBus::SubscriptionId m_enemyDefeatedSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_blockBrokenSubId   = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_coinParticleSubId  = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_playerDamagedSubId = static_cast<EventBus::SubscriptionId>(-1);

    // --- Boss fights ---------------------------------------------------
    // The boss in this level, or null. Found once when the level loads rather
    // than searched for every frame; cleared when it is removed.
    Boss* m_activeBoss = nullptr;
    // True once the player has crossed into the boss arena and the camera is
    // locked to it.
    bool m_arenaLocked = false;
    // The camera bounds to put back when the fight ends.
    AABB m_preArenaCameraBounds;

    // Latch onto whatever boss the freshly loaded level contains.
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
    MapGeneratorConfig m_genConfig;

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

    std::array<bool, 3> m_starCoinsCollected = {false, false, false};
    EventBus::SubscriptionId m_starCoinSubId = static_cast<EventBus::SubscriptionId>(-1);

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

    // Polymorphic animation dispatcher: routes entity to its matching sprite sheet
    void wireEntityAnimations(Entity* entity);
};

