#define NOMINMAX
#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/PauseState.hpp"
#include "Core/VictoryState.hpp"
#include "Core/GameOverState.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/CampaignProgress.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Projectile.hpp"
#include "Core/DifficultyStrategy.hpp"
#include "Utils/MetaGame.hpp"
#include "Core/DebugConsole.hpp"
#include "Core/ReplayRecorder.hpp"
#include "Core/Game.hpp"
#include "Core/ResourceManager.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/ScreenTransitionManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Graphics/EntityDeathEffect.hpp"
#include "Graphics/ColorPalette.hpp"
#include "Graphics/UiRenderer.hpp"
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
#include "Entities/OneUpMushroom.hpp"
#include "Entities/QuestionBlock.hpp"
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


PlayingState::PlayingState(bool startInEditor, bool isProcedural, const MapGeneratorConfig& genConfig,
                           int characterIndex, int levelIndex, bool twoPlayer)
    : m_selectedCharIndex(characterIndex),
      m_selectedLevelIndex(LevelCatalog::isValidIndex(levelIndex) ? levelIndex : 0),
      m_startInEditor(startInEditor), m_isProcedural(isProcedural), m_twoPlayer(twoPlayer),
      m_genConfig(genConfig) {}

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

    // Lead the player by up to a third of a screen at full run speed. Enough to
    // see what is coming; more than this and the player sits at the screen edge.
    m_camera.setLookahead(140.0f);

    m_playerSheet  = tryLoadSheet("player");
    m_enemySheet   = tryLoadSheet("enemy_projectile");
    m_itemSheet    = tryLoadSheet("item");
    m_scenerySheet = tryLoadSheet("world_scenery_item");
    // The backdrop draws from the same world atlas the tiles come from.
    m_background.setSpriteSheet(m_scenerySheet.get());
    // Note: tileset_blocks is deprecated — tile sprites come from world_scenery_item (m_scenerySheet)

    // Initialize HUD and Level Timer
    m_hud = std::make_unique<Hud>(sf::Vector2i(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT), m_itemSheet.get(), m_playerSheet.get());
    m_levelTimer = Constants::LEVEL_TIME * Game::getInstance().difficulty().levelTimeScale();
    m_tileAnimTimer = 0.0f;

    if (m_isProcedural) {
        cleanupTestScene();
        MapGenerator::generate(m_tileMap, m_entities, m_genConfig);

        // Wire animations for all procedurally generated entities
        for (auto& entity : m_entities) {
            admitEntity(entity.get());
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
            m_levelSpawnPoint = sf::Vector2f(96.0f, 64.0f);
        } else {
            m_levelSpawnPoint = m_player->getPosition();
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
        // Remember where to respawn. Death used to teleport to a hardcoded
        // (96,64) regardless of level or progress (audit G-3).
        if (m_player) {
            m_checkpointPosition = m_player->getPosition();
            m_hasCheckpoint = true;
        }
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

            auto fireball = m_fireballPool.acquire(spawnPos, vel);
            // Was pushed straight onto the list, so it never had its animations
            // wired and fell back to hand-drawn circles every time.
            admitEntity(fireball.get());
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

    // --- Question blocks: actually produce the item they announce ---
    m_powerUpSubId = bus.subscribe(EventType::PowerUpRequested, [this](const GameEvent& ev) {
        if (!ev.data.has_value() || ev.data.type() != typeid(PowerUpRequest)) return;
        const auto request = std::any_cast<PowerUpRequest>(ev.data);

        // The one place QuestionBlock::Content is translated into an entity.
        std::unique_ptr<Entity> item;
        switch (request.itemType) {
            case QuestionBlock::FireFlower:   item = std::make_unique<FireFlower>(request.spawnPosition);   break;
            case QuestionBlock::CapeFeather:  item = std::make_unique<CapeFeather>(request.spawnPosition);  break;
            case QuestionBlock::Star:         item = std::make_unique<Star>(request.spawnPosition);         break;
            case QuestionBlock::MiniMushroom: item = std::make_unique<MiniMushroom>(request.spawnPosition); break;
            case QuestionBlock::MegaMushroom: item = std::make_unique<MegaMushroom>(request.spawnPosition); break;
            case QuestionBlock::OneUp:        item = std::make_unique<OneUpMushroom>(request.spawnPosition);break;
            case QuestionBlock::Mushroom:
            default:                          item = std::make_unique<Mushroom>(request.spawnPosition);     break;
        }
        admitEntity(item.get());
        m_entities.push_back(std::move(item));
    });

    // --- Entities asking for other entities (Lakitu's Spinies, Hammer Bro's
    // hammers). Entities have no handle on the world list, so they publish and
    // this performs the spawn (audit B-6, B-7). ---
    m_entitySpawnSubId = bus.subscribe(EventType::EntitySpawnRequested, [this](const GameEvent& ev) {
        if (!ev.data.has_value() || ev.data.type() != typeid(EntitySpawnRequest)) return;
        const auto request = std::any_cast<EntitySpawnRequest>(ev.data);

        // Keep a lid on projectiles so a long fight cannot flood the world.
        if (m_entities.size() >= 400) return;

        auto spawned = spawnProjectile(request.type, request.position, request.velocity);
        if (!spawned) return;
        spawned->setVelocity(request.velocity);
        admitEntity(spawned.get());
        m_entities.push_back(std::move(spawned));
    });

    // --- Level complete: the flagpole fires this; without a listener the game
    // could be flagged but never actually finished (audit G-1). ---
    m_levelCompleteSubId = bus.subscribe(EventType::LevelComplete, [this](const GameEvent&) {
        if (m_levelComplete) return;   // flagpole can fire more than once
        m_levelComplete = true;
        m_levelCompleteTimer = 0.0f;
        std::cout << "[PlayingState] Level complete!" << std::endl;
    });

    // --- Checkpoint: remember where to respawn, then auto-save ---
    m_checkpointPosition = m_player ? m_player->getPosition() : sf::Vector2f(96.0f, 64.0f);
    m_hasCheckpoint = false;

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
                          &m_coinParticleSubId, &m_playerDamagedSubId,
                          &m_powerUpSubId, &m_levelCompleteSubId,
                          &m_entitySpawnSubId }) {
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
            // Escape (or P) opens the pause overlay. Escape used to quit the
            // whole process from Game::run(); the pause menu owns that choice now.
            if (keyPressed->code == sf::Keyboard::Key::Escape ||
                keyPressed->code == sf::Keyboard::Key::P) {
                Game::getInstance().pushState(std::make_unique<PauseState>(
                    [this]() { restartLevel(); },
                    [this]() { saveToSlot(Game::getInstance().getActiveSlot()); },
                    []() {
                        ScreenTransitionManager::getInstance().reset();
                        Game::getInstance().changeState(std::make_unique<MenuState>());
                    }));
                return;
            }

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
    // Apply anything the dev panels requested while rendering the previous frame.
    // Doing it here, before the simulation runs, keeps those mutations on the
    // fixed-timestep cadence rather than the render cadence (audit A-9).
    m_devPanel.flush(*this);

    // Advance tile animation timer
    m_tileAnimTimer += dt;

    if (m_mapEditor.isActive()) {
        sf::Vector2f mouseWorldPos = Game::getInstance().getMouseWorldPosition(m_camera.getView());
        m_mapEditor.update(m_tileMap, m_entities, mouseWorldPos, dt, &m_camera);
        m_camera.update(dt);
        return;
    }

    // The map editor unclamps the camera so the designer can pan off-map. It is
    // only updated while active, so it cannot restore the flag itself — gameplay
    // re-asserts the invariant here instead (audit C-3).
    if (!m_camera.isBoundsEnabled()) {
        m_camera.setBoundsEnabled(true);
    }

    // 1. Update trackers
    StatisticsTracker::getInstance().update(dt);
    AchievementManager::getInstance().update(dt);

    // 0a. Replay playback. Applied instead of simulating: physics and AI stay
    // switched off for the frame, so what is shown is what was recorded rather
    // than a re-simulation that would drift.
    if (ReplayRecorder::getInstance().isPlaying()) {
        if (const GameSnapshot* frame = ReplayRecorder::getInstance().advance()) {
            applySnapshot(*frame);
            m_camera.update(dt);
            m_background.update(dt);
            m_tileAnimTimer += dt;
            return;
        }
        std::cout << "[Replay] Playback finished." << std::endl;
    }

    // 0. Time Rewind check (Hold R or Left/Right Shift keys to rewind time using Memento snapshots)
    // R only. LShift is bound to RunCommand for Player 1, so accepting it here
    // meant holding Shift to run rewound time instead — running was unusable
    // (audit B-10).
    bool rewindRequested = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

    if (rewindRequested && m_rewindManager.hasSnapshots()) {
        m_rewindManager.setRewinding(true);
        applySnapshot(m_rewindManager.popSnapshot());
        return; // Skip forward physics integration while rewinding
    }

    m_rewindManager.setRewinding(false);
    // The snapshot for this frame is recorded at the end of update(), once
    // physics has run. Recording here as well used to double the rate and halve
    // the effective rewind window.

    // 2. Process held keys (MoveLeft, MoveRight, Crouch, Run).
    //
    // These are polled from the keyboard rather than driven by events, so the
    // console's event filter does not cover them: typing "difficulty hard" would
    // otherwise walk the player right on every "d".
    if (!DebugConsole::getInstance().isVisible()) {
        if (m_player)  InputManager::getInstance().update(*m_player);
        if (m_player2) InputManager::getInstance().update(*m_player2);
    }

    // 3. Update all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->update(dt);
        }
    }

    // 3. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 3b. Prune inactive entities (keep m_player intact even if inactive).
    //
    // stable_partition rather than remove_if: remove_if leaves the tail in a
    // moved-from state, so the dead objects are already gone by the time we
    // could offer them to a pool. Partitioning permutes instead, so the tail
    // holds the real entities and pooled types can be recycled (task 10.1).
    auto deadBegin = std::stable_partition(
        m_entities.begin(), m_entities.end(), [this](const std::unique_ptr<Entity>& entity) {
            const bool dead = entity && !entity->isActive() && entity.get() != m_player;
            return !dead;
        });
    for (auto it = deadBegin; it != m_entities.end(); ++it) {
        recycleEntity(std::move(*it));
    }
    m_entities.erase(deadBegin, m_entities.end());

    // 3b2. Lava burns. A Lava tile is not solid — you fall into it, you do not
    // stand on it — so nothing in the physics engine would ever have noticed it.
    // Checked against the player's feet, which is the part that touches first.
    if (m_player && !m_levelComplete) {
        const AABB box = m_player->getBoundingBox();
        const TileType underfoot = m_tileMap.getTileAt(box.x + box.width * 0.5f,
                                                       box.y + box.height - 2.0f);
        if (underfoot == TileType::Lava) {
            // takeDamage steps the form down and grants i-frames, so standing in
            // lava does not drain every life in one frame; Small Mario touching
            // it still dies outright, which is the intent.
            m_player->takeDamage(1);
            if (m_player->getLives() <= 0) {
                killPlayer("fell into the lava");
                return;
            }
        }
    }

    // 3c. Void fall — one shared death path, so the timer and the pit agree.
    const float bottomVoidY = (m_tileMap.getHeight() * Constants::TILE_SIZE) + 32.0f;
    if (m_player && m_player->getPosition().y > bottomVoidY) {
        killPlayer("fell into the void");
        if (allPlayersOut()) return;
    }
    // Player 2 falls into the same void.
    if (m_player2 && m_player2->getPosition().y > bottomVoidY) {
        SoundManager::getInstance().playSound("lost_life");
        m_player2->loseLife();
        if (m_player2->getLives() > 0) {
            m_player2->setPosition(m_hasCheckpoint ? m_checkpointPosition : m_levelSpawnPoint);
            m_player2->setVelocity({0.0f, 0.0f});
        } else if (allPlayersOut()) {
            killPlayer("both players are out");
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


    // 3e. Level timer. It was set once, shown in the HUD and snapshotted for
    // rewind, but never actually decremented — so there was no time pressure and
    // no time-out death, and TimeWarning was never published despite having a
    // subscriber (audit G-2).
    if (!m_levelComplete && m_levelTimer > 0.0f) {
        m_levelTimer -= dt;

        if (!m_timeWarningFired && m_levelTimer <= 100.0f) {
            m_timeWarningFired = true;
            EventBus::getInstance().publish({EventType::TimeWarning, static_cast<int>(m_levelTimer)});
        }

        if (m_levelTimer <= 0.0f) {
            m_levelTimer = 0.0f;
            killPlayer("ran out of time");
            if (m_player && m_player->getLives() <= 0) return;
        }
    }

    // 3f. Level complete: hold briefly on the flag, then show the summary.
    // The victory screen is an overlay, so this level stays on screen behind it
    // and the player sees the flag they just touched.
    if (m_levelComplete && !m_summaryShown) {
        m_levelCompleteTimer += dt;
        if (m_levelCompleteTimer >= 3.0f) {
            presentLevelSummary();
            return;
        }
    }

    // 3g. Boss arena. Runs before the camera follows, so the lock is in place
    // for this frame rather than one frame late.
    updateBossArena();

    // 4. Update Camera & Screen Transitions
    if (m_player2) {
        updateVersusCamera(dt);
    } else if (m_player) {
        // Velocity drives the lookahead (task 4.3): the camera leads the player
        // in the direction they are running.
        m_camera.follow(m_player->getPosition(), m_player->getVelocity(), dt);
    }
    m_camera.update(dt);
    m_background.update(dt);
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
        syncBossHud(hudData);
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
        // The replay wants the same Memento, kept for longer and thinned out —
        // no second capture path, and nothing to drift out of step (task 10.3).
        ReplayRecorder::getInstance().record(snapshot);
    }
}

