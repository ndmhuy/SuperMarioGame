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
#include "Entities/ShadowMario.hpp"
#include "Entities/AIController.hpp"
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


namespace {

// The generator picks a theme; the parallax backdrop needs the same one. Nothing
// carried it across, so every generated level — castle, ice, underground — was
// drawn against the overworld blue sky and green hills.
BackgroundTheme backdropForGeneratedTheme(MapTheme theme) {
    switch (theme) {
        case MapTheme::Underground: return BackgroundTheme::Underground;
        case MapTheme::Castle:      return BackgroundTheme::Castle;
        case MapTheme::Ice:         return BackgroundTheme::Ice;
        case MapTheme::Overworld:
        default:                    return BackgroundTheme::Overworld;
    }
}

} // namespace

PlayingState::PlayingState(bool startInEditor, bool isProcedural, const MapGeneratorConfig& genConfig,
                           int characterIndex, int levelIndex, MatchConfig match)
    : m_match(match),
      m_selectedCharIndex(characterIndex),
      m_selectedLevelIndex(LevelCatalog::isValidIndex(levelIndex) ? levelIndex : 0),
      m_startInEditor(startInEditor), m_isProcedural(isProcedural),
      m_genConfig(genConfig) {
    // Published now rather than in enter(), because the collision resolver can
    // be reached by a harness that never enters a state, and a stale co-op flag
    // there would silently turn a versus stomp into a friendly boost.
    Game::getInstance().setMatchConfig(m_match);
}

PlayingState::~PlayingState() {
    exit();
}

