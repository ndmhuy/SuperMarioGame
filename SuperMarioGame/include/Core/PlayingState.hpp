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
#include <vector>
#include <memory>

#include "Utils/MapGenerator.hpp"

class Entity;
class Player;

class PlayingState : public IGameState {
public:
    explicit PlayingState(bool startInEditor = false, bool isProcedural = false, const MapGeneratorConfig& genConfig = MapGeneratorConfig());
    ~PlayingState() override;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    PhysicsEngine m_physicsEngine;
    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;
    MapEditor m_mapEditor;
    EventBus::SubscriptionId m_checkpointSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_powerUpSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_fireballSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_levelCompleteSubId = static_cast<EventBus::SubscriptionId>(-1);
    Player* m_player = nullptr;
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    int m_selectedLevelIndex = 0; // 0: Level 1, 1: Level 2, 2: Level 3, 3: Bonus 1
    Camera m_camera;
    TimeRewindManager m_rewindManager;

    bool m_isLevelComplete = false;
    float m_levelCompleteTimer = 0.0f;
    int m_levelCompletePoints = 0;

    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = 300.0f;

    // Minimap overlay (toggled with M via EventType::MinimapToggled)
    std::unique_ptr<Minimap> m_minimap;

    // Combat/impact particle bursts, driven from EventBus
    ParticleEmitter m_particleEmitter;
    EventBus::SubscriptionId m_enemyDefeatedSubId = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_blockBrokenSubId   = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_coinParticleSubId  = static_cast<EventBus::SubscriptionId>(-1);
    EventBus::SubscriptionId m_playerDamagedSubId = static_cast<EventBus::SubscriptionId>(-1);

    bool m_startInEditor = false;
    bool m_isProcedural = false;
    MapGeneratorConfig m_genConfig;

    // Sprite Sheet Atlases (owned by PlayingState)
    std::unique_ptr<SpriteSheet> m_playerSheet;
    std::unique_ptr<SpriteSheet> m_enemySheet;
    std::unique_ptr<SpriteSheet> m_itemSheet;
    std::unique_ptr<SpriteSheet> m_scenerySheet;
    std::unique_ptr<SpriteSheet> m_blocksSheet;

    // Debug / dev overlays
    bool m_showAABB = false;

    // Tilemap animation timer for animated tiles (coin, question, water, etc.)
    float m_tileAnimTimer = 0.0f;

    std::array<bool, 3> m_starCoinsCollected = {false, false, false};
    EventBus::SubscriptionId m_starCoinSubId = static_cast<EventBus::SubscriptionId>(-1);

    void setupTestScene();
    void cleanupTestScene();
    void spawnSelectedPlayer(const sf::Vector2f& pos);
    bool loadLevelByPath(const std::string& jsonPath, sf::Vector2f spawnOverride = {0.0f, 0.0f});

    // Polymorphic animation dispatcher: routes entity to its matching sprite sheet
    void wireEntityAnimations(Entity* entity);
};

