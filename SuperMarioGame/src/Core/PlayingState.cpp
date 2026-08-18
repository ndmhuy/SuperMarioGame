#define NOMINMAX
#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/ResourceManager.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/ScreenTransitionManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Graphics/EntityDeathEffect.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Entities/Block.hpp"
#include "Entities/StarCoin.hpp"
#include "Entities/Pipe.hpp"

#include "Core/EventBus.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/InputManager.hpp"
#include "Entities/Player.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Star.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/EntityFactory.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <imgui.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <random>
#include <unordered_map>


PlayingState::PlayingState(bool startInEditor, bool isProcedural, const MapGeneratorConfig& genConfig)
    : m_startInEditor(startInEditor), m_isProcedural(isProcedural), m_genConfig(genConfig) {}

PlayingState::~PlayingState() {
    exit();
}

void PlayingState::enter() {
    std::cout << "Entering PlayingState (startInEditor: " << m_startInEditor << ", isProcedural: " << m_isProcedural << ")" << std::endl;

    // Load SFX & Start Level BGM
    SoundManager::getInstance().loadAllSounds();
    SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);

    // --- Load all 5 Sprite Sheet Atlases ---
    // Path candidates search: run from build/Debug or project root
    auto tryLoadSheet = [](const std::string& folderName) -> std::unique_ptr<SpriteSheet> {
        std::vector<std::string> candidates = {
            "assets/spriteSheet/" + folderName,
            "SuperMarioGame/assets/spriteSheet/" + folderName,
            "../assets/spriteSheet/" + folderName
        };
        for (const auto& path : candidates) {
            if (std::filesystem::exists(path)) {
                try {
                    return std::make_unique<SpriteSheet>(path);
                } catch (...) {}
            }
        }
        std::cerr << "[PlayingState] WARNING: Could not locate sprite sheet: " << folderName << std::endl;
        return nullptr;
    };

    m_playerSheet  = tryLoadSheet("player");
    m_enemySheet   = tryLoadSheet("enemy_projectile");
    m_itemSheet    = tryLoadSheet("item");
    m_scenerySheet = tryLoadSheet("world_scenery_item");
    // Note: tileset_blocks is deprecated — tile sprites come from world_scenery_item (m_scenerySheet)

    // Initialize HUD and Level Timer
    m_hud = std::make_unique<Hud>(sf::Vector2i(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT), m_itemSheet.get(), m_playerSheet.get());
    m_levelTimer = 300.0f;
    m_tileAnimTimer = 0.0f;

    if (m_isProcedural) {
        cleanupTestScene();
        MapGenerator::generate(m_tileMap, m_entities, m_genConfig);

        // Wire animations for all procedurally generated entities
        for (auto& entity : m_entities) {
            wireEntityAnimations(entity.get());
        }
        
        m_player = nullptr;
        for (const auto& entity : m_entities) {
            if (auto p = dynamic_cast<Player*>(entity.get())) {
                m_player = p;
                break;
            }
        }
        if (!m_player) {
            spawnSelectedPlayer(sf::Vector2f(96.0f, 64.0f));
        } else {
            InputManager::getInstance().registerPlayer(m_player, 0);
            Game::getInstance().setPlayer(m_player);
        }
        Game::getInstance().setTileMap(&m_tileMap);
        m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});
    } else {
        setupTestScene();
    }

    if (m_startInEditor && !m_mapEditor.isActive()) {
        m_mapEditor.toggleActive();
    }

    // Auto-save at checkpoint
    m_checkpointSubId = EventBus::getInstance().subscribe(EventType::CheckpointActivated, [this](const GameEvent& ev) {
        int activeSlot = Game::getInstance().getActiveSlot();
        if (!m_entities.empty() && m_entities[0]) {
            if (auto* player = dynamic_cast<Player*>(m_entities[0].get())) {
                bool success = Serializer::saveGame(activeSlot, *player, 1, "Level 1", Constants::LEVEL_TIME, player->getPosition().x, player->getPosition().y, std::vector<bool>(m_starCoinsCollected.begin(), m_starCoinsCollected.end()));
                if (success) {
                    std::cout << "[Auto-Save] Progress saved to Slot " << activeSlot << " at checkpoint!" << std::endl;
                }
            }
        }
    });

    // Fireball Shooting Event Listener
    m_fireballSubId = EventBus::getInstance().subscribe(EventType::PlayerShotFireball, [this](const GameEvent& ev) {
        auto* player = std::any_cast<Player*>(ev.data);
        if (!player) return;

        // Count active fireballs
        int activeFireballs = 0;
        for (const auto& entity : m_entities) {
            if (auto fb = dynamic_cast<Fireball*>(entity.get())) {
                if (fb->isActive()) {
                    activeFireballs++;
                }
            }
        }

        if (activeFireballs < 2) { // Max 2 active fireballs on screen
            float dir = player->isFacingRight() ? 1.0f : -1.0f;
            sf::Vector2f spawnPos = player->getPosition() + sf::Vector2f(dir * 16.0f, 8.0f);
            sf::Vector2f vel(dir * 350.0f, 50.0f);

            auto fireball = EntityFactory::createFireball(spawnPos, vel);
            m_entities.push_back(std::move(fireball));

            SoundManager::getInstance().playSound("fireball");
        }
    });

    m_starCoinsCollected = {false, false, false};
    m_starCoinSubId = EventBus::getInstance().subscribe(EventType::StarCoinCollected, [this](const GameEvent& ev) {
        for (int i = 0; i < 3; ++i) {
            if (!m_starCoinsCollected[i]) {
                m_starCoinsCollected[i] = true;
                break;
            }
        }
    });

    // --- Minimap: 200x40 screen-space overview, toggled with M ---
    // Constructed after the level is loaded so initialize() sees the final tile grid.
    m_minimap = std::make_unique<Minimap>(
        sf::Vector2f(Constants::WINDOW_WIDTH - 220.0f, Constants::WINDOW_HEIGHT - 60.0f),
        sf::Vector2f(200.0f, 40.0f));
    m_minimap->initialize(m_tileMap);

    // --- Particle bursts + death effects, driven from the EventBus ---
    EventBus& bus = EventBus::getInstance();

    m_enemyDefeatedSubId = bus.subscribe(EventType::EnemyDefeated, [this](const GameEvent&) {
        // The event only carries a score value, so locate the enemy that just went
        // inactive to place the burst and the flip animation at its last position.
        if (!m_enemySheet) return;
        for (const auto& entity : m_entities) {
            auto enemy = dynamic_cast<Enemy*>(entity.get());
            if (!enemy || enemy->isActive()) continue;

            const sf::Vector2f pos = enemy->getPosition();
            m_particleEmitter.burst(pos + sf::Vector2f(16.0f, 16.0f), ParticleType::Stomp);
            EntityDeathEffect::getInstance().spawnDeathEffect(
                pos, m_enemySheet->getSprite("goomba_brown_move_0"), DeathEffectType::EnemyFlip);
            break;
        }
    });

    m_blockBrokenSubId = bus.subscribe(EventType::BlockBroken, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter() + sf::Vector2f(0.0f, -24.0f),
                                              ParticleType::BrickBreak);
    });

    m_coinParticleSubId = bus.subscribe(EventType::CoinCollected, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter(), ParticleType::CoinSparkle);
    });

    m_playerDamagedSubId = bus.subscribe(EventType::PlayerDamaged, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter(), ParticleType::DeathPoof);
    });

    // --- Screen transition: fade the level in on entry ---
    ScreenTransitionManager::getInstance().reset();
    ScreenTransitionManager::getInstance().fadeIn(0.45f);
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
    cleanupTestScene();

    if (m_checkpointSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_checkpointSubId);
        m_checkpointSubId = static_cast<EventBus::SubscriptionId>(-1);
    }
    if (m_fireballSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_fireballSubId);
        m_fireballSubId = static_cast<EventBus::SubscriptionId>(-1);
    }
    if (m_starCoinSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_starCoinSubId);
        m_starCoinSubId = static_cast<EventBus::SubscriptionId>(-1);
    }

    // Particle / death-effect subscriptions. These capture `this`, so they must go
    // before the state is destroyed or the next publish dereferences freed memory.
    {
        EventBus& bus = EventBus::getInstance();
        const auto NONE = static_cast<EventBus::SubscriptionId>(-1);
        for (auto* id : { &m_enemyDefeatedSubId, &m_blockBrokenSubId,
                          &m_coinParticleSubId, &m_playerDamagedSubId }) {
            if (*id != NONE) { bus.unsubscribe(*id); *id = NONE; }
        }
    }

    m_minimap.reset();
    EntityDeathEffect::getInstance().clear();

    // Unregister player and tilemap to prevent dangling pointer crashes on exit
    InputManager::getInstance().registerPlayer(nullptr, 0);
    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::F1) {
            m_mapEditor.toggleActive();
            std::cout << "[Editor] F1 pressed. Active = " << m_mapEditor.isActive() << std::endl;
        }

        if (!m_mapEditor.isActive()) {
            if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                Game::getInstance().changeState(std::make_unique<MenuState>());
            }

            // M toggles the minimap. Minimap subscribes to this event itself.
            if (keyPressed->code == sf::Keyboard::Key::M) {
                EventBus::getInstance().publish({EventType::MinimapToggled, 0});
            }

            EventBus& bus = EventBus::getInstance();
            switch (keyPressed->code) {
                case sf::Keyboard::Key::Num1: // Collect Coin
                    bus.publish({EventType::CoinCollected, 1});
                    break;
                case sf::Keyboard::Key::Num2: // Defeat Enemy
                    bus.publish({EventType::EnemyDefeated, 0});
                    break;
                case sf::Keyboard::Key::Num3: // Take Damage
                    bus.publish({EventType::PlayerDamaged, 0});
                    break;
                case sf::Keyboard::Key::Num4: // Lose Life / Die
                    bus.publish({EventType::PlayerDied, 0});
                    break;
                case sf::Keyboard::Key::Num5: // Checkpoint Activated
                    bus.publish({EventType::CheckpointActivated, 0});
                    break;
                case sf::Keyboard::Key::Num6: // Finish Level 3 (unlocks character achievements)
                    bus.publish({EventType::LevelComplete, 3});
                    break;
                case sf::Keyboard::Key::Num7: // Boss Defeated
                    bus.publish({EventType::BossDefeated, 0});
                    break;
                case sf::Keyboard::Key::Num8: // Star Coin Collected
                    bus.publish({EventType::StarCoinCollected, 0});
                    break;
                case sf::Keyboard::Key::Num9: // Hidden Block Broken
                    bus.publish({EventType::BlockBroken, 0});
                    break;
                default:
                    break;
            }
        }
    }

    if (!m_mapEditor.isActive() && m_player) {
        InputManager::getInstance().handleInput(event, *m_player);
    }
}