void PlayingState::render(sf::RenderTarget& target) {
    // Parallax backdrop first, in screen space: each layer is offset by a
    // fraction of the camera position, so it has to be drawn before the world
    // view is applied (task 5.5).
    target.setView(target.getDefaultView());
    m_background.render(target, m_camera.getVisibleBounds());

    // Set view to camera view for scrolling world space rendering
    target.setView(m_camera.getView());

    // Compute animated tile frame indices from timer
    int coinFrame     = static_cast<int>(m_tileAnimTimer / 0.15f) % 4;
    int questionFrame = static_cast<int>(m_tileAnimTimer / 0.20f) % 4;
    // Two-frame wave cycle shared by water and lava surfaces.
    const int waveFrame = static_cast<int>(m_tileAnimTimer / 0.35f) % 2;

    // Only iterate tiles the camera can actually see. Sweeping the whole grid was
    // roughly 4,400 sprite draws per frame on a 200-wide level (audit A-14).
    // One tile of margin keeps partially-visible edges and the water bob covered.
    const AABB view = m_camera.getVisibleBounds();
    const int firstX = std::max(0, static_cast<int>(std::floor(view.x / Constants::TILE_SIZE)) - 1);
    const int lastX  = std::min(m_tileMap.getWidth() - 1,
                                static_cast<int>(std::floor((view.x + view.width) / Constants::TILE_SIZE)) + 1);
    const int firstY = std::max(0, static_cast<int>(std::floor(view.y / Constants::TILE_SIZE)) - 1);
    const int lastY  = std::min(m_tileMap.getHeight() - 1,
                                static_cast<int>(std::floor((view.y + view.height) / Constants::TILE_SIZE)) + 1);

    // 1. Draw the tilemap tiles (bottom-to-top to ensure overlapping/bobbing top layers draw on top of background)
    for (int y = lastY; y >= firstY; --y) {
        for (int x = firstX; x <= lastX; ++x) {
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
                        const bool isSurface = (y == 0) || (m_tileMap.getTileType(x, y - 1) != TileType::Water);
                        // The surface alternates between the two wave frames the
                        // atlas ships, so it actually moves rather than only
                        // bobbing up and down (task 5.10).
                        frameKey = isSurface
                            ? (waveFrame == 0 ? "water_dark_blue_wave_long" : "water_light_blue_wave_long")
                            : "water_dark_blue_bg";
                        break;
                    }
                    case TileType::Lava: {
                        const bool isSurface = (y == 0) || (m_tileMap.getTileType(x, y - 1) != TileType::Lava);
                        frameKey = isSurface
                            ? (waveFrame == 0 ? "lava_wave_long" : "lava_wave_short")
                            : "lava_bg";
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
                        const bool liquidSurface =
                            (tileType == TileType::Water &&
                             (y == 0 || m_tileMap.getTileType(x, y - 1) != TileType::Water)) ||
                            (tileType == TileType::Lava &&
                             (y == 0 || m_tileMap.getTileType(x, y - 1) != TileType::Lava));
                        if (liquidSurface) {
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
    if (m_devPanel.showAABB()) {
        // Tile grid AABBs — culled to the same visible window as the tile pass.
        for (int y = firstY; y <= lastY; ++y) {
            for (int x = firstX; x <= lastX; ++x) {
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
            // Category colours come from the palette, so the debug overlay is
            // readable under colourblind mode too (task 11.4).
            sf::Color outlineColor = ColorPalette::get(ColorPalette::Role::Player);
            if (dynamic_cast<Enemy*>(entity.get()))      outlineColor = ColorPalette::get(ColorPalette::Role::Enemy);
            else if (dynamic_cast<Item*>(entity.get()))  outlineColor = ColorPalette::get(ColorPalette::Role::Item);
            else if (dynamic_cast<Block*>(entity.get())) outlineColor = ColorPalette::get(ColorPalette::Role::Block);
            outlineColor.a = 220;
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

        renderVersusHud(target);

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

    // The map editor paints its own overlay and ImGui windows; otherwise reset
    // to screen space so the dev panels sit over the world.
    if (m_mapEditor.isActive()) {
        m_mapEditor.render(target, m_tileMap, m_entities, &m_camera);
        m_mapEditor.renderImGui(m_tileMap, m_entities);
    } else {
        target.setView(target.getDefaultView());
    }

    // Dev/debug ImGui surface. Issues ImGui calls only — every requested change
    // is queued and applied by update(), so render() mutates no game state.
    // Suppressed while an overlay is up: an ImGui window drawn underneath the
    // pause menu still reacts to the mouse, which would let the player edit the
    // level they just paused.
    if (!m_suspended) {
        m_devPanel.draw(*this);
    }
}

void PlayingState::onSuspend() {
    m_suspended = true;
    SoundManager::getInstance().pauseMusic();
}

void PlayingState::onResume() {
    m_suspended = false;
    SoundManager::getInstance().resumeMusic();
}

void PlayingState::setupTestScene() {
    cleanupTestScene();

    LevelLoader loader;
    LevelData levelData;
    // Campaign order lives in LevelCatalog — this used to be an if-chain here
    // with a matching hardcoded level count in advanceToNextLevel().
    std::string levelPath = LevelCatalog::pathFor(m_selectedLevelIndex);


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
        m_levelSpawnPoint = levelData.spawnPoint;
        m_hasCheckpoint = false;   // a new level invalidates the old checkpoint
        spawnSelectedPlayer(levelData.spawnPoint);
        
        // Transfer all loaded items/entities and wire their animations
        for (auto& entity : levelData.entities) {
            admitEntity(entity.get());
            m_entities.push_back(std::move(entity));
        }

        // Set camera bounds matching the level size
        m_camera.setBounds(AABB{0.0f, 0.0f, levelData.width * Constants::TILE_SIZE, levelData.height * Constants::TILE_SIZE});
        m_background.setTheme(levelData.theme);
    } else {
        // Fallback: manually setup scene if file loading fails
        m_tileMap.initialize(40, 22);
        for (int x = 0; x < 40; ++x) {
            m_tileMap.setTile(x, 20, TileType::Ground);
            m_tileMap.setTile(x, 21, TileType::Ground);
        }
        m_levelSpawnPoint = sf::Vector2f(100.0f, 100.0f);
        m_hasCheckpoint = false;
        spawnSelectedPlayer(m_levelSpawnPoint);
        m_camera.setBounds(AABB{0.0f, 0.0f, 40.0f * Constants::TILE_SIZE, 22.0f * Constants::TILE_SIZE});
    }

    // Player 2 joins beside Player 1. Character is whichever of Mario/Luigi
    // Player 1 did not take, so the two are always visually distinct.
    if (m_twoPlayer && m_player) {
        const sf::Vector2f spawn = m_player->getPosition() + sf::Vector2f(48.0f, 0.0f);
        std::unique_ptr<Player> second = (m_selectedCharIndex == 1)
            ? std::unique_ptr<Player>(std::make_unique<Mario>(spawn))
            : std::unique_ptr<Player>(std::make_unique<Luigi>(spawn));
        second->restoreStats(Game::getInstance().difficulty().startingLives(), 0, 0);

        m_player2 = second.get();
        admitEntity(m_player2);
        m_entities.push_back(std::move(second));

        // Player 2's bindings (arrows, M, N) have existed in InputManager since
        // it was written; nothing had ever registered a second player against
        // them (task 11.1).
        InputManager::getInstance().registerPlayer(m_player2, 1);
        std::cout << "[PlayingState] Two-player versus: P2 is "
                  << m_player2->getCharacterName() << std::endl;
    }

    findActiveBoss();

    // Always recording, so "replay save" after something interesting happens
    // actually has the interesting thing in it. Bounded by kMaxFrames.
    ReplayRecorder::getInstance().startRecording(
        m_isProcedural ? "procedural" : LevelCatalog::nameFor(m_selectedLevelIndex));
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
        admitEntity(entity.get());
    }

    sf::Vector2f spawnPos = (spawnOverride.x != 0.0f || spawnOverride.y != 0.0f) ? spawnOverride : levelData.spawnPoint;

    // cleanupTestScene() nulled m_player, so spawnSelectedPlayer cannot carry the
    // stats itself — apply them here, through the same silent path.
    m_levelSpawnPoint = spawnPos;
    m_hasCheckpoint = false;
    spawnSelectedPlayer(spawnPos);
    if (m_player) {
        m_player->restoreStats(savedLives, savedCoins, savedScore);
    }

    Game::getInstance().setTileMap(&m_tileMap);
    m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});
    m_background.setTheme(levelData.theme);
    findActiveBoss();

    std::cout << "[PlayingState] Loaded sub-level / main level: " << chosenPath << " at spawn (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return true;
}


void PlayingState::regenerateProceduralLevel() {
    MapGenerator::generate(m_tileMap, m_entities, m_genConfig);
    for (auto& entity : m_entities) {
        admitEntity(entity.get());
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
    m_camera.setBounds(AABB{0.0f, 0.0f,
                            m_tileMap.getWidth() * Constants::TILE_SIZE,
                            m_tileMap.getHeight() * Constants::TILE_SIZE});
    if (m_minimap) m_minimap->initialize(m_tileMap);
}

void PlayingState::saveToSlot(int slot) {
    if (!m_player) return;
    Serializer::saveGame(slot, *m_player, 1, "Level 1", Constants::LEVEL_TIME,
                         m_player->getPosition().x, m_player->getPosition().y,
                         std::vector<bool>(m_starCoinsCollected.begin(), m_starCoinsCollected.end()));
}

void PlayingState::loadFromSlot(int slot) {
    std::unique_ptr<Player> loadedPlayer;
    int lvlId = 0;
    std::string lvlName;
    float timeRem = 0.0f, checkX = 0.0f, checkY = 0.0f;
    std::vector<bool> starCoins;

    if (!Serializer::loadGame(slot, loadedPlayer, lvlId, lvlName, timeRem, checkX, checkY, starCoins)
        || !loadedPlayer) {
        return;
    }

    if (starCoins.size() >= 3) {
        m_starCoinsCollected = {starCoins[0], starCoins[1], starCoins[2]};
    }
    // adoptPlayer refreshes m_player, InputManager and Game together; assigning
    // m_entities[0] directly here is what left m_player dangling (audit A-3).
    adoptPlayer(std::move(loadedPlayer));
    Game::getInstance().setActiveSlot(slot);
    std::cout << "Loaded save slot " << slot << " successfully!" << std::endl;
}

void PlayingState::updateVersusCamera(float dt) {
    if (!m_player || !m_player2) return;

    // Frame the midpoint, so neither player is privileged.
    const sf::Vector2f a = m_player->getPosition();
    const sf::Vector2f b = m_player2->getPosition();
    const sf::Vector2f midpoint{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};

    // No lookahead here: with two players the frame is already a compromise, and
    // biasing it towards one player's direction of travel makes it lurch.
    m_camera.follow(midpoint, sf::Vector2f{0.0f, 0.0f}, dt);

    // The tether. Whoever falls behind is shoved along at the screen edge rather
    // than being left behind, which is what stops one player dragging the other
    // out of the level — the alternative is split screen, and every screen-space
    // overlay in the game would need to learn about viewports.
    const AABB view = m_camera.getVisibleBounds();
    const float margin = Constants::TILE_SIZE;
    for (Player* player : {m_player, m_player2}) {
        if (!player) continue;
        const AABB box = player->getBoundingBox();
        sf::Vector2f position = player->getPosition();
        bool moved = false;

        if (position.x < view.x + margin) {
            position.x = view.x + margin;
            moved = true;
        } else if (position.x + box.width > view.x + view.width - margin) {
            position.x = view.x + view.width - margin - box.width;
            moved = true;
        }
        if (moved) {
            player->setPosition(position);
            // Kill the horizontal velocity too, or the player grinds against the
            // edge with their run animation playing and never actually moves.
            player->setVelocity({0.0f, player->getVelocity().y});
        }
    }
}

bool PlayingState::allPlayersOut() const {
    const bool oneOut = !m_player || m_player->getLives() <= 0;
    const bool twoOut = !m_player2 || m_player2->getLives() <= 0;
    return oneOut && twoOut;
}

void PlayingState::renderVersusHud(sf::RenderTarget& target) const {
    if (!m_player2 || !m_player) return;

    // Deliberately drawn here rather than inside Hud: HudData describes one
    // player, and widening it would push two-player concerns into every
    // single-player HUD field.
    const std::string p1 = "P1 " + std::to_string(m_player->getScore()) +
                           "  x" + std::to_string(m_player->getLives());
    const std::string p2 = "P2 " + std::to_string(m_player2->getScore()) +
                           "  x" + std::to_string(m_player2->getLives());

    UiRenderer::drawText(target, p1, {24.0f, Constants::WINDOW_HEIGHT - 56.0f}, 12,
                         ColorPalette::get(ColorPalette::Role::Player));
    UiRenderer::drawText(target, p2, {24.0f, Constants::WINDOW_HEIGHT - 34.0f}, 12,
                         ColorPalette::get(ColorPalette::Role::Item));

    // Who is winning, which is the whole point of versus.
    const int lead = m_player->getScore() - m_player2->getScore();
    const std::string status = (lead == 0) ? "TIED"
                             : (lead > 0)  ? "P1 LEADS BY " + std::to_string(lead)
                                           : "P2 LEADS BY " + std::to_string(-lead);
    UiRenderer::drawText(target, status,
                         {Constants::WINDOW_WIDTH * 0.5f, Constants::WINDOW_HEIGHT - 34.0f},
                         12, sf::Color(255, 216, 0), true);
}

void PlayingState::applySnapshot(const GameSnapshot& snapshot) {
    m_levelTimer = snapshot.levelTimer;
    m_camera.snapTo(snapshot.cameraCenter);

    if (m_player) {
        m_player->restoreMemento(snapshot.playerState);
    }

    // Restore entities by id, not by index. Between record and restore the prune
    // step removes inactive entities and the fireball listener appends new ones,
    // so positions in m_entities do not correspond across frames (audit A-5).
    std::unordered_map<std::uint32_t, const EntitySnapshot*> byId;
    byId.reserve(snapshot.entityStates.size());
    for (const auto& entityState : snapshot.entityStates) {
        byId.emplace(entityState.id, &entityState);
    }
    for (const auto& entity : m_entities) {
        if (!entity) continue;
        auto it = byId.find(entity->getId());
        if (it == byId.end()) continue;  // spawned after this snapshot — leave it alone
        entity->setPosition(it->second->position);
        entity->setVelocity(it->second->velocity);
        entity->active = it->second->active;
    }
}

void PlayingState::findActiveBoss() {
    m_activeBoss = nullptr;
    m_arenaLocked = false;
    for (const auto& entity : m_entities) {
        if (auto* boss = dynamic_cast<Boss*>(entity.get())) {
            m_activeBoss = boss;
            break;   // one boss per level; the SPEC has no fight with two
        }
    }
    if (m_activeBoss) {
        std::cout << "[PlayingState] Boss in this level: "
                  << m_activeBoss->getDisplayName() << std::endl;
    }
}

void PlayingState::updateBossArena() {
    // The boss entity is owned by m_entities and pruned when it deactivates, so
    // the pointer has to be dropped in the same frame it stops being active.
    if (m_activeBoss && !m_activeBoss->isActive()) {
        if (m_arenaLocked) {
            m_camera.setBounds(m_preArenaCameraBounds);
            m_camera.setScrollMode(Camera::ScrollMode::Free);
            m_arenaLocked = false;
            std::cout << "[PlayingState] Boss arena released." << std::endl;
        }
        m_activeBoss = nullptr;
    }

    if (!m_activeBoss || !m_player || !m_activeBoss->hasArena()) return;

    const AABB arena = m_activeBoss->getArena();

    if (!m_arenaLocked) {
        // Lock once the player is properly inside, not the instant they clip the
        // edge, so the camera does not snap while they are still walking in.
        const sf::Vector2f p = m_player->getPosition();
        if (p.x > arena.x + Constants::TILE_SIZE && p.x < arena.x + arena.width) {
            m_preArenaCameraBounds = m_camera.getBounds();
            m_camera.setBounds(arena);
            // Stop chasing as well as clamping: with the arena narrower than the
            // view, chasing only fights the clamp.
            m_camera.setScrollMode(Camera::ScrollMode::Locked);
            m_arenaLocked = true;
            SoundManager::getInstance().playMusic("bowser_boss_battle");
            std::cout << "[PlayingState] Boss arena locked: " << m_activeBoss->getDisplayName()
                      << std::endl;
        }
        return;
    }

    // "No escape until defeated" (SPEC 6.4): hold the player inside the arena.
    // Clamping the position rather than adding walls keeps this out of the
    // physics engine, which knows nothing about bosses.
    const AABB box = m_player->getBoundingBox();
    sf::Vector2f p = m_player->getPosition();
    if (p.x < arena.x) {
        p.x = arena.x;
    } else if (p.x + box.width > arena.x + arena.width) {
        p.x = arena.x + arena.width - box.width;
    }
    m_player->setPosition(p);
}

void PlayingState::syncBossHud(HudData& hudData) const {
    if (!m_activeBoss || !m_arenaLocked) {
        hudData.bossActive = false;
        return;
    }
    hudData.bossActive    = true;
    hudData.bossName      = m_activeBoss->getDisplayName();
    hudData.bossHealth    = m_activeBoss->getHealth();
    hudData.bossMaxHealth = m_activeBoss->getMaxHealth();
}

void PlayingState::killPlayer(const char* reason) {
    if (!m_player) return;

    SoundManager::getInstance().playSound("lost_life");
    m_player->loseLife();

    // In versus, one player running out does not end the run — the other is
    // still playing, and ending it early would hand them the win by default.
    if (m_twoPlayer && m_player->getLives() <= 0 && !allPlayersOut()) {
        std::cout << "[PlayingState] Player 1 is out (" << reason
                  << "); Player 2 continues." << std::endl;
        return;
    }

    if (m_player->getLives() > 0) {
        // Respawn at the last checkpoint if one was reached, otherwise at the
        // level's own spawn point — never at a hardcoded corner.
        const sf::Vector2f respawn = m_hasCheckpoint ? m_checkpointPosition : m_levelSpawnPoint;
        m_player->setPosition(respawn);
        m_player->setVelocity({0.0f, 0.0f});
        m_camera.snapTo(respawn);

        // A fresh clock per life, so a timeout death is recoverable.
        m_levelTimer = Constants::LEVEL_TIME * Game::getInstance().difficulty().levelTimeScale();
        m_timeWarningFired = false;

        std::cout << "[PlayingState] Player " << reason << ". Lives remaining: "
                  << m_player->getLives() << std::endl;
    } else {
        std::cout << "[PlayingState] Game Over — " << reason << "." << std::endl;
        EventBus::getInstance().publish({EventType::GameOver, 0});

        // The summary has to be taken *now*: by the time the fade completes the
        // callback runs, this state is being replaced and m_player is gone.
        RunSummary summary = buildRunSummary();
        ScreenTransitionManager::getInstance().fadeOut(0.6f, [summary]() {
            Game::getInstance().changeState(std::make_unique<GameOverState>(summary));
        });
    }
}

RunSummary PlayingState::buildRunSummary() const {
    RunSummary summary;
    summary.levelIndex      = m_selectedLevelIndex;
    summary.characterIndex  = m_selectedCharIndex;
    summary.isProcedural    = m_isProcedural;
    summary.generatorConfig = m_genConfig;
    summary.starCoins = static_cast<int>(std::count(m_starCoinsCollected.begin(),
                                                    m_starCoinsCollected.end(), true));
    if (m_player) {
        summary.score         = m_player->getScore();
        summary.coins         = m_player->getCoins();
        summary.characterName = m_player->getCharacterName();
    }
    return summary;
}

void PlayingState::restartLevel() {
    std::cout << "[PlayingState] Restarting level " << LevelCatalog::nameFor(m_selectedLevelIndex)
              << std::endl;
    m_levelComplete = false;
    m_levelCompleteTimer = 0.0f;
    m_summaryShown = false;
    m_levelTimer = Constants::LEVEL_TIME * Game::getInstance().difficulty().levelTimeScale();
    m_timeWarningFired = false;
    m_hasCheckpoint = false;
    m_starCoinsCollected = {false, false, false};
    setupTestScene();
    SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);
    ScreenTransitionManager::getInstance().fadeIn(0.45f);
}

void PlayingState::presentLevelSummary() {
    if (m_summaryShown) return;
    m_summaryShown = true;

    // Record the clear before the summary goes up, so the world map is correct
    // the moment the player gets back to it. Procedural levels are not part of
    // the campaign and are deliberately not recorded.
    if (!m_isProcedural) {
        CampaignProgress::recordLevelCleared(m_selectedLevelIndex, m_starCoinsCollected);
    }

    LevelSummary summary;
    summary.levelName    = m_isProcedural ? "Procedural" : LevelCatalog::nameFor(m_selectedLevelIndex);
    summary.timeRemaining = static_cast<int>(m_levelTimer);
    summary.timeBonus     = summary.timeRemaining * 50;   // SPEC: 50 points per second left
    summary.starCoins     = m_starCoinsCollected;
    summary.isFinalLevel  = m_isProcedural || (m_selectedLevelIndex + 1 >= LevelCatalog::count());

    if (m_player) {
        summary.characterName   = m_player->getCharacterName();
        summary.coins           = m_player->getCoins();
        summary.scoreBeforeBonus = m_player->getScore();
        // Award the bonus for real before the screen animates it, so the number
        // the player watches tick up is the number they actually have.
        m_player->addScore(summary.timeBonus);
        summary.finalScore = m_player->getScore();
    }

    Game::getInstance().pushState(std::make_unique<VictoryState>(
        summary, [this]() { advanceToNextLevel(); }));
}

void PlayingState::advanceToNextLevel() {
    // Campaign order matches the level dropdown: 1-1, 1-1 sub, 1-2, 1-2 sub,
    // 1-3, 1-3 sub, bonus. Finishing the last one returns to the menu.
    const int nextIndex = m_selectedLevelIndex + 1;

    if (m_isProcedural || nextIndex >= LevelCatalog::count()) {
        if (!m_isProcedural) {
            // Finishing the last level opens the next New Game+ cycle: the level
            // flags reset, the counter and the unlocks do not (task 11.3).
            MetaGame::advanceNewGamePlus();
            std::cout << "[PlayingState] New Game+ level is now "
                      << MetaGame::newGamePlusLevel() << "." << std::endl;
        }
        std::cout << "[PlayingState] Campaign complete — returning to menu." << std::endl;
        ScreenTransitionManager::getInstance().fadeOut(0.8f, []() {
            Game::getInstance().changeState(std::make_unique<MenuState>());
        });
        return;
    }

    std::cout << "[PlayingState] Advancing to level index " << nextIndex << std::endl;
    m_selectedLevelIndex = nextIndex;
    m_levelComplete = false;
    m_levelCompleteTimer = 0.0f;
    m_summaryShown = false;
    m_starCoinsCollected = {false, false, false};
    m_levelTimer = Constants::LEVEL_TIME * Game::getInstance().difficulty().levelTimeScale();
    m_timeWarningFired = false;
    m_hasCheckpoint = false;
    setupTestScene();
    SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);
    ScreenTransitionManager::getInstance().fadeIn(0.45f);
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
    // First spawn of a run gets the difficulty's life count; later spawns carry
    // whatever the player had, because this branch only runs when there is none.
    int oldLives = Game::getInstance().difficulty().startingLives();
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

std::unique_ptr<Entity> PlayingState::spawnProjectile(int entityType, sf::Vector2f position,
                                                      sf::Vector2f velocity) {
    switch (static_cast<EntityType>(entityType)) {
        case EntityType::Hammer:
            return m_hammerPool.acquire(position, velocity);
        case EntityType::BossFireball:
            return m_bossFireballPool.acquire(position, velocity);
        default:
            // Not a pooled type — the factory stays the single construction
            // point for everything else.
            return EntityFactory::create(static_cast<EntityType>(entityType), position);
    }
}

void PlayingState::recycleEntity(std::unique_ptr<Entity> entity) {
    if (!entity) return;

    // The cast has to happen before release(), and the raw pointer has to be
    // re-wrapped in a typed unique_ptr: the pool stores concrete types so it can
    // call resetForPool on them.
    if (dynamic_cast<Fireball*>(entity.get())) {
        m_fireballPool.release(std::unique_ptr<Fireball>(static_cast<Fireball*>(entity.release())));
        return;
    }
    if (dynamic_cast<Hammer*>(entity.get())) {
        m_hammerPool.release(std::unique_ptr<Hammer>(static_cast<Hammer*>(entity.release())));
        return;
    }
    if (dynamic_cast<BossFireball*>(entity.get())) {
        m_bossFireballPool.release(
            std::unique_ptr<BossFireball>(static_cast<BossFireball*>(entity.release())));
        return;
    }
    // Everything else dies here, exactly as it did before pooling existed.
}

void PlayingState::admitEntity(Entity* entity) {
    if (!entity) return;
    wireEntityAnimations(entity);

    // Difficulty is applied here, at the single door every entity comes through,
    // rather than in EntityFactory — the factory should not have to know that a
    // Game singleton with a difficulty setting exists.
    if (auto* enemy = dynamic_cast<Enemy*>(entity)) {
        // Difficulty and New Game+ compound: a second playthrough on Hard is
        // meant to be harder than either alone (tasks 9.4 and 11.3).
        enemy->applySpeedScale(Game::getInstance().difficulty().enemySpeedScale()
                               * MetaGame::enemySpeedMultiplier());
    }
}

void PlayingState::wireEntityAnimations(Entity* entity) {
    if (!entity) return;

    // Route entity to its matching sprite sheet atlas based on type hierarchy.
    // setupAnimations() lives on Player, Enemy, Item, Block — not on Entity base.
    if (auto* p = dynamic_cast<Player*>(entity)) {
        if (m_playerSheet) p->setupAnimations(m_playerSheet.get());
    } else if (auto* e = dynamic_cast<Enemy*>(entity)) {
        if (m_enemySheet) e->setupAnimations(m_enemySheet.get());
    } else if (auto* proj = dynamic_cast<Projectile*>(entity)) {
        // Hammers and Bowser's fire breath both live in the enemy/projectile
        // atlas. One branch covers every projectile, so a new one is wired by
        // existing.
        if (m_enemySheet) proj->setupAnimations(m_enemySheet.get());
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