void PlayingState::enter() {
    std::cout << "Entering PlayingState (startInEditor: " << m_startInEditor << ", isProcedural: " << m_isProcedural << ")" << std::endl;

    // Load SFX & Start Level BGM
    SoundManager::getInstance().loadAllSounds();
    SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);

    // --- Load all sprite sheet atlases ---
    // Path resolution lives in SpriteSheet::loadAtlas now; this used to
    // carry its own three-candidate search, and three other screens had
    // copied it (audit A-13).

    // Lead the player by up to a third of a screen at full run speed. Enough to
    // see what is coming; more than this and the player sits at the screen edge.
    m_camera.setLookahead(140.0f);

    m_playerSheet  = SpriteSheet::loadAtlas("player");
    m_enemySheet   = SpriteSheet::loadAtlas("enemy_projectile");
    m_itemSheet    = SpriteSheet::loadAtlas("item");
    m_scenerySheet = SpriteSheet::loadAtlas("world_scenery_item");
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
        m_background.setTheme(backdropForGeneratedTheme(m_genConfig.theme));
        syncBackdropGround();

        // Wire animations for all procedurally generated entities
        for (auto& entity : m_entities) {
            admitEntity(entity.get());
        }
        
        // The generator always builds a Mario (MapGenerator::generate does
        // `std::make_unique<Mario>`), and this used to simply adopt whatever
        // Player it found in the generated list. So the character the player
        // picked was carried all the way here in m_selectedCharIndex — it is even
        // echoed back in buildRunSummary — and then ignored, which is why the
        // daily challenge always came out as Mario however you chose.
        //
        // Take the generator's spawn point, discard its player, and build the
        // selected character there instead.
        sf::Vector2f generatedSpawn{96.0f, 64.0f};
        for (auto it = m_entities.begin(); it != m_entities.end(); ) {
            if (dynamic_cast<Player*>(it->get())) {
                generatedSpawn = (*it)->getPosition();
                it = m_entities.erase(it);
            } else {
                ++it;
            }
        }
        m_player = nullptr;
        m_levelSpawnPoint = generatedSpawn;
        // adoptPlayer (via spawnSelectedPlayer) refreshes m_player, InputManager
        // and Game together, which is the only sanctioned way in (audit A-3).
        spawnSelectedPlayer(generatedSpawn);
        spawnMatchParticipants();
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

    // A player death reported from anywhere — an enemy killing Small Mario, the
    // debug key, a script — runs that player's death sequence.
    //
    // The payload matters and used to be discarded. Player::powerDown() publishes
    // {PlayerDied, this}, so the event has always known WHICH player died; the
    // handler ignored it and killPlayer() acted on Player 1 unconditionally. An
    // enemy hitting Player 2 therefore ran the death fall, the life deduction and
    // the game-over test on Player 1, standing unharmed somewhere else — the
    // "kills the wrong player" report, exactly.
    //
    // The debug key publishes an int rather than a Player*, so the cast is
    // guarded and falls back to Player 1.
    m_playerDiedSubId = EventBus::getInstance().subscribe(
        EventType::PlayerDied, [this](const GameEvent& ev) {
            Player* who = nullptr;
            if (const Player* const* asPlayer = std::any_cast<Player*>(&ev.data)) {
                who = const_cast<Player*>(*asPlayer);
            }
            killPlayer(who, "was defeated");
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
    if (m_playerDiedSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_playerDiedSubId);
        m_playerDiedSubId = static_cast<EventBus::SubscriptionId>(-1);
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
    InputManager::getInstance().registerPlayer(nullptr, 1);
    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setSecondPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
    // The match ends with the state. Leaving it set would carry co-op's
    // friendly-fire rule into the next single-player run, since the collision
    // resolver reads it from the singleton rather than from this object.
    Game::getInstance().setMatchConfig(MatchConfig{});
    m_aiController.reset();
    m_player2 = nullptr;
    m_shadow = nullptr;
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

            // Tab toggles the minimap. Minimap subscribes to this event itself.
            //
            // This was M, which is also Player 2's bound fire key. In a
            // two-player match one keypress therefore both threw a fireball and
            // flipped the minimap, and the minimap won the visual argument — so
            // the collision read as "P2 cannot shoot". Tab is bound to nothing
            // and is the conventional map key.
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
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

    if (!m_mapEditor.isActive()) {
        InputManager& input = InputManager::getInstance();
        if (m_player) input.handleInput(event, *m_player);
        // Player 2's press actions — jump, fire, ground pound — were never
        // dispatched: only the *hold* mappings ran, from update(). Player 2
        // could walk and crouch but could not jump, for the entire life of
        // two-player mode. A CPU-driven Player 2 never reads the keyboard, so
        // it is skipped rather than fed Player 2's keys.
        if (m_player2 && !m_aiController) input.handleInput(event, *m_player2);
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
        // The editor skips the simulation, not the presentation layer. enter()
        // starts a 0.45s fade-in, and render() draws that overlay after the
        // world; returning here without advancing it pinned a full-screen black
        // rectangle over the editor forever. Entering the editor straight from
        // the menu ("Level Editor", "Generate & Edit") therefore showed nothing
        // but the grid, while F1 mid-level looked fine because the fade had
        // already finished.
        m_background.update(dt);
        ScreenTransitionManager::getInstance().update(dt);
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
    // Toast fading is advanced by Game::run, so it keeps running across a state
    // change. Ticking it here as well would fade them at double speed.

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
    // Same defect as the movement keys: isKeyPressed reads global OS state and
    // returns false without macOS Input Monitoring permission, so rewind was
    // dead for the same invisible reason.
    const bool rewindRequested = InputManager::getInstance().isHeld(sf::Keyboard::Key::R);

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
    // Held-key state comes from the event stream now, so being unfocused already
    // means no keys are held — InputManager clears them on FocusLost. What still
    // needs gating is who the keys belong to while the window *is* focused:
    // typing "difficulty hard" in the console, or into any ImGui field, must not
    // also walk the player right on every "d".
    const bool inputBelongsToGameplay =
        !DebugConsole::getInstance().isVisible() &&
        !ImGui::GetIO().WantCaptureKeyboard;

    if (inputBelongsToGameplay) {
        if (m_player)  InputManager::getInstance().update(*m_player);
        // A CPU-driven Player 2 is steered by its controller instead. Polling the
        // keyboard for it as well would let the human drive both characters.
        if (m_player2 && !m_aiController) InputManager::getInstance().update(*m_player2);
    }

    // 2b. The CPU opponent decides. Runs in the same slot human input does, so a
    // bot and a human are subject to the same physics, the same collision pass
    // and the same frame ordering — the bot has no privileged access to the
    // world, only a wider view of it than a human can hold in their head.
    if (m_aiController && m_player2) {
        m_aiController->update(dt, m_player, m_tileMap, m_entities);
    }

    // 2c. Shadow Mario samples the human's inputs for this frame. Before the
    // entity pass, so this frame's packet cannot also be replayed this frame.
    updateShadow(dt);

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
            const bool dead = entity && !entity->isActive() &&
                              entity.get() != m_player && entity.get() != m_player2;
            return !dead;
        });
    // Every raw pointer this state keeps into m_entities has to be dropped
    // *before* the object behind it is freed. Defeating Bowser was a hard crash:
    // his defeat sequence ends in destroy(), the prune below deleted him, and
    // updateBossArena() then read m_activeBoss->isActive() through the dangling
    // pointer ninety lines later. Clearing it there was always one frame too
    // late — the free happens here.
    for (auto it = deadBegin; it != m_entities.end(); ++it) {
        if (*it) forgetEntity(it->get());
    }
    for (auto it = deadBegin; it != m_entities.end(); ++it) {
        recycleEntity(std::move(*it));
    }
    m_entities.erase(deadBegin, m_entities.end());

    // 3b1. Death sequence. While it runs, the corpse is falling through the
    // level on purpose, so none of the hazard checks below may fire again.
    //
    // Note this still short-circuits the whole frame, which in two-player mode
    // means the survivor is held still for the ~1s their partner spends falling.
    // That is deliberate and matches the single-player feel; letting one player
    // keep running while the other dies would need the hazard checks below to be
    // per-player, which is a larger change than this fix.
    if (anyDeathInProgress()) {
        updateDeathSequence(dt);
        m_camera.update(dt);
        m_background.update(dt);
        ScreenTransitionManager::getInstance().update(dt);
        return;
    }

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
            if (m_player->getLives() <= 0 || m_player->isDying()) {
                killPlayer(m_player, "fell into the lava");
                return;
            }
        }
    }

    // 3c. Void fall — one shared death path, so the timer and the pit agree.
    // Sideways counts too: a bad warp exit could park the player past the edge
    // of the map, where they were outside every tile, never fell, and never
    // died. "In the void but not dying" was exactly that.
    //
    // Both players go through the same path now. Player 2 had a hand-rolled
    // version that only checked the *bottom* — so a Player 2 shoved off the left
    // edge was stuck alive outside the map forever — and which skipped the death
    // fall, the jingle and the i-frames that Player 1 got.
    const float bottomVoidY = (m_tileMap.getHeight() * Constants::TILE_SIZE) + 32.0f;
    const float rightVoidX  = (m_tileMap.getWidth()  * Constants::TILE_SIZE) + 64.0f;
    for (Player* who : {m_player, m_player2}) {
        if (!who || !who->isActive()) continue;
        const sf::Vector2f at = who->getPosition();
        if (at.y > bottomVoidY || at.x < -64.0f || at.x > rightVoidX) {
            // Nothing else publishes for a fall, so this path does.
            EventBus::getInstance().publish({EventType::PlayerDied, who});
            killPlayer(who, "fell into the void");
        }
    }
    if (anyDeathInProgress()) return;

    // 3d. Warp Pipe check for sub-level transitions or teleportation.
    //
    // Two things this has to get right, both of which it used to get wrong:
    //
    //  - The decision is taken while iterating m_entities, but acting on it
    //    replaces that whole vector. The intent is collected first and applied
    //    after the loop, so nothing reads a destroyed Pipe.
    //  - Holding the crouch key kept re-triggering the warp every frame. A
    //    round trip that lands anywhere near another pipe then ping-pongs at
    //    60Hz, reloading the level and stacking two sounds per frame. The
    //    cooldown below makes one press mean one warp.
    if (m_warpCooldown > 0.0f) m_warpCooldown -= dt;

    if (m_player && m_warpCooldown <= 0.0f) {
        std::string warpTarget;
        sf::Vector2f warpExit{0.0f, 0.0f};
        bool warping = false;

        for (const auto& entity : m_entities) {
            if (auto pipe = dynamic_cast<Pipe*>(entity.get())) {
                if (pipe->checkWarp(*m_player)) {
                    warpTarget = pipe->getTargetLevel();
                    warpExit   = pipe->getExitPosition();
                    warping    = true;
                    break;
                }
            }
        }

        if (warping) {
            m_warpCooldown = 0.75f;
            SoundManager::getInstance().playSound("pipe");
            EventBus::getInstance().publish({EventType::PlayerWarped, 0});

            if (!warpTarget.empty()) {
                loadLevelByPath(warpTarget, warpExit);
            } else if (warpExit.x != 0.0f || warpExit.y != 0.0f) {
                m_player->setPosition(warpExit);
                m_player->setVelocity({0.0f, 0.0f});
            }
            return;   // the world just changed underneath us; resume next frame
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
            killPlayer(m_player, "ran out of time");
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
            hudData.comboTimer = player->getComboTimer();
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
        if (m_player2) {
            snapshot.hasSecondPlayer = true;
            snapshot.secondPlayerState = m_player2->createSnapshot();
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

void PlayingState::syncBackdropGround() {
    // Find the top of the ground the player actually walks on, and hand it to the
    // backdrop so the hills and bushes stand on it.
    //
    // Scanned rather than assumed: the campaign levels put their surface on tile
    // row 21 of 23, but a generated level or a sub-level can be any height, and a
    // hardcoded screen line was wrong for all of them. The lowest row containing
    // a solid tile is the floor; its top edge is where decorations belong.
    int groundRow = -1;
    for (int y = m_tileMap.getHeight() - 1; y >= 0 && groundRow < 0; --y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            if (TileMap::getInfo(m_tileMap.getTileType(x, y)).isSolid) {
                groundRow = y;
                break;
            }
        }
    }
    if (groundRow < 0) {
        m_background.setWorldGroundY(0.0f);   // no floor at all: fall back
        return;
    }
    m_background.setWorldGroundY(static_cast<float>(groundRow) * Constants::TILE_SIZE);
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
                        // Themed terrain. This used to be brown-on-grey for every
                        // level in the game, so an ice cavern and a castle floor
                        // and a grass field all looked identical — and a
                        // generated level looked like nothing in particular.
                        const bool isTopExposed =
                            (y == 0) || (m_tileMap.getTileType(x, y - 1) == TileType::Empty);
                        switch (m_background.getTheme()) {
                            case BackgroundTheme::Underground:
                                frameKey = isTopExposed ? "solid_block_grey" : "brick_grey_inside";
                                break;
                            case BackgroundTheme::Castle:
                                frameKey = isTopExposed ? "castle_brick_white" : "brick_grey_inside";
                                break;
                            case BackgroundTheme::Ice:
                                frameKey = isTopExposed ? "solid_block_blue" : "brick_blue_inside";
                                break;
                            case BackgroundTheme::Overworld:
                            default:
                                frameKey = isTopExposed ? "solid_block_brown" : "brick_brown_inside";
                                break;
                        }
                        break;
                    }
                    case TileType::Brick:
                        frameKey = (m_background.getTheme() == BackgroundTheme::Ice)
                                       ? "brick_blue_one_side"
                                       : (m_background.getTheme() == BackgroundTheme::Underground ||
                                          m_background.getTheme() == BackgroundTheme::Castle)
                                             ? "brick_grey_one_side"
                                             : "brick_brown_side";
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

        renderMatchHud(target);

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
    // The bot's decision clock is not the simulation clock. Left running across
    // a pause it banks the whole paused duration and spends it as a burst of
    // decisions on the first frame back.
    if (m_aiController) m_aiController->setPaused(true);
}

void PlayingState::onResume() {
    m_suspended = false;
    SoundManager::getInstance().resumeMusic();
    if (m_aiController) m_aiController->setPaused(false);
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
        syncBackdropGround();
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

    spawnMatchParticipants();

    findActiveBoss();

    // Always recording, so "replay save" after something interesting happens
    // actually has the interesting thing in it. Bounded by kMaxFrames.
    ReplayRecorder::getInstance().startRecording(
        m_isProcedural ? "procedural" : LevelCatalog::nameFor(m_selectedLevelIndex));
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
    m_player = nullptr;
    // Every observer pointer into the vector that was just emptied. m_player2
    // and m_shadow used to survive a level reload as dangling pointers — the
    // warp path calls this and then rebuilds, so a two-player warp read freed
    // memory on the next frame's camera update.
    m_player2 = nullptr;
    m_shadow = nullptr;
    m_aiController.reset();
    Game::getInstance().setSecondPlayer(nullptr);
    InputManager::getInstance().registerPlayer(nullptr, 1);
    // The participants this vector held are gone, so their death records describe
    // nobody. Carrying `eliminated` across a level load would lock a player out
    // of the next level they were never eliminated in.
    m_death = DeathState{};
    m_death2 = DeathState{};
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

    // Everything that has to survive the swap. loadLevelByPath destroys the
    // Player and builds a new one, so anything not captured here is lost: the
    // power-up form was, which is why Fire Mario came out of a pipe as Small.
    int savedLives = Constants::INITIAL_LIVES;
    int savedCoins = 0;
    int savedScore = 0;
    Player::Form savedForm = Player::Form::Small;
    if (m_player) {
        savedLives = m_player->getLives();
        savedCoins = m_player->getCoins();
        savedScore = m_player->getScore();
        savedForm  = m_player->getForm();
    }

    cleanupTestScene();

    m_entities = std::move(levelData.entities);

    for (auto& entity : m_entities) {
        admitEntity(entity.get());
    }

    sf::Vector2f spawnPos = (spawnOverride.x != 0.0f || spawnOverride.y != 0.0f)
                                ? spawnOverride : levelData.spawnPoint;

    // A pipe exit is level data, and level data can be wrong: these used to name
    // tile (104,20) of a 65-tile-wide level, which dropped the player outside
    // the map into open air. Rather than trust it, check that the exit is inside
    // the destination and has ground beneath it, and fall back to the level's
    // own spawn point when it does not. Landing at the start of a room is a
    // visible oddity; landing in the void is an unrecoverable one.
    const float mapRight  = m_tileMap.getWidth()  * Constants::TILE_SIZE;
    const float mapBottom = m_tileMap.getHeight() * Constants::TILE_SIZE;
    auto standableBelow = [this, mapBottom](sf::Vector2f p) {
        for (float y = p.y; y < mapBottom; y += 8.0f) {
            if (TileMap::getInfo(m_tileMap.getTileAt(p.x, y)).isSolid) return true;
        }
        return false;
    };
    const bool insideMap = spawnPos.x >= 0.0f && spawnPos.x < mapRight &&
                           spawnPos.y >= 0.0f && spawnPos.y < mapBottom;
    if (!insideMap || !standableBelow(spawnPos)) {
        std::cerr << "[PlayingState] Spawn (" << spawnPos.x << ", " << spawnPos.y
                  << ") is outside " << chosenPath
                  << " or has no floor; using the level's own spawn point." << std::endl;
        spawnPos = levelData.spawnPoint;
    }

    // cleanupTestScene() nulled m_player, so spawnSelectedPlayer cannot carry the
    // stats itself — apply them here, through the same silent path.
    m_levelSpawnPoint = spawnPos;
    m_hasCheckpoint = false;
    spawnSelectedPlayer(spawnPos);
    if (m_player) {
        m_player->restoreStats(savedLives, savedCoins, savedScore);
        m_player->setForm(savedForm);
    }

    Game::getInstance().setTileMap(&m_tileMap);
    m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});
    m_background.setTheme(levelData.theme);
    syncBackdropGround();
    findActiveBoss();

    std::cout << "[PlayingState] Loaded sub-level / main level: " << chosenPath << " at spawn (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return true;
}