void PlayingState::update(float dt) {
    // Advance tile animation timer
    m_tileAnimTimer += dt;

    if (m_mapEditor.isActive()) {
        sf::Vector2f mouseWorldPos = Game::getInstance().getMouseWorldPosition(m_camera.getView());
        m_mapEditor.update(m_tileMap, m_entities, mouseWorldPos, dt, &m_camera);
        m_camera.update(dt);
        return;
    }

    // 1. Update trackers
    StatisticsTracker::getInstance().update(dt);
    AchievementManager::getInstance().update(dt);

    // 0. Time Rewind check (Hold R or Left/Right Shift keys to rewind time using Memento snapshots)
    bool rewindRequested = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R) ||
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

    if (rewindRequested && m_rewindManager.hasSnapshots()) {
        m_rewindManager.setRewinding(true);
        GameSnapshot snapshot = m_rewindManager.popSnapshot();

        m_levelTimer = snapshot.levelTimer;
        m_camera.getView().setCenter(snapshot.cameraCenter);

        if (m_player) {
            m_player->restoreMemento(snapshot.playerState);
        }

        // Restore entities by id, not by index. Between record and restore the
        // prune step removes inactive entities and the fireball listener appends
        // new ones, so positions in m_entities do not correspond across frames.
        std::unordered_map<std::uint32_t, const EntitySnapshot*> byId;
        byId.reserve(snapshot.entityStates.size());
        for (const auto& es : snapshot.entityStates) {
            byId.emplace(es.id, &es);
        }
        for (const auto& entity : m_entities) {
            if (!entity) continue;
            auto it = byId.find(entity->getId());
            if (it == byId.end()) continue;  // spawned after this snapshot — leave it alone
            entity->setPosition(it->second->position);
            entity->setVelocity(it->second->velocity);
            entity->active = it->second->active;
        }
        return; // Skip forward physics integration while rewinding
    }

    m_rewindManager.setRewinding(false);
    // The snapshot for this frame is recorded at the end of update(), once
    // physics has run. Recording here as well used to double the rate and halve
    // the effective rewind window.

    // 2. Process held keys (MoveLeft, MoveRight, Crouch, Run)
    if (m_player) {
        InputManager::getInstance().update(*m_player);
    }

    // 3. Update all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->update(dt);
        }
    }

    // 3. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 3b. Prune inactive entities (keep m_player intact even if inactive)
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(), [this](const std::unique_ptr<Entity>& entity) {
            return entity && !entity->isActive() && entity.get() != m_player;
        }),
        m_entities.end()
    );

    // 3c. Void fall detection & Revive Mechanics
    float bottomVoidY = (m_tileMap.getHeight() * Constants::TILE_SIZE) + 32.0f;
    if (m_player && m_player->getPosition().y > bottomVoidY) {
        SoundManager::getInstance().playSound("pipe");
        if (m_player->getLives() > 1) {
            m_player->loseLife();
            m_player->setPosition(sf::Vector2f(96.0f, 64.0f));
            m_player->setVelocity(sf::Vector2f(0.0f, 0.0f));
            m_camera.getView().setCenter(sf::Vector2f(640.0f, 360.0f));
            std::cout << "[PlayingState] Player fell into void pit! Revived at overhead spawn. Lives remaining: " << m_player->getLives() << std::endl;
        } else {
            m_player->loseLife();
            std::cout << "[PlayingState] Game Over! Out of lives." << std::endl;
            // Fade to black before handing control back, rather than cutting instantly.
            ScreenTransitionManager::getInstance().fadeOut(0.6f, []() {
                Game::getInstance().changeState(std::make_unique<MenuState>());
            });
            return;
        }
    }

    // 3d. Warp Pipe check for sub-level transitions or teleportation
    for (const auto& entity : m_entities) {
        if (auto pipe = dynamic_cast<Pipe*>(entity.get())) {
            if (m_player && pipe->checkWarp(*m_player)) {
                std::string target = pipe->getTargetLevel();
                sf::Vector2f exitPos = pipe->getExitPosition();
                if (!target.empty()) {
                    loadLevelByPath(target, exitPos);
                } else if (exitPos.x != 0.0f || exitPos.y != 0.0f) {
                    m_player->setPosition(exitPos);
                    m_player->setVelocity({0.0f, 0.0f});
                }
                break;
            }
        }
    }


    // 4. Update Camera & Screen Transitions
    if (m_player) {
        m_camera.follow(m_player->getPosition(), dt);
    }
    m_camera.update(dt);
    ScreenTransitionManager::getInstance().update(dt);

    // 4b. Update the wired visual subsystems.
    // Death effects need the camera's bottom edge so instances can despawn once
    // they fall off-screen; particles and the minimap are camera-independent.
    ParticleSystem::getInstance().update(dt);
    AABB visible = m_camera.getVisibleBounds();
    EntityDeathEffect::getInstance().update(dt, visible.y + visible.height);
    if (m_minimap) {
        m_minimap->update(dt, m_player, m_entities);
    }

    // 5. Sync HUD with player stats or fallback mock data
    if (m_hud) {
        m_hud->update(dt);
        HudData hudData;
        hudData.timeLeft = static_cast<int>(m_levelTimer);
        
        if (auto* player = Game::getInstance().getPlayer()) {
            hudData.score = player->getScore();
            hudData.coins = player->getCoins();
            hudData.lives = player->getLives();
            hudData.comboCount = player->getComboCounter();
            hudData.characterName = player->getCharacterName();
            hudData.starCoinsCollected = m_starCoinsCollected;
        } else {
            // Mockup values matching the visual reference when running the test scene
            hudData.score = 102520;
            hudData.coins = 57;
            hudData.lives = 9;
            hudData.worldMajor = 1;
            hudData.worldMinor = 1;
            hudData.characterName = "mario";
            hudData.starCoinsCollected = {true, true, false}; // 2 out of 3 collected
        }
        m_hud->sync(hudData);
    }

    // Record the Memento snapshot for this frame — exactly once, here, after
    // physics has run. This is the ONLY recording site; a second one at the top
    // of update() used to double the rate and halve the rewind window.
    if (!m_rewindManager.isRewinding()) {
        GameSnapshot snapshot;
        snapshot.levelTimer = m_levelTimer;
        snapshot.cameraCenter = m_camera.getView().getCenter();

        if (m_player) {
            snapshot.playerState = m_player->createSnapshot();
        }

        snapshot.entityStates.reserve(m_entities.size());
        for (const auto& entity : m_entities) {
            if (entity) {
                snapshot.entityStates.push_back({
                    entity->getId(),
                    entity->getPosition(),
                    entity->getVelocity(),
                    entity->isActive()
                });
            }
        }

        m_rewindManager.recordSnapshot(snapshot);
    }
}

void PlayingState::render(sf::RenderTarget& target) {
    // Set view to camera view for scrolling world space rendering
    target.setView(m_camera.getView());

    // Compute animated tile frame indices from timer
    int coinFrame     = static_cast<int>(m_tileAnimTimer / 0.15f) % 4;
    int questionFrame = static_cast<int>(m_tileAnimTimer / 0.20f) % 4;
    int conveyorFrame = static_cast<int>(m_tileAnimTimer / 0.10f) % 4;
    int waterFrame    = static_cast<int>(m_tileAnimTimer / 0.18f) % 4;

    // 1. Draw the tilemap tiles (bottom-to-top to ensure overlapping/bobbing top layers draw on top of background)
    for (int y = m_tileMap.getHeight() - 1; y >= 0; --y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            TileType tileType = m_tileMap.getTileType(x, y);
            if (tileType == TileType::Empty) continue;

            sf::Vector2f tilePos(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE);
            bool spriteDrawn = false;

            if (m_scenerySheet) {
                std::string frameKey;

                switch (tileType) {
                    case TileType::Ground: {
                        // Use grass-top tile on exposed top row, dirt fill otherwise
                        bool isTopExposed = (y == 0) || (m_tileMap.getTileType(x, y - 1) == TileType::Empty);
                        frameKey = isTopExposed ? "solid_block_brown" : "solid_block_grey";
                        break;
                    }
                    case TileType::Brick:
                        frameKey = "brick_brown_side";
                        break;
                    case TileType::Question:
                        frameKey = "question_block_" + std::to_string(questionFrame % 3);
                        break;
                    case TileType::Pipe: {
                        bool isTopExposed = (y == 0) || (m_tileMap.getTileType(x, y - 1) != TileType::Pipe);
                        bool hasLeftTile = (x > 0) && (m_tileMap.getTileType(x - 1, y) == TileType::Pipe);
                        if (isTopExposed) {
                            frameKey = hasLeftTile ? "pipe_green_head_right" : "pipe_green_head_left";
                        } else {
                            frameKey = hasLeftTile ? "pipe_green_body_right" : "pipe_green_body_left";
                        }
                        break;
                    }
                    case TileType::Ice:
                        // No dedicated ice sprite in world_scenery — use solid_block_blue as fallback
                        frameKey = "solid_block_blue";
                        break;
                    case TileType::Conveyor:
                        frameKey = "conveyor_belt_green";
                        break;
                    case TileType::Water: {
                        bool isSurface = (y == 0) || (m_tileMap.getTileType(x, y - 1) != TileType::Water);
                        frameKey = isSurface ? "water_dark_blue_wave_short" : "water_dark_blue_bg";
                        break;
                    }
                    case TileType::Coin:
                        frameKey = "coin_" + std::to_string(coinFrame % 2);
                        break;
                    default:
                        break;
                }

                if (!frameKey.empty()) {
                    sf::Sprite tileSprite = m_scenerySheet->getSprite(frameKey);
                    auto bounds = tileSprite.getLocalBounds();
                    if (bounds.size.x > 0 && bounds.size.y > 0) {
                        tileSprite.setScale(sf::Vector2f(
                            Constants::TILE_SIZE / bounds.size.x,
                            Constants::TILE_SIZE / bounds.size.y
                        ));
                        sf::Vector2f drawPos = tilePos;
                        if (tileType == TileType::Water && (y == 0 || m_tileMap.getTileType(x, y - 1) != TileType::Water)) {
                            // Pure vertical bobbing up and down (started 5px lower to prevent exposing top gap)
                            float bobY = std::sin(m_tileAnimTimer * 3.0f) * 2.5f;
                            drawPos.y += 5.0f + bobY;
                        }
                        tileSprite.setPosition(drawPos);
                        target.draw(tileSprite);
                        spriteDrawn = true;
                    }
                }
            }

            // Fallback to debug color rectangles if no atlas loaded or unknown tile
            if (!spriteDrawn) {
                const TileInfo& info = TileMap::getInfo(tileType);
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(tilePos);
                tileShape.setFillColor(info.debugColor);
                tileShape.setOutlineColor(sf::Color(60, 40, 20));
                tileShape.setOutlineThickness(0.5f);
                target.draw(tileShape);

                // Decoration on debug shapes
                if (tileType == TileType::Ground) {
                    sf::RectangleShape grassShape(sf::Vector2f(Constants::TILE_SIZE, 6.0f));
                    grassShape.setPosition(tilePos);
                    grassShape.setFillColor(sf::Color(46, 139, 87));
                    target.draw(grassShape);
                }
            }
        }
    }

    // 2. Draw all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(target);
        }
    }

    // 3. Draw entity death effects (flying trajectories) and impact particles.
    // Both are world-space, so they must be drawn while the camera view is still active.
    EntityDeathEffect::getInstance().render(target);
    target.draw(ParticleSystem::getInstance());

    // 4. Draw AABB overlays (dev toggle)
    if (m_showAABB) {
        // Tile grid AABBs
        for (int y = 0; y < m_tileMap.getHeight(); ++y) {
            for (int x = 0; x < m_tileMap.getWidth(); ++x) {
                TileType tt = m_tileMap.getTileType(x, y);
                if (tt == TileType::Empty) continue;
                sf::RectangleShape dbg(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                dbg.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                dbg.setFillColor(sf::Color::Transparent);
                dbg.setOutlineColor(tt == TileType::Ground ? sf::Color(0, 255, 0, 180) : sf::Color(255, 255, 0, 180));
                dbg.setOutlineThickness(1.0f);
                target.draw(dbg);
            }
        }
        // Entity AABBs
        for (auto& entity : m_entities) {
            if (!entity || !entity->isActive()) continue;
            AABB bb = entity->getBoundingBox();
            sf::RectangleShape dbg(sf::Vector2f(bb.width, bb.height));
            dbg.setPosition(sf::Vector2f(bb.x, bb.y));
            dbg.setFillColor(sf::Color::Transparent);
            sf::Color outlineColor = sf::Color(0, 255, 0, 220);   // Green = player/generic
            if (dynamic_cast<Enemy*>(entity.get()))  outlineColor = sf::Color(255, 50, 50, 220);   // Red
            else if (dynamic_cast<Item*>(entity.get()))  outlineColor = sf::Color(255, 220, 0, 220);   // Yellow
            else if (dynamic_cast<Block*>(entity.get())) outlineColor = sf::Color(0, 220, 255, 220);  // Cyan
            dbg.setOutlineColor(outlineColor);
            dbg.setOutlineThickness(1.5f);
            target.draw(dbg);
        }
    }

    // Draw the screen-space HUD overlay
    if (m_hud) {
        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());
        target.draw(*m_hud);

        // Minimap is screen-space and manages its own visibility via MinimapToggled
        if (m_minimap) {
            target.draw(*m_minimap);
        }

        if (m_rewindManager.isRewinding()) {
            // Full-screen cyan vignette scanline filter overlay
            sf::RectangleShape rewindOverlay(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
            rewindOverlay.setFillColor(sf::Color(0, 200, 255, 45));
            target.draw(rewindOverlay);

            sf::RectangleShape rewindBox(sf::Vector2f(420.0f, 44.0f));
            rewindBox.setPosition({430.0f, 65.0f});
            rewindBox.setFillColor(sf::Color(0, 0, 0, 220));
            rewindBox.setOutlineColor(sf::Color(0, 255, 255));
            rewindBox.setOutlineThickness(2.0f);
            target.draw(rewindBox);

            sf::Text rewindText(ResourceManager::getInstance().getFont("PressStart2P"));
            rewindText.setString("<< MEMENTO TIME REWINDING (" + std::to_string(m_rewindManager.getSnapshotCount()) + ")");
            rewindText.setCharacterSize(12);
            rewindText.setFillColor(sf::Color(0, 255, 255));
            rewindText.setPosition({445.0f, 80.0f});
            target.draw(rewindText);
        }

        target.setView(oldView);
    }

    // Render screen transitions overlay
    ScreenTransitionManager::getInstance().render(target);

    // ImGui Panel for controlling and monitoring the gameplay simulation
    ImGui::Begin("Gameplay Controls & Navigation");
    ImGui::Text("Simulation State:");
    if (m_player) {
        ImGui::Text("Player Position: (%.1f, %.1f)", m_player->getPosition().x, m_player->getPosition().y);
        ImGui::Text("Player Velocity: (%.1f, %.1f)", m_player->getVelocity().x, m_player->getVelocity().y);
        ImGui::Text("Lives: %d | Coins: %d | Score: %d", m_player->getLives(), m_player->getCoins(), m_player->getScore());
    } else {
        ImGui::Text("No active player.");
    }
    ImGui::Separator();
    ImGui::Text("Select Campaign Level:");
    const char* campaignLevels[] = {
        "World 1-1: Grassland Overworld",
        "World 1-1 Sub: Underground Vault",
        "World 1-2: Ice Cavern Path",
        "World 1-2 Sub: Sky Platform Canopy",
        "World 1-3: Bowser's Castle Fortress",
        "World 1-3 Sub: Secret Castle Vault",
        "Bonus Stage 1: Coin Paradise"
    };
    int oldNavLevel = m_selectedLevelIndex;
    if (ImGui::Combo("Select Level", &m_selectedLevelIndex, campaignLevels, 7)) {
        if (m_selectedLevelIndex != oldNavLevel || m_isProcedural) {
            m_isProcedural = false;
            setupTestScene();
        }
    }

    ImGui::Separator();
    ImGui::Text("Active Level Tube / Warp Pipe Destinations:");

    std::vector<Pipe*> activePipes;
    std::vector<std::string> pipeLabels;
    for (const auto& entity : m_entities) {
        if (auto pipe = dynamic_cast<Pipe*>(entity.get())) {
            activePipes.push_back(pipe);
            std::string label = "Tube @" + std::to_string(static_cast<int>(pipe->getPosition().x / Constants::TILE_SIZE)) +
                                " -> " + (pipe->getTargetLevel().empty() ? "Same Level Teleport" : pipe->getTargetLevel());
            pipeLabels.push_back(label);
        }
    }

    static int selectedPipeIndex = 0;
    if (activePipes.empty()) {
        ImGui::TextDisabled("No active warp tubes in current level.");
    } else {
        if (selectedPipeIndex >= static_cast<int>(activePipes.size())) selectedPipeIndex = 0;
        
        std::vector<const char*> pipeItems;
        for (const auto& l : pipeLabels) pipeItems.push_back(l.c_str());

        ImGui::Combo("Tube Dropdown", &selectedPipeIndex, pipeItems.data(), static_cast<int>(pipeItems.size()));
        ImGui::SameLine();
        if (ImGui::Button("🌀 Enter Tube")) {
            Pipe* targetPipe = activePipes[selectedPipeIndex];
            if (m_player) {
                SoundManager::getInstance().playSound("pipe");
                std::string targetLevel = targetPipe->getTargetLevel();
                sf::Vector2f exitPos = targetPipe->getExitPosition();
                if (!targetLevel.empty()) {
                    loadLevelByPath(targetLevel, exitPos);
                } else if (exitPos.x != 0.0f || exitPos.y != 0.0f) {
                    m_player->setPosition(exitPos);
                    m_player->setVelocity({0.0f, 0.0f});
                }
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("🏠 Return to Main Menu")) {
        Game::getInstance().changeState(std::make_unique<MenuState>());
    }

    ImGui::SameLine();
    if (ImGui::Button("🛠️ Toggle Map Editor (F1)")) {
        m_mapEditor.toggleActive();
    }
    ImGui::End();


    if (m_isProcedural) {
        ImGui::Begin("Procedural Level Generator Tuning");
        ImGui::Text("Live tuning parameters:");

        int themeIdx = static_cast<int>(m_genConfig.theme);
        const char* themes[] = { "Overworld", "Underground", "Castle", "Ice" };
        if (ImGui::Combo("Theme", &themeIdx, themes, 4)) {
            m_genConfig.theme = static_cast<MapTheme>(themeIdx);
        }

        int diffIdx = static_cast<int>(m_genConfig.difficulty);
        const char* diffs[] = { "Easy", "Medium", "Hard" };
        if (ImGui::Combo("Difficulty", &diffIdx, diffs, 3)) {
            m_genConfig.difficulty = static_cast<MapDifficulty>(diffIdx);
        }

        ImGui::SliderFloat("Roughness", &m_genConfig.roughness, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Pit Ratio", &m_genConfig.pitProbability, 0.0f, 0.35f, "%.2f");
        ImGui::SliderFloat("Enemy Rate", &m_genConfig.enemySpawnRate, 0.0f, 0.40f, "%.2f");
        ImGui::Checkbox("Castle Lava Hazards", &m_genConfig.enableLava);
        ImGui::Checkbox("Moving Platforms", &m_genConfig.enableMovingPlatforms);

        int seedVal = static_cast<int>(m_genConfig.seed);
        if (ImGui::InputInt("Seed (0=Random)", &seedVal)) {
            m_genConfig.seed = (seedVal < 0) ? 0 : static_cast<unsigned int>(seedVal);
        }
        ImGui::SameLine();
        if (ImGui::Button("🎲 New Seed")) {
            m_genConfig.seed = std::random_device{}();
        }

        if (ImGui::Button("🔄 Regenerate Level")) {
            MapGenerator::generate(m_tileMap, m_entities, m_genConfig);
            for (auto& entity : m_entities) {
                wireEntityAnimations(entity.get());
            }
            m_player = nullptr;
            for (const auto& entity : m_entities) {
                if (auto p = dynamic_cast<Player*>(entity.get())) {
                    m_player = p;
                    break;
                }
            }
            if (m_player) {
                InputManager::getInstance().registerPlayer(m_player, 0);
                Game::getInstance().setPlayer(m_player);
            }
            m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});
        }
        ImGui::End();
    }

    // Draw Map Editor overlays if active

    if (m_mapEditor.isActive()) {
        m_mapEditor.render(target, m_tileMap, m_entities, &m_camera);
        m_mapEditor.renderImGui(m_tileMap, m_entities);
    } else {
        // Reset view to default view for static screen space rendering (HUD, ImGui overlays)
        target.setView(target.getDefaultView());

        // --- ImGui Dev Interface for Phase 2, 3 & 4 (Playground) ---
        ImGui::Begin("Physics & Input Test Playground (Phase 2 & 3)");

        // Character Selection
        ImGui::Text("Select Active Character:");
        const char* characters[] = { "Mario (Red)", "Luigi (Green)", "Toad (Blue)", "Peach (Pink)" };
        int oldCharSelected = m_selectedCharIndex;
        if (ImGui::Combo("Character", &m_selectedCharIndex, characters, 4)) {
            if (m_selectedCharIndex != oldCharSelected && m_player) {
                spawnSelectedPlayer(m_player->getPosition());
            }
        }

        // Level Selection
        ImGui::Text("Select Active Level:");
        const char* levels[] = {
            "World 1-1: Grassland Overworld",
            "World 1-1 Sub: Underground Vault",
            "World 1-2: Ice Cavern Path",
            "World 1-2 Sub: Sky Platform Canopy",
            "World 1-3: Bowser's Castle Fortress",
            "World 1-3 Sub: Secret Castle Vault",
            "Bonus Stage 1: Coin Paradise"
        };
        int oldLevelSelected = m_selectedLevelIndex;
        if (ImGui::Combo("Level", &m_selectedLevelIndex, levels, 7)) {
            if (m_selectedLevelIndex != oldLevelSelected || m_isProcedural) {
                m_isProcedural = false;
                setupTestScene();
            }
        }


        ImGui::Separator();

        if (m_player) {
            ImGui::Text("Character Stats:");
            ImGui::BulletText("Position: (%.2f, %.2f)", m_player->getPosition().x, m_player->getPosition().y);
            ImGui::BulletText("Velocity: (%.2f, %.2f)", m_player->getVelocity().x, m_player->getVelocity().y);
            ImGui::BulletText("onGround: %s", m_player->isOnGround() ? "TRUE" : "FALSE");
            ImGui::BulletText("onWall: %s", m_player->isOnWall() ? "TRUE" : "FALSE");
            ImGui::BulletText("Crouched: %s", m_player->isCrouched() ? "TRUE" : "FALSE");
            ImGui::BulletText("Sliding: %s", m_player->isSliding() ? "TRUE" : "FALSE");
            ImGui::BulletText("Coyote Frames Left: %d", m_player->getCoyoteFramesLeft());
            ImGui::BulletText("Jump Buffer Frames Left: %d", m_player->getJumpBufferFramesLeft());
            ImGui::BulletText("Lives: %d, Coins: %d, Score: %d", m_player->getLives(), m_player->getCoins(), m_player->getScore());
            
            std::string stateName = "Unknown";
            if (IPlayerState* state = m_player->getCurrentState()) {
                IPlayerState* baseState = state;
                bool invincible = false;
                bool mega = false;
                while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
                    if (dynamic_cast<StarDecorator*>(decorator)) invincible = true;
                    if (dynamic_cast<MegaDecorator*>(decorator)) mega = true;
                    baseState = decorator->getWrappedState();
                }

                if (dynamic_cast<SmallState*>(baseState)) stateName = "Small";
                else if (dynamic_cast<SuperState*>(baseState)) stateName = "Super";
                else if (dynamic_cast<FireState*>(baseState)) stateName = "Fire";
                else if (dynamic_cast<CapeState*>(baseState)) stateName = "Cape";
                else if (dynamic_cast<MiniState*>(baseState)) stateName = "Mini";

                if (invincible) stateName += " + Star (Invincible)";
                if (mega) stateName += " + Mega (Giant)";
            }
            ImGui::BulletText("Active Form: %s", stateName.c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No active player.");
        }

        ImGui::Separator();
        
        // Spawner buttons
        ImGui::Text("Spawn items at player position:");
        if (m_player) {
            sf::Vector2f spawnPos = m_player->getPosition() + sf::Vector2f(0.0f, -64.0f);
            if (ImGui::Button("Spawn Mushroom")) {
                m_entities.push_back(std::make_unique<Mushroom>(spawnPos));
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Fire Flower")) {
                m_entities.push_back(std::make_unique<FireFlower>(spawnPos));
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Coin")) {
                m_entities.push_back(std::make_unique<Coin>(spawnPos));
            }
            if (ImGui::Button("Spawn Star")) {
                m_entities.push_back(std::make_unique<Star>(spawnPos));
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Cape Feather")) {
                m_entities.push_back(std::make_unique<CapeFeather>(spawnPos));
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Mega Mushroom")) {
                m_entities.push_back(std::make_unique<MegaMushroom>(spawnPos));
            }
            if (ImGui::Button("Spawn Mini Mushroom")) {
                m_entities.push_back(std::make_unique<MiniMushroom>(spawnPos));
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Swap Bricks <-> Coins (P-Switch test)")) {
            m_tileMap.swapBricksAndCoins();
        }

        ImGui::Separator();

        ImGui::Checkbox("Show Physics AABB Overlays", &m_showAABB);

        ImGui::Separator();

        if (ImGui::Button("Reset Simulation")) {
            setupTestScene();
        }

        ImGui::End();

        // --- ImGui Dev Interface for Phase 8 (Save/Load & Persistence) ---
        ImGui::Begin("Save/Load & Persistence (Phase 8)");

        Player* player = (!m_entities.empty() && m_entities[0]) ? dynamic_cast<Player*>(m_entities[0].get()) : nullptr;
        if (player) {
            ImGui::Text("Active Slot: %d", Game::getInstance().getActiveSlot());
            ImGui::Text("Active Character: %s", (dynamic_cast<Mario*>(player) ? "Mario" : (dynamic_cast<Luigi*>(player) ? "Luigi" : "Other")));
            ImGui::Text("Lives: %d | Coins: %d | Score: %d", player->getLives(), player->getCoins(), player->getScore());
            ImGui::Text("Position: (%.1f, %.1f)", player->getPosition().x, player->getPosition().y);
        } else {
            ImGui::Text("No active player character loaded.");
        }
        
        ImGui::Separator();

        // 1. Settings Persistence Tab
        if (ImGui::CollapsingHeader("Settings Configuration")) {
            float sfx = Game::getInstance().getSfxVolume();
            float music = Game::getInstance().getMusicVolume();
            bool colorblind = Game::getInstance().getColorblindMode();

            if (ImGui::SliderFloat("SFX Volume", &sfx, 0.0f, 100.0f)) {
                Game::getInstance().setSfxVolume(sfx);
            }
            if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 100.0f)) {
                Game::getInstance().setMusicVolume(music);
            }
            if (ImGui::Checkbox("Colorblind Mode", &colorblind)) {
                Game::getInstance().setColorblindMode(colorblind);
            }

            std::string diff = Game::getInstance().getDifficulty();
            const char* difficulties[] = { "easy", "normal", "hard" };
            int activeDiffIdx = 0;
            for (int i = 0; i < 3; ++i) {
                if (diff == difficulties[i]) activeDiffIdx = i;
            }
            if (ImGui::Combo("Difficulty", &activeDiffIdx, difficulties, 3)) {
                Game::getInstance().setDifficulty(difficulties[activeDiffIdx]);
            }
        }

        // 2. Save Slots Persistence Tab
        if (ImGui::CollapsingHeader("Save/Load Slots")) {
            for (int slot = 1; slot <= 3; ++slot) {
                ImGui::PushID(slot);
                SaveSlotPreview preview = Serializer::getSlotPreview(slot);
                if (preview.exists) {
                    ImGui::Text("Slot %d: [%s] Lvl:%d (%s) Score:%d, Star Coins:%d, Play Time:%.1fs, Saved:%s",
                                slot, preview.character.c_str(), preview.levelId, preview.levelName.c_str(),
                                preview.score, preview.starCoinsCount, preview.playTime, preview.timestamp.c_str());
                } else {
                    ImGui::Text("Slot %d: Empty", slot);
                }

                if (ImGui::Button("Save")) {
                    if (player) {
                        Serializer::saveGame(slot, *player, 1, "Level 1", Constants::LEVEL_TIME, player->getPosition().x, player->getPosition().y, std::vector<bool>(m_starCoinsCollected.begin(), m_starCoinsCollected.end()));
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    std::unique_ptr<Player> loadedPlayer;
                    int lvlId;
                    std::string lvlName;
                    float timeRem, checkX, checkY;
                    std::vector<bool> starCoins;
                    bool success = Serializer::loadGame(slot, loadedPlayer, lvlId, lvlName, timeRem, checkX, checkY, starCoins);
                    if (success && loadedPlayer) {
                        if (starCoins.size() >= 3) {
                            m_starCoinsCollected = {starCoins[0], starCoins[1], starCoins[2]};
                        }
                        // adoptPlayer refreshes m_player, InputManager and Game.
                        // Assigning m_entities[0] directly here left m_player dangling.
                        adoptPlayer(std::move(loadedPlayer));
                        Game::getInstance().setActiveSlot(slot);
                        std::cout << "Loaded save slot " << slot << " successfully!" << std::endl;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    Serializer::deleteSlot(slot);
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }

        // 3. Statistics Persistence Tab
        if (ImGui::CollapsingHeader("Session & Overall Statistics")) {
            const auto& stats = StatisticsTracker::getInstance().getStats();
            ImGui::Text("Total Enemies Defeated: %d", stats.totalEnemiesDefeated);
            ImGui::Text("Total Coins Collected: %d", stats.totalCoinsCollected);
            ImGui::Text("Total Deaths: %d", stats.totalDeaths);
            ImGui::Text("Total Time Played: %.1fs", stats.totalTimePlayed);
            ImGui::Text("Highest Combo: %d", stats.highestCombo);
            if (ImGui::Button("Reset Stats")) {
                StatisticsTracker::getInstance().reset();
            }
        }

        // 4. Achievements Persistence Tab
        if (ImGui::CollapsingHeader("Achievements Monitor")) {
            const auto& achievements = AchievementManager::getInstance().getAchievements();
            int unlockedCount = 0;
            for (const auto& a : achievements) {
                if (a.unlocked) unlockedCount++;
            }
            ImGui::Text("Unlocked: %d / %d", unlockedCount, static_cast<int>(achievements.size()));
            ImGui::Separator();
            for (const auto& a : achievements) {
                ImGui::Text("[%s] %s: %s (%s)",
                            (a.unlocked ? "UNLOCKED" : "LOCKED"),
                            a.name.c_str(),
                            a.condition.c_str(),
                            a.icon.c_str());
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Reset Game Data (Lock all & wipe slot 1)")) {
            AchievementManager::getInstance().reset();
            StatisticsTracker::getInstance().reset();
            Serializer::deleteSlot(1);
        }
        
        ImGui::TextDisabled("\nSimulation Keyboard Testing Commands:\n"
                            "- Num1: Collect coin     - Num2: Defeat enemy\n"
                            "- Num3: Take damage      - Num4: Lose life/Die\n"
                            "- Num5: Cross Checkpoint - Num6: Clear Level 3\n"
                            "- Num7: Defeat Bowser    - Num8: Collect star coin\n"
                            "- Num9: Find hidden block");

        ImGui::End();

        // --- Overlay Toast Notifications ---
        const auto& toasts = AchievementManager::getInstance().getActiveToasts();
        if (!toasts.empty()) {
            ImGui::SetNextWindowPos(ImVec2(Constants::WINDOW_WIDTH - 320.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300.0f, 80.0f * toasts.size()), ImGuiCond_Always);
            ImGui::Begin("Achievements Toasts Overlay", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);
            
            for (const auto& toast : toasts) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, toast.alpha);
                ImGui::BeginChild(toast.id.c_str(), ImVec2(290, 70), true);
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Achievement Unlocked!");
                ImGui::Text("[%s] %s", toast.icon.c_str(), toast.name.c_str());
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
    }
}

void PlayingState::setupTestScene() {
    cleanupTestScene();

    LevelLoader loader;
    LevelData levelData;
    std::string levelPath = "assets/levels/level_1.json";
    if (m_selectedLevelIndex == 0) levelPath = "assets/levels/level_1.json";
    else if (m_selectedLevelIndex == 1) levelPath = "assets/levels/level_1_sub.json";
    else if (m_selectedLevelIndex == 2) levelPath = "assets/levels/level_2.json";
    else if (m_selectedLevelIndex == 3) levelPath = "assets/levels/level_2_sub.json";
    else if (m_selectedLevelIndex == 4) levelPath = "assets/levels/level_3.json";
    else if (m_selectedLevelIndex == 5) levelPath = "assets/levels/level_3_sub.json";
    else if (m_selectedLevelIndex == 6) levelPath = "assets/levels/bonus_1.json";


    std::vector<std::string> pathCandidates = {
        levelPath,
        "SuperMarioGame/" + levelPath,
        "../" + levelPath
    };
    std::string chosenPath = levelPath;
    for (const auto& candidate : pathCandidates) {
        if (std::filesystem::exists(candidate)) {
            chosenPath = candidate;
            break;
        }
    }

    if (loader.loadLevel(chosenPath, m_tileMap, levelData)) {
        // Spawn active player character at the spawnPoint loaded from JSON
        spawnSelectedPlayer(levelData.spawnPoint);
        
        // Transfer all loaded items/entities and wire their animations
        for (auto& entity : levelData.entities) {
            wireEntityAnimations(entity.get());
            m_entities.push_back(std::move(entity));
        }

        // Set camera bounds matching the level size
        m_camera.setBounds(AABB{0.0f, 0.0f, levelData.width * Constants::TILE_SIZE, levelData.height * Constants::TILE_SIZE});
    } else {
        // Fallback: manually setup scene if file loading fails
        m_tileMap.initialize(40, 22);
        for (int x = 0; x < 40; ++x) {
            m_tileMap.setTile(x, 20, TileType::Ground);
            m_tileMap.setTile(x, 21, TileType::Ground);
        }
        spawnSelectedPlayer(sf::Vector2f(100.0f, 100.0f));
        m_camera.setBounds(AABB{0.0f, 0.0f, 40.0f * Constants::TILE_SIZE, 22.0f * Constants::TILE_SIZE});
    }
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
    m_player = nullptr;
}

bool PlayingState::loadLevelByPath(const std::string& jsonPath, sf::Vector2f spawnOverride) {
    LevelLoader loader;
    LevelData levelData;

    std::vector<std::string> pathCandidates = {
        jsonPath,
        "SuperMarioGame/" + jsonPath,
        "../" + jsonPath
    };
    std::string chosenPath = jsonPath;
    for (const auto& candidate : pathCandidates) {
        if (std::filesystem::exists(candidate)) {
            chosenPath = candidate;
            break;
        }
    }

    if (!loader.loadLevel(chosenPath, m_tileMap, levelData)) {
        std::cerr << "[PlayingState] Failed to load level: " << jsonPath << std::endl;
        return false;
    }

    int savedLives = Constants::INITIAL_LIVES;
    int savedCoins = 0;
    int savedScore = 0;
    if (m_player) {
        savedLives = m_player->getLives();
        savedCoins = m_player->getCoins();
        savedScore = m_player->getScore();
    }

    cleanupTestScene();

    m_entities = std::move(levelData.entities);

    for (auto& entity : m_entities) {
        wireEntityAnimations(entity.get());
    }

    sf::Vector2f spawnPos = (spawnOverride.x != 0.0f || spawnOverride.y != 0.0f) ? spawnOverride : levelData.spawnPoint;

    // cleanupTestScene() nulled m_player, so spawnSelectedPlayer cannot carry the
    // stats itself — apply them here, through the same silent path.
    spawnSelectedPlayer(spawnPos);
    if (m_player) {
        m_player->restoreStats(savedLives, savedCoins, savedScore);
    }

    Game::getInstance().setTileMap(&m_tileMap);
    m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});

    std::cout << "[PlayingState] Loaded sub-level / main level: " << chosenPath << " at spawn (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return true;
}


void PlayingState::adoptPlayer(std::unique_ptr<Player> player) {
    if (!player) return;

    // Drop any previous player entity first so the vector never holds two.
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
                           [this](const std::unique_ptr<Entity>& e) { return e.get() == m_player; });
    if (it != m_entities.end()) {
        m_entities.erase(it);
    }

    m_player = player.get();
    m_entities.insert(m_entities.begin(), std::move(player));

    // Refresh every observer holding a raw Player*. Skipping any one of these is
    // what made the save-slot Load button a use-after-free.
    InputManager::getInstance().registerPlayer(m_player, 0);
    Game::getInstance().setPlayer(m_player);
    wireEntityAnimations(m_player);
}

void PlayingState::spawnSelectedPlayer(const sf::Vector2f& pos) {
    int oldCoins = 0;
    int oldScore = 0;
    int oldLives = Constants::INITIAL_LIVES;
    if (m_player) {
        oldCoins = m_player->getCoins();
        oldScore = m_player->getScore();
        oldLives = m_player->getLives();
    }

    std::unique_ptr<Player> newPlayer;
    if (m_selectedCharIndex == 0) {
        newPlayer = std::make_unique<Mario>(pos);
    } else if (m_selectedCharIndex == 1) {
        newPlayer = std::make_unique<Luigi>(pos);
    } else if (m_selectedCharIndex == 2) {
        newPlayer = std::make_unique<Toad>(pos);
    } else {
        newPlayer = std::make_unique<Peach>(pos);
    }

    // Carry stats across the swap silently. addCoins()/loseLife() would publish
    // CoinCollected and GameOver here, inflating statistics and achievements.
    newPlayer->restoreStats(oldLives, oldCoins, oldScore);

    adoptPlayer(std::move(newPlayer));
}

void PlayingState::wireEntityAnimations(Entity* entity) {
    if (!entity) return;

    // Route entity to its matching sprite sheet atlas based on type hierarchy.
    // setupAnimations() lives on Player, Enemy, Item, Block — not on Entity base.
    if (auto* p = dynamic_cast<Player*>(entity)) {
        if (m_playerSheet) p->setupAnimations(m_playerSheet.get());
    } else if (auto* e = dynamic_cast<Enemy*>(entity)) {
        if (m_enemySheet) e->setupAnimations(m_enemySheet.get());
    } else if (dynamic_cast<StarCoin*>(entity)) {
        // StarCoin's big_coin_0/1/2 frames live in world_scenery_item, not item atlas
        if (auto* sc = dynamic_cast<Item*>(entity)) {
            if (m_scenerySheet) sc->setupAnimations(m_scenerySheet.get());
        }
    } else if (auto* i = dynamic_cast<Item*>(entity)) {
        if (m_itemSheet) i->setupAnimations(m_itemSheet.get());
    } else if (auto* b = dynamic_cast<Block*>(entity)) {
        if (m_scenerySheet) b->setupAnimations(m_scenerySheet.get());
    } else {
        // Fallback: unknown entity type — silently skip
        std::cerr << "[wireEntityAnimations] Unknown entity type, skipping animation setup." << std::endl;
    }
}