void PlayingState::regenerateProceduralLevel() {
    MapGenerator::generate(m_tileMap, m_entities, m_genConfig);
    m_background.setTheme(backdropForGeneratedTheme(m_genConfig.theme));
    syncBackdropGround();
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
    // A regenerated level is a different level. Keeping the old spawn point and
    // checkpoint would respawn the player into geometry that no longer exists.
    m_levelSpawnPoint = m_player ? m_player->getPosition() : sf::Vector2f(96.0f, 64.0f);
    m_checkpointPosition = m_levelSpawnPoint;
    m_hasCheckpoint = false;
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

void PlayingState::spawnMatchParticipants() {
    if (!m_player) return;

    Game& game = Game::getInstance();

    if (m_match.hasSecondPlayer()) {
        // Player 2 joins beside Player 1. Character is whichever of Mario/Luigi
        // Player 1 did not take, so the two are always visually distinct.
        const sf::Vector2f spawn = m_player->getPosition() + sf::Vector2f(48.0f, 0.0f);
        std::unique_ptr<Player> second = (m_selectedCharIndex == 1)
            ? std::unique_ptr<Player>(std::make_unique<Mario>(spawn))
            : std::unique_ptr<Player>(std::make_unique<Luigi>(spawn));
        second->restoreStats(game.difficulty().startingLives(), 0, 0);

        m_player2 = second.get();
        admitEntity(m_player2);
        m_entities.push_back(std::move(second));

        if (m_match.isCpuOpponent()) {
            // A bot has no keyboard, so it is deliberately NOT registered with
            // InputManager: registering it would let Player 2's arrow keys drive
            // the opponent the human is playing against.
            m_aiController = std::make_unique<AIController>(
                *m_player2, m_match.aiDifficulty, m_match.aiArchetype);
            std::cout << "[PlayingState] " << toString(m_match.mode) << ": CPU is "
                      << m_player2->getCharacterName() << " ("
                      << toString(m_match.aiDifficulty) << " "
                      << toString(m_match.aiArchetype) << ")" << std::endl;
        } else {
            // Player 2's bindings (arrows, M, N) have existed in InputManager
            // since it was written; nothing had ever registered a second player
            // against them (task 11.1).
            InputManager::getInstance().registerPlayer(m_player2, 1);
            std::cout << "[PlayingState] " << toString(m_match.mode) << ": P2 is "
                      << m_player2->getCharacterName() << std::endl;
        }

        // Enemy AI targets the nearer of the two from here on. Without this
        // every enemy in the level chased Player 1 and walked through Player 2.
        game.setSecondPlayer(m_player2);
        return;
    }

    if (m_match.isShadowChase()) {
        // Spawned on top of the player: for the first three seconds the shadow
        // has nothing to replay, so it stands exactly where the run began and
        // then sets off along the path the player took out of it.
        auto shadow = std::make_unique<ShadowMario>(m_player->getPosition());
        m_shadow = shadow.get();
        admitEntity(m_shadow);
        m_entities.push_back(std::move(shadow));
        std::cout << "[PlayingState] Shadow Chase: replaying at "
                  << m_shadow->getDelay() << "s delay." << std::endl;
    }
}

void PlayingState::updateShadow(float dt) {
    (void)dt;
    if (!m_shadow || !m_player) return;

    // Sampled here, before the entity update pass runs the shadow, so a packet
    // recorded this frame can never also be consumed this frame. The clock is
    // the level timer counting down, inverted into elapsed time: it advances
    // with the simulation and stops with it, which a wall clock would not.
    const float elapsed = Constants::LEVEL_TIME - m_levelTimer;
    m_shadow->recordFrame(elapsed, *m_player);
}

float PlayingState::shadowProximitySeconds() const {
    if (!m_shadow || !m_player) return -1.0f;
    if (!m_shadow->hasStarted()) return m_shadow->getDelay();

    // Not the configured delay: the *spatial* gap expressed as time. The shadow
    // is three seconds behind in the recording, but if the player has doubled
    // back they can be standing next to it, and that is what the gauge must
    // warn about.
    const float gap = std::abs(m_player->getPosition().x - m_shadow->getPosition().x) +
                      std::abs(m_player->getPosition().y - m_shadow->getPosition().y) * 0.5f;
    return gap / std::max(1.0f, Constants::RUN_SPEED);
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
    // Versus bounds hard, co-op bounds soft. In versus, whoever falls behind is
    // shoved along at the left edge — being dragged is the cost of falling
    // behind. In co-op there is nothing to lose by falling behind, so the
    // trailing player is left alone and the *leader* is blocked at the right
    // edge instead: the screen simply stops advancing until their partner
    // catches up, which is the mechanic the mode is built around.
    const bool softBounds = m_match.isCoop();
    for (Player* player : {m_player, m_player2}) {
        if (!player) continue;
        const AABB box = player->getBoundingBox();
        sf::Vector2f position = player->getPosition();
        bool moved = false;

        if (position.x < view.x + margin) {
            if (softBounds) continue;
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

void PlayingState::renderMatchHud(sf::RenderTarget& target) const {
    if (!m_player) return;

    // Deliberately drawn here rather than inside Hud: HudData describes one
    // player, and widening it would push multiplayer concerns into every
    // single-player HUD field.
    constexpr float kRowTop = Constants::WINDOW_HEIGHT - 56.0f;
    constexpr float kRowBottom = Constants::WINDOW_HEIGHT - 34.0f;
    const sf::Color kAccent(255, 216, 0);

    // --- Shadow Chase: one player, and a countdown to your own past ----------
    if (m_shadow) {
        const float seconds = shadowProximitySeconds();
        // Under a second of separation is the warning band: at that range the
        // shadow is close enough that the player needs to know without looking
        // away from where they are jumping.
        const bool danger = seconds >= 0.0f && seconds < 1.0f;

        UiRenderer::drawText(target, m_shadow->hasStarted() ? "SHADOW" : "SHADOW WAKING",
                             {24.0f, kRowTop}, 12,
                             danger ? sf::Color(255, 80, 80) : sf::Color(180, 130, 255));

        // The gauge: a bar that empties as the shadow closes in. Three seconds
        // of separation is full, nothing is contact.
        constexpr float kBarWidth = 180.0f;
        constexpr float kBarHeight = 10.0f;
        const float fill = std::clamp(seconds / m_shadow->getDelay(), 0.0f, 1.0f);

        sf::RectangleShape track({kBarWidth, kBarHeight});
        track.setPosition({24.0f, kRowBottom});
        track.setFillColor(sf::Color(0, 0, 0, 180));
        track.setOutlineColor(sf::Color(180, 130, 255, 200));
        track.setOutlineThickness(1.0f);
        target.draw(track);

        sf::RectangleShape level({kBarWidth * fill, kBarHeight});
        level.setPosition({24.0f, kRowBottom});
        level.setFillColor(danger ? sf::Color(255, 60, 60) : sf::Color(150, 90, 230));
        target.draw(level);

        if (danger) {
            UiRenderer::drawText(target, "BEHIND YOU",
                                 {Constants::WINDOW_WIDTH * 0.5f, kRowBottom}, 12,
                                 sf::Color(255, 80, 80), true);
        }
        return;
    }

    if (!m_player2) return;

    // --- Co-op: one shared pool, both forms ----------------------------------
    if (m_match.isCoop()) {
        // Shared lives and score, so showing them twice would imply two pools.
        // The per-player line carries what actually differs: who they are and
        // what form they are in.
        const std::string pool = "TEAM " + std::to_string(m_player->getScore() +
                                                          m_player2->getScore()) +
                                 "  x" + std::to_string(m_player->getLives() +
                                                        m_player2->getLives());
        UiRenderer::drawText(target, pool, {24.0f, kRowTop}, 12, kAccent);
        const std::string forms = "P1 " + m_player->getCharacterName() +
                                  "   P2 " + m_player2->getCharacterName();
        UiRenderer::drawText(target, forms, {24.0f, kRowBottom}, 12,
                             ColorPalette::get(ColorPalette::Role::Player));
        return;
    }

    // --- Versus: two scores and who is ahead ---------------------------------
    // The second row names the opponent rather than always saying "P2": against
    // a bot, which archetype is playing changes how you should play, so it is
    // worth a permanent line rather than a menu the player has already left.
    const std::string p2Label =
        m_aiController ? std::string("CPU ") + m_aiController->policyName() : "P2";

    const std::string p1Line = "P1 " + std::to_string(m_player->getScore()) +
                               "  x" + std::to_string(m_player->getLives());
    const std::string p2Line = p2Label + " " + std::to_string(m_player2->getScore()) +
                               "  x" + std::to_string(m_player2->getLives());

    UiRenderer::drawText(target, p1Line, {24.0f, kRowTop}, 12,
                         ColorPalette::get(ColorPalette::Role::Player));
    UiRenderer::drawText(target, p2Line, {24.0f, kRowBottom}, 12,
                         ColorPalette::get(ColorPalette::Role::Item));

    // Who is winning, which is the whole point of versus.
    const int lead = m_player->getScore() - m_player2->getScore();
    const std::string status = (lead == 0) ? "TIED"
                             : (lead > 0)  ? "P1 LEADS BY " + std::to_string(lead)
                                           : p2Label + " LEADS BY " + std::to_string(-lead);
    UiRenderer::drawText(target, status,
                         {Constants::WINDOW_WIDTH * 0.5f, kRowBottom},
                         12, kAccent, true);
}

void PlayingState::applySnapshot(const GameSnapshot& snapshot) {
    m_levelTimer = snapshot.levelTimer;
    m_camera.snapTo(snapshot.cameraCenter);

    if (m_player) {
        m_player->restoreMemento(snapshot.playerState);
    }
    // Only when the snapshot actually recorded one: replaying a single-player
    // recording in a two-player level must not zero Player 2's score from a
    // default-constructed PlayerSnapshot.
    if (m_player2 && snapshot.hasSecondPlayer) {
        m_player2->restoreMemento(snapshot.secondPlayerState);
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

void PlayingState::forgetEntity(Entity* entity) {
    if (!entity) return;

    if (entity == m_activeBoss) {
        releaseBossArena();
        m_activeBoss = nullptr;
    }
    // A carried shell that expires while held would leave the player animating
    // a carry forever, and throwHeldEntity() would dereference freed memory.
    if (m_player && m_player->getHeldEntity() == entity)  m_player->releaseHeldEntity();
    if (m_player2 && m_player2->getHeldEntity() == entity) m_player2->releaseHeldEntity();

    // Same rule as the boss pointer: drop the observer before the unique_ptr
    // behind it is released. The shadow outlives the level only if nothing
    // pruned it, and updateShadow() would read freed memory the frame after.
    if (entity == m_shadow) m_shadow = nullptr;
    if (entity == m_player2) {
        m_player2 = nullptr;
        m_aiController.reset();
        Game::getInstance().setSecondPlayer(nullptr);
    }
}

void PlayingState::releaseBossArena() {
    if (!m_arenaLocked) return;
    m_camera.setBounds(m_preArenaCameraBounds);
    m_camera.setScrollMode(Camera::ScrollMode::Free);
    m_arenaLocked = false;
    // Back to the level's own music: the fight track kept playing over the
    // victory tally otherwise.
    SoundManager::getInstance().restoreLevelBGM();
    std::cout << "[PlayingState] Boss arena released." << std::endl;
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
        releaseBossArena();
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

PlayingState::DeathState* PlayingState::deathStateFor(const Player* who) {
    if (who && who == m_player2) return &m_death2;
    if (who && who != m_player)  return nullptr;   // the shadow, or a stale pointer
    return &m_death;
}

bool PlayingState::anyDeathInProgress() const {
    return m_death.phase != DeathPhase::None || m_death2.phase != DeathPhase::None;
}

void PlayingState::killPlayer(Player* who, const char* reason) {
    // Null means Player 1: the debug key and the level-timer path have no
    // particular player in mind.
    if (!who) who = m_player;
    if (!who) return;

    DeathState* death = deathStateFor(who);
    if (!death) return;   // not a participant — the shadow cannot die

    // One death per death. This used to run its whole body every frame the
    // player was still out of bounds: the pit check fires again next frame
    // because nothing has moved the corpse, so "lost_life" played sixty times a
    // second and — worse — fadeOut() was restarted every frame, which resets its
    // elapsed time. The fade therefore never completed, its callback never ran,
    // and the game-over screen never appeared. Falling into the void simply hung
    // the game with a scream.
    if (death->phase != DeathPhase::None) return;
    // And nothing may kill a player who is already out. Their corpse stays below
    // the level, so the void check fired again every frame and an eliminated
    // Player 1 put the match into an endless loop of death jingles.
    if (death->eliminated) return;

    const bool isSecond = (who == m_player2);
    std::cout << "[PlayingState] " << (isSecond ? "Player 2 " : "Player 1 ") << reason
              << "." << std::endl;

    // Set the phase BEFORE publishing: the PlayerDied subscription calls back
    // into here, and the guard above is what absorbs it.
    death->phase  = DeathPhase::Falling;
    death->timer  = DEATH_FALL_SECONDS;
    death->reason = reason;

    // Attribute the death to the shadow when the shadow actually caused it. The
    // damage arrives through Player::takeDamage and surfaces as a generic
    // PlayerDied, which would report "was defeated" for the one death the mode
    // is entirely about.
    //
    // This asked whether the two were overlapping, which is not the same
    // question: a player killed by a Goomba while standing on the spot their
    // shadow was parked on was reported as having been caught by it. The shadow
    // is told when it lands a hit, so this reads a fact.
    if (m_shadow && m_shadow->caughtPlayerRecently()) {
        death->reason = "was caught by their own shadow";
    }

    // Pop up, then fall through the level. The respawn happens when the fall is
    // over, not on the frame of the hit.
    who->beginDeathFall();

    // Deliberately does NOT re-publish PlayerDied. Player::powerDown() already
    // published it for a damage death, and re-publishing meant StatisticsTracker
    // counted two deaths for every one — which is why the game-over panel's
    // DEATHS line ran at double the real figure. The void and timeout paths
    // publish it themselves, since nothing else did.
}

void PlayingState::updateDeathSequence(float dt) {
    // Both participants advance independently. With one shared phase, a second
    // death during the first player's fall was silently dropped.
    for (Player* who : {m_player, m_player2}) {
        if (!who) continue;
        DeathState* death = deathStateFor(who);
        if (!death || death->phase != DeathPhase::Falling) continue;

        death->timer -= dt;
        const float elapsed = DEATH_FALL_SECONDS - death->timer;

        // Either the timer runs out or the corpse has visibly left the screen,
        // whichever comes first — a death high above the ground should not make
        // the player wait out a fall they cannot see. But not before
        // DEATH_FALL_MINIMUM: a death near the bottom of the view cleared the
        // screen edge in a few frames, so the death "animation" was over before
        // it could be seen at all.
        const float offScreenY = m_camera.getVisibleBounds().y +
                                 m_camera.getVisibleBounds().height + 96.0f;
        const bool leftScreen = who->getPosition().y > offScreenY &&
                                elapsed >= DEATH_FALL_MINIMUM;
        if (death->timer > 0.0f && !leftScreen) continue;

        death->phase = DeathPhase::None;
        who->endDeathFall();
        who->loseLife();

        if (who->getLives() > 0) {
            respawnPlayer(who);
            continue;
        }

        // Out of lives.
        death->eliminated = true;

        // In versus, one player running out does not end the run — the other is
        // still playing, and ending it early would hand them the win by default.
        // The corpse is parked out of the way so no hazard check finds it again.
        if (m_match.hasSecondPlayer() && !allPlayersOut()) {
            std::cout << "[PlayingState] " << (who == m_player2 ? "Player 2" : "Player 1")
                      << " is out; the other continues." << std::endl;
            who->destroy();
            forgetEntity(who);
            continue;
        }

        std::cout << "[PlayingState] Game Over — " << death->reason << "." << std::endl;
        EventBus::getInstance().publish({EventType::GameOver, 0});

        // Taken *now*: by the time the fade completes and the callback runs, this
        // state is being replaced and m_player is gone.
        const RunSummary summary = buildRunSummary();
        death->phase = DeathPhase::GameOver;   // nothing else may start a transition
        ScreenTransitionManager::getInstance().fadeOut(0.6f, [summary]() {
            Game::getInstance().changeState(std::make_unique<GameOverState>(summary));
        });
        return;
    }
}

void PlayingState::makeSpawnSafe(Player* who, sf::Vector2f respawn) {
    if (!who) return;

    // Respawning was a teleport, not a reset: the entity list was never touched,
    // so an enemy that had wandered onto the checkpoint while the player was
    // dying was still standing there — and the player came back with zero
    // invincibility frames, because beginDeathFall() zeroes them and nothing put
    // them back. Landing on that enemy cost the next life immediately, and since
    // the enemy never moved either, the loop ran until the lives were gone. That
    // is the "spawn killing" the report describes.
    //
    // Two halves to the fix. Clear the immediate area, so nothing is *already*
    // touching the player on frame one:
    constexpr float kClearRadius = Constants::TILE_SIZE * 2.5f;
    int cleared = 0;
    for (const auto& entity : m_entities) {
        if (!entity || !entity->isActive()) continue;
        if (entity->getCategory() != EntityCategory::Enemy) continue;

        const sf::Vector2f delta = entity->getPosition() - respawn;
        if (std::abs(delta.x) > kClearRadius || std::abs(delta.y) > kClearRadius) continue;

        // Destroyed rather than nudged: moving it leaves it walking straight back
        // in, and a checkpoint the player cannot survive arriving at is worse
        // than a level one enemy lighter.
        entity->destroy();
        forgetEntity(entity.get());
        ++cleared;
    }
    if (cleared > 0) {
        std::cout << "[PlayingState] Cleared " << cleared
                  << " enemy(ies) from the respawn point." << std::endl;
    }

    // And grant landing invincibility, so anything that walks back in during the
    // next second and a half cannot convert the respawn into another death.
    who->setInvincible(1.5f);
}

void PlayingState::respawnPlayer(Player* who) {
    if (!who) return;
    const bool isFirst = (who == m_player);

    // The last checkpoint if one was reached, otherwise the level's own spawn
    // point — never a hardcoded corner.
    sf::Vector2f respawn = m_hasCheckpoint ? m_checkpointPosition : m_levelSpawnPoint;

    // If the spawn point is itself in the void, respawning there is an infinite
    // death loop that drains every life in a second.
    const float mapBottom = m_tileMap.getHeight() * Constants::TILE_SIZE;
    const float mapRight  = m_tileMap.getWidth()  * Constants::TILE_SIZE;
    if (respawn.y > mapBottom || respawn.x < 0.0f || respawn.x > mapRight) {
        respawn = {Constants::TILE_SIZE * 3.0f, Constants::TILE_SIZE * 2.0f};
        std::cerr << "[PlayingState] Respawn point was outside the map; "
                     "using the top-left of the level instead." << std::endl;
    }
    // Player 2 comes back beside Player 1's spawn rather than inside it, so the
    // two do not resolve out of each other on the first frame.
    if (!isFirst) respawn.x += 48.0f;

    who->setPosition(respawn);
    who->setVelocity({0.0f, 0.0f});
    who->setGrounded(false);
    // A corpse must not still be carrying a shell it picked up before dying.
    who->dropHeldEntity();
    makeSpawnSafe(who, respawn);

    if (!isFirst) {
        std::cout << "[PlayingState] Player 2 respawned. Lives remaining: "
                  << who->getLives() << std::endl;
        return;
    }

    m_camera.snapTo(respawn);

    // The chase restarts with the life. The shadow was left holding the dead
    // life's path, which the correction lerp then dragged it back across the
    // level to rejoin — arriving on top of the player and taking the next life
    // seconds after the respawn.
    if (m_shadow) m_shadow->resetChase(respawn);
    // Same reasoning for the bot: a fresh life is a fresh episode, and a policy
    // that kept its committed direction would walk straight back into whatever
    // just killed the player.
    if (m_aiController) m_aiController->reset();

    // A fresh clock per life, so a timeout death is recoverable.
    m_levelTimer = Constants::LEVEL_TIME * Game::getInstance().difficulty().levelTimeScale();
    m_timeWarningFired = false;

    SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);
    ScreenTransitionManager::getInstance().fadeIn(0.35f);

    std::cout << "[PlayingState] Respawned. Lives remaining: "
              << who->getLives() << std::endl;
}

RunSummary PlayingState::buildRunSummary() const {
    RunSummary summary;
    summary.levelIndex      = m_selectedLevelIndex;
    summary.characterIndex  = m_selectedCharIndex;
    summary.isProcedural    = m_isProcedural;
    summary.generatorConfig = m_genConfig;
    summary.starCoins = static_cast<int>(std::count(m_starCoinsCollected.begin(),
                                                    m_starCoinsCollected.end(), true));
    summary.match = m_match;
    summary.cause = m_death.reason;
    summary.caughtByShadow = m_shadow && m_shadow->caughtPlayerRecently();
    if (m_player) {
        summary.score         = m_player->getScore();
        summary.coins         = m_player->getCoins();
        summary.characterName = m_player->getCharacterName();
    }
    if (m_player2) {
        summary.opponentScore = m_player2->getScore();
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
    // The death records have to go too. Pause is reachable during the death fall,
    // and its "Restart Level" callback lands here — leaving phase == Falling, so
    // updateDeathSequence() immediately ran the tail of the old death on the
    // brand-new player and took a life off a fresh run.
    m_death = DeathState{};
    m_death2 = DeathState{};
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

