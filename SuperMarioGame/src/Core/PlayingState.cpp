#define NOMINMAX
#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/PauseState.hpp"
#include "Core/VictoryState.hpp"
#include "Core/GameOverState.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/CampaignProgress.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/BridgeAxe.hpp"
#include "Entities/Castle.hpp"
#include "Entities/Flagpole.hpp"
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
#include <vector>


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
                           int characterIndex, int levelIndex, MatchConfig match, bool isEndless,
                           int pendingLoadSlot, bool isAttractDemo,
                           std::string customLevelPath, bool isPlaytest)
    : m_match(match),
      m_selectedCharIndex(characterIndex),
      m_selectedLevelIndex(LevelCatalog::isValidIndex(levelIndex) ? levelIndex : 0),
      m_startInEditor(startInEditor), m_isProcedural(isProcedural),
      m_customLevelPath(std::move(customLevelPath)), m_isPlaytest(isPlaytest),
      m_genConfig(genConfig), m_pendingLoadSlot(pendingLoadSlot),
      m_isAttractDemo(isAttractDemo), m_isEndless(isEndless) {
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

    // Every run starts honest. Cheats are a per-take recording aid, so they are
    // reset at the one boundary that is unambiguously "a new run has begun"
    // rather than left to be turned off by hand — the alternative is a player
    // who tried immortality once and then spends the campaign wondering why
    // nothing can kill them. This also clears the taint flag, so the next run's
    // score is eligible for the high-score table again.
    //
    // A level TRANSITION (1-1 to 1-2) constructs a new PlayingState and so also
    // resets: mildly annoying mid-recording, and the right default. Toggling a
    // cheat back on costs one click.
    Game::getInstance().debugCheats().resetForNewRun();

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
    m_runElapsed = 0.0f;
    m_tileAnimTimer = 0.0f;

    if (m_isProcedural) {
        cleanupTestScene();
        m_lastLevelUnverified = !MapGenerator::generateSolvable(m_tileMap, m_entities, m_genConfig);
        if (m_lastLevelUnverified) {
            std::cerr << "[PlayingState] WARNING: procedural level shipped unverified — "
                         "every solvability reseed attempt failed" << std::endl;
        }
        m_background.setTheme(backdropForGeneratedTheme(m_genConfig.theme));
        syncBackdropGround();
        syncVoidPlane();
        settleEndOfLevelScenery();
        refreshWorldLabel(m_isProcedural ? std::string() : LevelCatalog::pathFor(m_selectedLevelIndex));

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

        if (m_isEndless) {
            // The first chunk came from the same generate() call every
            // procedural run uses, which always places a flagpole and a
            // castle near its own exitX (MapGenerator.cpp). Endless Mode has
            // neither — there is nothing to reach, only how far you get — so
            // both are removed once, here, rather than teaching the generator
            // a mode it does not otherwise need to know about.
            for (auto it = m_entities.begin(); it != m_entities.end(); ) {
                const std::string type = (*it)->getTypeName();
                if (type == "flagpole" || type == "castle") {
                    it = m_entities.erase(it);
                } else {
                    ++it;
                }
            }
            m_worldLabel = "ENDLESS";
        }

        // Without this a procedurally generated Bowser was inert scenery:
        // findActiveBoss() was called only from setupTestScene() and
        // loadLevelByPath(), never from any procedural path, so m_activeBoss
        // stayed null — updateBossArena() returned immediately, syncBossHud()
        // reported bossActive=false, and there was no health bar, no battle
        // music, no camera lock and nothing for chopBridge() to defeat.
        findActiveBoss();
    } else {
        setupTestScene();
    }

    // Before the editor can place or erase anything: without an admitter it
    // pushes raw into m_entities and frees Players out from under m_player.
    m_mapEditor.setEntityAdmitter(&m_editorBridge);
    m_mapEditor.setSpawnPoint(m_levelSpawnPoint);

    if (m_startInEditor && !m_mapEditor.isActive()) {
        m_mapEditor.toggleActive();
    }

    // Auto-save at checkpoint
    m_checkpointSub = EventBus::ScopedSubscription(EventType::CheckpointActivated, [this](const GameEvent& ev) {
        // Remember where to respawn. Death used to teleport to a hardcoded
        // (96,64) regardless of level or progress (audit G-3).
        if (m_player) {
            m_checkpointPosition = m_player->getPosition();
            m_hasCheckpoint = true;
        }
        int activeSlot = Game::getInstance().getActiveSlot();
        if (!m_entities.empty() && m_entities[0]) {
            if (auto* player = dynamic_cast<Player*>(m_entities[0].get())) {
                // Real level, not the "1, Level 1" this used to hardcode
                // regardless of where the checkpoint actually was (D30): every
                // autosave recorded World 1-1 even when the player was three
                // levels in, so LOAD GAME had nothing but level 1 to resume
                // into no matter which slot or level. levelId is 1-based to
                // match the convention getSlotPreview()/loadFromSlot() already
                // use.
                const int levelId = m_selectedLevelIndex + 1;
                const std::string levelName = m_isProcedural ? "Procedural"
                                                              : LevelCatalog::nameFor(m_selectedLevelIndex);
                bool success = Serializer::saveGame(activeSlot, *player, levelId, levelName, Constants::LEVEL_TIME, player->getPosition().x, player->getPosition().y, std::vector<bool>(m_starCoinsCollected.begin(), m_starCoinsCollected.end()));
                if (success) {
                    std::cout << "[Auto-Save] Progress saved to Slot " << activeSlot << " at checkpoint!" << std::endl;
                }
            }
        }
    });

    // Fireball Shooting Event Listener
    m_fireballSub = EventBus::ScopedSubscription(EventType::PlayerShotFireball, [this](const GameEvent& ev) {
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

        // Max 2 active fireballs on screen, unless Debug > Cheats' INFINITE
        // FIREBALLS is on — the cap is the half of the ration that lives here;
        // Player::canShootFireball() owns the cooldown half.
        if (activeFireballs < 2 ||
            Game::getInstance().debugCheats().liftsFireballCap()) {
            float dir = player->isFacingRight() ? 1.0f : -1.0f;
            sf::Vector2f spawnPos = player->getPosition() + sf::Vector2f(dir * 16.0f, 8.0f);
            sf::Vector2f vel(dir * 350.0f, 50.0f);

            auto fireball = m_fireballPool.acquire(spawnPos, vel);
            // Was pushed straight onto the list, so it never had its animations
            // wired and fell back to hand-drawn circles every time.
            queueSpawn(std::move(fireball));

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
    m_playerDiedSub = EventBus::ScopedSubscription(
        EventType::PlayerDied, [this](const GameEvent& ev) {
            Player* who = nullptr;
            if (const Player* const* asPlayer = std::any_cast<Player*>(&ev.data)) {
                who = const_cast<Player*>(*asPlayer);
            }
            killPlayer(who, "was defeated");
        });

    m_starCoinsCollected = {false, false, false};
    m_starCoinSub = EventBus::ScopedSubscription(EventType::StarCoinCollected, [this](const GameEvent& ev) {
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
    m_enemyDefeatedSub = EventBus::ScopedSubscription(EventType::EnemyDefeated, [this](const GameEvent&) {
        // The event only carries a score value, so locate the enemy that just went
        // inactive to place the burst and the flip animation at its last position.
        if (!m_enemySheet) return;
        for (const auto& entity : m_entities) {
            auto enemy = dynamic_cast<Enemy*>(entity.get());
            if (!enemy || enemy->isActive()) continue;

            const sf::Vector2f pos = enemy->getPosition();
            m_particleEmitter.burst(pos + sf::Vector2f(16.0f, 16.0f), ParticleType::Stomp);
            // A star-power kill gets the spin launch (SPEC 15.2's "All enemies
            // (Star kill)" row) instead of the ordinary flip — checked against
            // whichever player is currently starred, since CollisionResolver
            // does not thread the kill method through this event (it only ever
            // carried a score value; widening it would touch every enemy class
            // that publishes EnemyDefeated). This event fires synchronously
            // from within the same collision-resolution call that granted the
            // kill, so the player's star state has not had a chance to expire
            // between the two.
            const bool starKill = (m_player && m_player->hasStarPower()) ||
                                  (m_player2 && m_player2->hasStarPower());
            EntityDeathEffect::getInstance().spawnDeathEffect(
                pos, m_enemySheet->getSprite("goomba_brown_move_0"),
                starKill ? DeathEffectType::StarKillSpin : DeathEffectType::EnemyFlip);
            break;
        }
    });

    m_blockBrokenSub = EventBus::ScopedSubscription(EventType::BlockBroken, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter() + sf::Vector2f(0.0f, -24.0f),
                                              ParticleType::BrickBreak);
    });

    m_coinParticleSub = EventBus::ScopedSubscription(EventType::CoinCollected, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter(), ParticleType::CoinSparkle);
    });

    m_playerDamagedSub = EventBus::ScopedSubscription(EventType::PlayerDamaged, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(m_player->getBoundingBox().getCenter(), ParticleType::DeathPoof);
    });

    // ParticleType::Combo: the fourth declared-but-unused particle type
    // (R7 audit). Same event SoundManager's combo pitch escalation listens for
    // (Player.cpp publishes ComboHit once per chained kill); kept as a
    // separate subscriber here rather than widening either existing one.
    m_comboParticleSub = EventBus::ScopedSubscription(EventType::ComboHit, [this](const GameEvent&) {
        if (m_player) m_particleEmitter.burst(
            m_player->getBoundingBox().getCenter() + sf::Vector2f(0.0f, -32.0f), ParticleType::Combo);
    });

    // --- Question blocks: actually produce the item they announce ---
    m_powerUpSub = EventBus::ScopedSubscription(EventType::PowerUpRequested, [this](const GameEvent& ev) {
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
        queueSpawn(std::move(item));
    });

    // --- Entities asking for other entities (Lakitu's Spinies, Hammer Bro's
    // hammers). Entities have no handle on the world list, so they publish and
    // this performs the spawn (audit B-6, B-7). ---
    m_entitySpawnSub = EventBus::ScopedSubscription(EventType::EntitySpawnRequested, [this](const GameEvent& ev) {
        if (!ev.data.has_value() || ev.data.type() != typeid(EntitySpawnRequest)) return;
        const auto request = std::any_cast<EntitySpawnRequest>(ev.data);

        // Keep a lid on projectiles so a long fight cannot flood the world.
        if (m_entities.size() + m_pendingSpawns.size() >= 400) return;

        auto spawned = spawnProjectile(request.type, request.position, request.velocity);
        if (!spawned) return;
        spawned->setVelocity(request.velocity);
        queueSpawn(std::move(spawned));
    });

    // --- P-Switch and POW block: the two items that published an event and
    // then did nothing at all. Both effects are the level's business, not the
    // item's, so they are performed here where the tilemap and the entity list
    // are in reach. ---
    m_pSwitchSub = EventBus::ScopedSubscription(EventType::PSwitchActivated, [this](const GameEvent& ev) {
        float seconds = 15.0f;
        if (const float* asFloat = std::any_cast<float>(&ev.data)) seconds = *asFloat;
        beginPSwitch(seconds);
    });

    m_powSub = EventBus::ScopedSubscription(EventType::POWBlockHit, [this](const GameEvent&) {
        detonatePOW();
    });

    m_bridgeSub = EventBus::ScopedSubscription(EventType::BridgeChopped, [this](const GameEvent&) {
        // Every axe has to be reached before the bridge goes; only the last one
        // chops. m_axesRemaining <= 1 covers the single-axe fight, the last axe
        // of a Hard fight, and m_axesTotal == 0 (an axe the map editor dropped
        // into a running level, which configureBridgeAxes() never saw) — all
        // three mean "this touch is the one that drops the bridge".
        if (m_axesRemaining > 1) {
            --m_axesRemaining;
            std::cout << "[PlayingState] Axe reached; " << m_axesRemaining
                      << " still to go before the bridge falls." << std::endl;
            // A cue for the axes that do NOT chop, so reaching one is not
            // silence. Louder feedback than nothing, cheaper than a new asset.
            m_camera.triggerScreenShake(4.0f, 0.2f);
            return;
        }
        m_axesRemaining = 0;
        chopBridge();
    });

    // --- Level complete: the flagpole fires this; without a listener the game
    // could be flagged but never actually finished (audit G-1). ---
    m_levelCompleteSub = EventBus::ScopedSubscription(EventType::LevelComplete, [this](const GameEvent&) {
        if (m_levelComplete) return;   // flagpole can fire more than once
        // "No escape until defeated" (SPEC 6.4) applies to finishing the level,
        // not only to leaving the arena: a boss level's flagpole must not clear
        // the level while its boss is still alive. The arena's position clamp
        // (updateBossArena) is meant to make this unreachable in the first
        // place, but a misplaced arena/flagpole in level data (as level_2's
        // Boom Boom arena once was) must not be able to skip the fight.
        if (!levelMayComplete()) return;
        m_levelComplete = true;
        m_levelCompleteTimer = 0.0f;
        m_hasLevelCompleteCastle = false;
        // The castle answers the flagpole: its own flag climbs the gatehouse as
        // the level's does down the pole. Its door is also where the player
        // walks to next, instead of standing at the flagpole until the summary
        // screen cuts in.
        for (const auto& entity : m_entities) {
            if (auto* castle = dynamic_cast<Castle*>(entity.get())) {
                castle->raiseFlag();
                m_hasLevelCompleteCastle = true;
                // The box, not WIDTH_TILES: a themed castle can be a few pixels
                // wider (Castle::setFrame), and the door is at its centre.
                m_levelCompleteCastleTarget = castle->getPosition() +
                    sf::Vector2f{castle->getBoundingBox().width * 0.5f, 0.0f};
            }
        }
        std::cout << "[PlayingState] Level complete!" << std::endl;
    });

    // --- Checkpoint: remember where to respawn, then auto-save ---
    m_checkpointPosition = m_player ? m_player->getPosition() : sf::Vector2f(96.0f, 64.0f);
    m_hasCheckpoint = false;

    // --- Menu-driven Load Game: this instance exists purely to give
    // loadFromSlot somewhere to run. DevPanel's Save/Load Slots panel is the
    // only other caller, and it always has a PlayingState already running to
    // call into; MenuState does not, so it asks for one built specifically to
    // load into, via this constructor argument. Reuses the exact same private
    // method (and the adoptPlayer use-after-free fix it depends on) rather
    // than forking any of it. ---
    if (m_pendingLoadSlot > 0) {
        loadFromSlot(m_pendingLoadSlot);
        m_pendingLoadSlot = 0;
    }

    // --- Screen transition: fade the level in on entry ---
    ScreenTransitionManager::getInstance().reset();
    ScreenTransitionManager::getInstance().fadeIn(0.45f);
}

void PlayingState::exit() {
    // GameStateManager calls exit() explicitly (applyPendingOps()'s
    // PendingKind::Change case, and clearStates()) before pop_back() destroys
    // the outgoing state — but ~PlayingState() also calls exit() unconditionally
    // as a safety net, so pop_back()'s destructor call ran the full teardown a
    // second time on every single state transition: cleanupTestScene(), every
    // EventBus unsubscribe, InputManager::registerPlayer(nullptr, ...),
    // Game::setPlayer(nullptr)/setSecondPlayer(nullptr)/setTileMap(nullptr)/
    // setMatchConfig({}), m_minimap.reset(), EntityDeathEffect::clear() — all of
    // it. Harmless by luck (every one of those happens to be idempotent), but a
    // real double-execution, not merely the duplicate "Exiting PlayingState"
    // line a live pause -> quit-to-menu script surfaced. Re-implements the same
    // guard as A/fix/duplicate-playingstate-exit (f169a6c, off a much older
    // dev) on current PlayingState.
    if (m_hasExited) return;
    m_hasExited = true;

    std::cout << "Exiting PlayingState" << std::endl;

    // No cheat follows the player out of the level. Slow motion in particular is
    // applied by Game::run's accumulator, which keeps running over the menus, so
    // leaving a 0.2x take on would crawl every screen after it. The taint is
    // deliberately kept — GameOverState and VictoryState are entered after this
    // and still have to refuse the high-score table for a cheated run.
    Game::getInstance().debugCheats().disengageAll();

    cleanupTestScene();

    // RAII subscription tokens (audit X-7): reset() unsubscribes immediately
    // rather than waiting for ~PlayingState(). These lambdas capture `this`, so
    // they must go before the state is destroyed or the next publish
    // dereferences freed memory — exit() always runs immediately before
    // GameStateManager destroys this object (see GameStateManager.cpp), so this
    // ordering is unchanged from before; there is just no sentinel to check.
    m_checkpointSub.reset();
    m_playerDiedSub.reset();
    m_fireballSub.reset();
    m_starCoinSub.reset();
    m_pSwitchSub.reset();
    m_powSub.reset();
    m_bridgeSub.reset();
    m_enemyDefeatedSub.reset();
    m_blockBrokenSub.reset();
    m_coinParticleSub.reset();
    m_playerDamagedSub.reset();
    m_powerUpSub.reset();
    m_levelCompleteSub.reset();
    m_entitySpawnSub.reset();

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
    if (m_isAttractDemo) {
        // SPEC 10.2: "dismissed instantly by any key". Handled here, before any
        // of the ordinary per-key logic below runs, so a dismiss key that also
        // happens to be a game action (Escape opening pause, Num1 faking a
        // coin, ...) does not also perform that action on the way out — this
        // returns and does nothing else. changeState() only queues the switch
        // (GameStateManager::applyPendingOps runs at the top of the *next*
        // handleInput/update call), so this same key event cannot also reach
        // the MenuState it is switching to.
        if (event.is<sf::Event::KeyPressed>()) {
            Game::getInstance().changeState(std::make_unique<MenuState>());
        }
        return;
    }

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
                    [this]() {
                        ScreenTransitionManager::getInstance().reset();
                        leaveToCallingScreen();
                    }));
                return;
            }

            if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                leaveToCallingScreen();
            }

            // M or Tab toggles the minimap. Minimap subscribes to this event itself.
            if (keyPressed->code == sf::Keyboard::Key::M || keyPressed->code == sf::Keyboard::Key::Tab) {
                EventBus::getInstance().publish({EventType::MinimapToggled, 0});
            }

            // R21: the number row publishes gameplay events directly -- coins,
            // deaths, level completion, boss defeat. Useful for exercising the
            // HUD and the achievement rules, and indefensible in a release
            // build, where Num6 finishes the level and Num4 kills the player.
            // Behind the same Options > DEBUG MODE flag as the dev panel.
            if (Game::getInstance().getDebugMode()) {
                // Debug > Cheats on the function row, so the switches this
                // session is most likely to flip mid-take (immortality, a hidden
                // HUD) do not need the mouse and a collapsed panel reopened
                // while the camera is rolling. F1 is the editor and F5 is
                // playtest/attract, so both are skipped. The panel is the
                // discoverable surface; these are the shortcut to it, and both
                // read and write the one DebugCheats object.
                DebugCheats& cheats = Game::getInstance().debugCheats();
                // Reported on stderr as well as flipped, because the panel these
                // shadow is collapsed by default: without a line here there is
                // no way to tell a shortcut that landed from one that did not.
                auto toggleAndReport = [&cheats](DebugCheats::Cheat cheat) {
                    cheats.toggle(cheat);
                    std::cout << "[Cheats] " << DebugCheats::label(cheat) << ": "
                              << (cheats.isOn(cheat) ? "ON" : "off") << std::endl;
                };
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::F2:
                        toggleAndReport(DebugCheats::Cheat::Immortal); break;
                    case sf::Keyboard::Key::F3:
                        toggleAndReport(DebugCheats::Cheat::Invincible); break;
                    case sf::Keyboard::Key::F4:
                        toggleAndReport(DebugCheats::Cheat::InfiniteLives); break;
                    case sf::Keyboard::Key::F6:
                        toggleAndReport(DebugCheats::Cheat::FreezeTimer); break;
                    case sf::Keyboard::Key::F7:
                        toggleAndReport(DebugCheats::Cheat::HideHud); break;
                    case sf::Keyboard::Key::F8:
                        toggleAndReport(DebugCheats::Cheat::Noclip); break;
                    case sf::Keyboard::Key::F9:
                        toggleAndReport(DebugCheats::Cheat::FreeCamera); break;
                    case sf::Keyboard::Key::F10:
                        toggleAndReport(DebugCheats::Cheat::InfiniteFireballs); break;
                    case sf::Keyboard::Key::F11: {
                        // Steps the slider's useful stops rather than nudging by
                        // a delta: mid-take you want a named speed you can get
                        // back out of, not to hunt for 1.0x again by ear.
                        constexpr float kStops[] = {1.0f, 0.5f, 0.25f, 0.1f};
                        const float now = cheats.simulationTimeScale();
                        int next = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (std::abs(kStops[i] - now) < 0.001f) { next = (i + 1) % 4; break; }
                        }
                        cheats.setTimeScale(kStops[next]);
                        std::cout << "[Cheats] Time scale: " << kStops[next] << "x" << std::endl;
                        break;
                    }
                    default:
                        break;
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
                    case sf::Keyboard::Key::Num9: // Hidden Block Found
                        // Was BlockBroken, which is a brick shattering, not a secret
                        // being found; the two are separate events now.
                        bus.publish({EventType::HiddenBlockFound, 0});
                        break;
                    default:
                        break;
                }
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
        m_mapEditor.update(m_tileMap, m_entities, mouseWorldPos,
                           Game::getInstance().getMousePixelPosition(), dt, &m_camera);
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
        if (!m_replayPlaybackActive) {
            // A fresh isPlaying()==true edge — the console's "replay play" can
            // start a new playback in a PlayingState that already ran one to
            // completion, and a stale hold count left over from that earlier
            // run must not eat into this one's first frames.
            m_replayPlaybackActive = true;
            m_replayHoldTicks = 0;
        }

        // ReplayRecorder::record() keeps 1 real simulation frame in every
        // kFrameInterval (see its doc comment), so pulling a new one from
        // advance() every single update() tick plays the whole recording back
        // kFrameInterval times faster than it was captured — a ~28s
        // attract-mode demo finished in under 5 seconds the first time this
        // was actually watched rather than just built. Hold the last applied
        // snapshot for the frames in between instead.
        bool stillPlaying = true;
        if (m_replayHoldTicks > 0) {
            --m_replayHoldTicks;
        } else if (const GameSnapshot* frame = ReplayRecorder::getInstance().advance()) {
            applySnapshot(*frame);
            m_replayHoldTicks = ReplayRecorder::kFrameInterval - 1;
        } else {
            stillPlaying = false;
        }

        if (stillPlaying) {
            m_camera.update(dt);
            m_background.update(dt);
            m_tileAnimTimer += dt;
            // Same defect the map editor's early return above already names:
            // enter() always starts a 0.45s fade-in, and this branch returns
            // before the code that would otherwise tick it, which pinned a
            // full-screen black rectangle over the whole demo forever — first
            // seen on attract mode (F5), whose replay starts playing from
            // frame 1 of enter(), so the fade never gets a normal frame to
            // advance on before this early return takes over.
            ScreenTransitionManager::getInstance().update(dt);
            return;
        }

        m_replayPlaybackActive = false;
        std::cout << "[Replay] Playback finished." << std::endl;
        if (m_isAttractDemo) {
            // The demo ran out on its own (nobody pressed a key) — go back to
            // the menu rather than falling through into live physics with
            // whatever input state the idle player happened to leave behind.
            Game::getInstance().changeState(std::make_unique<MenuState>());
            return;
        }
    } else if (m_replayPlaybackActive) {
        // Playback was stopped from outside (the console's "replay stop")
        // between ticks, so the block above never got a chance to reset this
        // itself. Left alone, the next "replay play" in this same instance
        // would wrongly think it was resuming a playback already in progress
        // and skip the fresh-start reset a few lines up.
        m_replayPlaybackActive = false;
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

    // 1c. A pipe entry is in progress — the player is sliding into the mouth.
    //
    // Ahead of both the input poll and the physics pass deliberately: the slide
    // is scripted motion through a solid block, so the collision resolver would
    // push the player back out of the pipe they are entering, and the movement
    // keys would let them walk away halfway in. The presentation layer still
    // ticks, or the fade this starts would be pinned on screen — the same defect
    // the map-editor and replay early returns above each name.
    if (m_pipeEntry.active) {
        updatePipeEntry(dt);
        m_camera.update(dt);
        m_background.update(dt);
        ScreenTransitionManager::getInstance().update(dt);
        return;
    }

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
    // Debug > Cheats' FREE CAMERA borrows the same keys the editor's free camera
    // uses (WASD/arrows), so while it is on the movement keys belong to the
    // camera and the player holds still — which is the point of framing a shot
    // without walking there.
    const bool inputBelongsToGameplay =
        !DebugConsole::getInstance().isVisible() &&
        !ImGui::GetIO().WantCaptureKeyboard &&
        !Game::getInstance().debugCheats().detachesCamera();

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

    // 3. Update all active entities, except the ones nobody can observe.
    //
    // The gate is here rather than inside each entity because "am I worth
    // simulating" is a question about the CAMERA, and an entity has no business
    // knowing where the camera is. See thinkingRegion() for the margin and
    // freezableOffCamera() for what is exempt and why.
    const AABB thinking = thinkingRegion();
    m_entitiesThought = 0;
    m_entitiesFrozen  = 0;
    m_entitiesExempt  = 0;
    for (auto& entity : m_entities) {
        if (!entity || !entity->isActive()) continue;
        if (!entity->getBoundingBox().intersects(thinking)) {
            if (freezableOffCamera(*entity)) {
                ++m_entitiesFrozen;
                continue;
            }
            ++m_entitiesExempt;
        }
        ++m_entitiesThought;
        entity->update(dt);
    }

    // 3. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 3a2. Admit everything a handler asked for during the two loops above.
    //
    // This is the only point in the frame where m_entities is allowed to grow:
    // the entity update loop and the physics pass have both finished, so no
    // iterator or reference into the vector is still live.
    flushPendingSpawns();

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

    // 3b1a. Ambient zone particles (ParticleEmitter had WaterBubble and LavaEmber
    // settings defined but nothing ever called burst() with them — R7 audit).
    // Timer-gated per player so standing in a zone queues an occasional puff
    // rather than one every single frame.
    for (Player* who : {m_player, m_player2}) {
        if (!who || !who->isActive() || who->isDying()) continue;
        const AABB box = who->getBoundingBox();
        const sf::Vector2f center = box.getCenter();
        const TileType occupied = m_tileMap.getTileSurfaceType(center.x, center.y);
        if (occupied != TileType::Water && occupied != TileType::Lava) continue;

        m_ambientParticleTimer -= dt;
        if (m_ambientParticleTimer <= 0.0f) {
            m_ambientParticleTimer = 0.4f;
            m_particleEmitter.burst(center, occupied == TileType::Water
                                             ? ParticleType::WaterBubble
                                             : ParticleType::LavaEmber);
        }
        break; // one zone check is enough to keep the timer meaningful
    }

    // 3b1b. Wall-slide dust. isOnWall() while airborne is exactly the condition
    // CollisionResolver uses to cap the fall speed into a slide (see the wall
    // slide comment in resolveEntityVsTile) — the same state, just read here
    // instead of re-derived.
    const bool sliding = (m_player && m_player->isOnWall() && !m_player->isOnGround()) ||
                         (m_player2 && m_player2->isOnWall() && !m_player2->isOnGround());
    if (sliding) {
        m_wallDustTimer -= dt;
        if (m_wallDustTimer <= 0.0f) {
            m_wallDustTimer = 0.1f;
            Player* slidingPlayer = (m_player && m_player->isOnWall() && !m_player->isOnGround())
                                    ? m_player : m_player2;
            if (slidingPlayer) {
                m_particleEmitter.burst(slidingPlayer->getBoundingBox().getCenter(), ParticleType::WallDust);
            }
        }
    } else {
        m_wallDustTimer = 0.0f;
    }

    // 3b1c. Surface-dependent footsteps (SPEC 11.4). Three WAVs were loaded in
    // SoundManager::loadAllSounds() and never once played (R7 audit).
    updateFootstep(m_player, m_footstepTimer, dt);
    updateFootstep(m_player2, m_footstepTimer2, dt);

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

    // 3b3. Endless Mode: keep the road ahead of the player generated, and the
    // world label showing how far they have actually gotten.
    if (m_isEndless && m_player) {
        extendEndlessLevelIfNeeded();
        const float distanceTiles = m_player->getPosition().x / Constants::TILE_SIZE;
        if (distanceTiles > m_endlessBestDistanceTiles) {
            m_endlessBestDistanceTiles = distanceTiles;
            m_worldLabel = "ENDLESS  " + std::to_string(static_cast<int>(m_endlessBestDistanceTiles)) + "m";
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
    // Where the void begins.
    //
    // This used to sit a tile BELOW the whole tilemap. In a 23-row level that is
    // world y 768 against a 720px window, so the player was already well off the
    // bottom of the screen and several frames into an unrecoverable fall before
    // anything registered — and by then the rewind buffer's most recent frames
    // were all of the same doomed fall.
    //
    // The plane is now one tile under the LOWEST FLOOR IN THE LEVEL, which for a
    // pit that reaches the map's bottom row is the same place, but for every
    // level whose floor sits above the map's edge is meaningfully earlier. The
    // hit lands while the fall is still recent, so the seconds R can rewind into
    // are the seconds before the mistake rather than the mistake itself.
    const float bottomVoidY = m_voidPlaneY;
    const float rightVoidX  = (m_tileMap.getWidth()  * Constants::TILE_SIZE) + 64.0f;
    for (Player* who : {m_player, m_player2}) {
        if (!who || !who->isActive()) continue;
        const sf::Vector2f at = who->getPosition();
        if (at.y > bottomVoidY || at.x < -64.0f || at.x > rightVoidX) {
            // Debug > Cheats' IMMORTAL short-circuits ahead of the publish, not
            // just inside killPlayer(): PlayerDied is what StatisticsTracker
            // counts a death on, and a run nobody died in must not be reported
            // as one full of deaths.
            if (Game::getInstance().debugCheats().rescueInsteadOfKill()) {
                rescuePlayer(who, "fell into the void");
                continue;
            }
            // Nothing else publishes for a fall, so this path does.
            EventBus::getInstance().publish({EventType::PlayerDied, who});
            killPlayer(who, "fell into the void");
        }
    }
    // The loop above checks players and only players, so anything else that
    // left the level fell forever: still active, still updated, still tracked,
    // just permanently out of sight. For an ordinary enemy that is an invisible
    // leak. For a boss it is a softlock — syncBossHud() keeps drawing a
    // full-health bar for a boss who is no longer anywhere, and updateBossArena()
    // keeps the player clamped inside an arena whose fight can neither be won
    // nor left. Reported live as "BOWSER, full health, nothing on the bridge".
    //
    // What leaving means is the entity's own business (Entity::onLeftLevel):
    // ordinary entities are despawned, a boss puts itself back in its arena.
    for (auto& entity : m_entities) {
        if (!entity || !entity->isActive()) continue;
        Entity* raw = entity.get();
        if (raw == m_player || raw == m_player2) continue;   // handled above
        const sf::Vector2f at = raw->getPosition();
        if (at.y <= bottomVoidY && at.x >= -64.0f && at.x <= rightVoidX) continue;
        if (raw->onLeftLevel()) raw->destroy();
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

        sf::Vector2f warpMouth{0.0f, 0.0f};
        float warpApproachX = 0.0f;

        for (const auto& entity : m_entities) {
            if (auto pipe = dynamic_cast<Pipe*>(entity.get())) {
                if (pipe->checkWarp(*m_player)) {
                    warpTarget    = pipe->getTargetLevel();
                    warpExit      = pipe->getExitPosition();
                    warpMouth     = pipe->getMouthCenter();
                    warpApproachX = pipe->getSideApproachDirection();
                    warping       = true;
                    break;
                }
            }
        }

        if (warping && (!warpTarget.empty() || warpExit.x != 0.0f || warpExit.y != 0.0f)) {
            // Long enough to cover the slide as well as the warp itself, so the
            // exit cannot immediately re-trigger the pipe it landed next to.
            m_warpCooldown = 1.5f;
            SoundManager::getInstance().playSound("pipe");
            EventBus::getInstance().publish({EventType::PlayerWarped, 0});
            beginPipeEntry(warpMouth, warpApproachX, warpTarget, warpExit);
            return;   // the slide owns the next few frames
        }
    }


    // 3e. Level timer. It was set once, shown in the HUD and snapshotted for
    // rewind, but never actually decremented — so there was no time pressure and
    // no time-out death, and TimeWarning was never published despite having a
    // subscriber (audit G-2).
    // Debug > Cheats' FREEZE TIMER holds the countdown where it is, so a take can
    // run past the level's own clock without the countdown creeping into
    // the shot or timing the run out mid-sentence.

    // How long this run has actually been going, whatever the countdown is
    // doing. Kept separately because two things stop the countdown without
    // stopping the run — Endless Mode below and the FREEZE TIMER cheat above —
    // and updateShadow() reads "LEVEL_TIME minus the countdown" as its clock,
    // so a held countdown froze Shadow Mario's replay timeline along with it.
    if (!m_levelComplete) m_runElapsed += dt;

    // Endless Mode has no countdown at all. It is an overworld survival run
    // scored on distance — there is no flagpole to reach and no time bonus to
    // earn — and nothing gated the countdown on it, so an endless mode ENDED,
    // every time, after Constants::LEVEL_TIME (300) seconds, by killing the
    // player for running out of time on a level that has no end.
    if (!m_levelComplete && !m_isEndless && m_levelTimer > 0.0f &&
        !Game::getInstance().debugCheats().holdsLevelTimer()) {
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

    // 3f. Level complete: walk to the castle door, then show the summary.
    // The victory screen is an overlay, so this level stays on screen behind it
    // and the player is seen arriving at the castle they just cleared, rather
    // than standing frozen at the flagpole.
    if (m_levelComplete && !m_summaryShown) {
        m_levelCompleteTimer += dt;
        if (m_player && m_hasLevelCompleteCastle) {
            const float dx = m_levelCompleteCastleTarget.x - m_player->getPosition().x;
            m_player->clearMovementRequests();
            if (dx > 4.0f)       m_player->moveRight();
            else if (dx < -4.0f) m_player->moveLeft();
        }
        if (m_levelCompleteTimer >= 3.0f) {
            presentLevelSummary();
            return;
        }
    }

    // 3g. Boss arena. Runs before the camera follows, so the lock is in place
    // for this frame rather than one frame late.
    updateBossArena();

    // 4. Update Camera & Screen Transitions
    //
    // Debug > Cheats' FREE CAMERA takes the camera off the player entirely so a
    // shot can be framed without walking there. It has to pre-empt every follow
    // path below, versus included, or the follow would drag the frame straight
    // back to the player on the same tick the pan moved it away.
    if (Game::getInstance().debugCheats().detachesCamera()) {
        updateFreeCamera(dt);
    } else if (bothPlayersPresent()) {
        // bothPlayersPresent(), not m_player2: eliminating Player 1 leaves
        // m_player2 non-null, this branch was taken anyway, and
        // updateVersusCamera() returns at its first line without both players —
        // so the camera stopped moving for the rest of the match and the winner
        // ran out of frame. See activeParticipant()'s comment.
        updateVersusCamera(dt);
    } else if (Player* solo = activeParticipant()) {
        // Velocity drives the lookahead (task 4.3): the camera leads the player
        // in the direction they are running.
        m_camera.follow(solo->getPosition(), solo->getVelocity(), dt);
    }
    m_camera.update(dt);

    // 4a. Keep a lone player inside the camera's view. updateVersusCamera()
    // above already tethers both players to the view when there are two
    // (that "hard bounds" loop at its end) — a single player had no equivalent,
    // so outrunning the camera's lag/lookahead simply carried them past the
    // edge of what was on screen with nothing to stop them (only a boss arena,
    // via updateBossArena(), clamped position at all before this — R7 audit).
    // X only, and the bottom edge is deliberately left unclamped: the void-kill
    // plane (3c, above) is a world-space plane well below the level, and a
    // falling player must keep dropping below the visible view until that
    // check catches them rather than being arrested at the screen's bottom.
    // Exempt under FREE CAMERA (the player is standing still somewhere the frame
    // has deliberately been moved away from, and shoving them back into it would
    // undo the shot) and under NOCLIP (a ghost that cannot leave the visible
    // window is not a ghost).
    //
    // Written against activeParticipant() rather than m_player: the survivor of
    // a two-player match is whichever of the two is left, and gating on
    // `m_player && !m_player2` left an eliminated Player 1's survivor with
    // neither this tether nor the versus one.
    const DebugCheats& cheatsForClamp = Game::getInstance().debugCheats();
    Player* lone = bothPlayersPresent() ? nullptr : activeParticipant();
    if (lone && !m_arenaLocked && !lone->isDying() &&
        !cheatsForClamp.detachesCamera() && !cheatsForClamp.passesThroughSolids()) {
        const AABB view = m_camera.getVisibleBounds();
        const AABB box = lone->getBoundingBox();
        sf::Vector2f position = lone->getPosition();
        bool moved = false;
        if (position.x < view.x) {
            position.x = view.x;
            moved = true;
        } else if (position.x + box.width > view.x + view.width) {
            position.x = view.x + view.width - box.width;
            moved = true;
        }
        if (moved) {
            lone->setPosition(position);
            lone->setVelocity({0.0f, lone->getVelocity().y});
        }
    }

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
    updatePSwitch(dt);

    if (m_hud) {
        m_hud->update(dt);
        HudData hudData;
        hudData.timeLeft = static_cast<int>(m_levelTimer);
        // A frozen "300" in the TIME field would read as a broken clock. Say
        // there is no clock instead.
        hudData.timeUnlimited = m_isEndless;
        // The HUD has always drawn these two; nothing ever set them, so the
        // countdown never appeared however long the switch ran.
        hudData.pSwitchActive = m_pSwitchActive;
        hudData.pSwitchTimer  = m_pSwitchTimer;
        hudData.worldLabel    = m_worldLabel;

        if (m_player2 && m_player2->isActive()) {
            hudData.hasSecondPlayer      = true;
            hudData.secondCharacterName  = m_player2->getCharacterName();
            hudData.secondLives          = m_player2->getLives();
            hudData.secondCoins          = m_player2->getCoins();
            // Deliberately short. The badge sits between Player 1's and the
            // WORLD field, and "CPU SPEEDRUNNER" is wide enough to draw straight
            // over the world label. Which archetype is playing is already named
            // in full on the match line at the bottom of the screen, which has a
            // whole row to itself.
            hudData.secondPlayerLabel = m_aiController ? std::string("CPU")
                                                       : std::string("P2");
        }
        
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
            hudData.characterName = "mario";
            hudData.starCoinsCollected = {true, true, false}; // 2 out of 3 collected
        }
        // Last, so it can overwrite what the two blocks above left for a player
        // who is no longer in the level at all.
        applyEliminatedBadges(hudData);
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

void PlayingState::beginPipeEntry(sf::Vector2f mouthCenter, float approachX,
                                  const std::string& targetLevel, sf::Vector2f exit) {
    if (!m_player) return;

    const AABB box = m_player->getBoundingBox();
    m_pipeEntry = PipeEntry{};
    m_pipeEntry.active      = true;
    m_pipeEntry.duration    = 0.5f;
    m_pipeEntry.from        = m_player->getPosition();
    m_pipeEntry.targetLevel = targetLevel;
    m_pipeEntry.exit        = exit;

    if (approachX == 0.0f) {
        // Top entry: sink straight down, and line up with the mouth on the way
        // so a player standing half off the rim does not slide down its edge.
        // Far enough that the whole sprite is inside the pipe's own art by the
        // end — render() draws them behind it for exactly these frames.
        m_pipeEntry.to = {mouthCenter.x - box.width * 0.5f,
                          m_pipeEntry.from.y + box.height + 8.0f};
    } else {
        // Side entry: walk on into the mouth. A pipe is solid, so the player is
        // already flush against its face and this carries them through it.
        m_pipeEntry.to = {m_pipeEntry.from.x + approachX * (box.width + 24.0f),
                          m_pipeEntry.from.y};
    }

    // The existing screen transition, not a second one: by the time the slide
    // ends the screen is dark, so the level swap underneath it is not seen.
    // A circle iris centred on the pipe would suit it better, but the focal
    // point is in screen space and nothing exposes the world-to-screen mapping,
    // so a mis-centred iris would be worse than a clean fade.
    ScreenTransitionManager::getInstance().fadeOut(m_pipeEntry.duration);
}

void PlayingState::updatePipeEntry(float dt) {
    m_pipeEntry.elapsed += dt;
    const float t = (m_pipeEntry.duration <= 0.0f)
                        ? 1.0f
                        : std::min(1.0f, m_pipeEntry.elapsed / m_pipeEntry.duration);

    if (m_player) {
        // Requests are cleared every frame, not once at the start: they are
        // rebuilt from held keys by the input poll this early return skips, but
        // whatever was held on the frame the warp fired is still in there.
        m_player->clearMovementRequests();
        m_player->setVelocity({0.0f, 0.0f});
        m_player->setPosition(m_pipeEntry.from +
                              (m_pipeEntry.to - m_pipeEntry.from) * t);
    }

    if (t < 1.0f) return;

    // Copied out and the slide cleared BEFORE the warp runs. loadLevelByPath()
    // replaces m_entities and rebuilds the player, so reading m_pipeEntry after
    // it would be reading a slide whose start position belongs to a level that
    // no longer exists — and render() keys its draw order off `active`.
    const std::string target = m_pipeEntry.targetLevel;
    const sf::Vector2f exit  = m_pipeEntry.exit;
    m_pipeEntry = PipeEntry{};

    if (!target.empty()) {
        // Starts its own fade-in, which replaces the fade-out above.
        loadLevelByPath(target, exit);
    } else if (m_player) {
        m_player->setPosition(exit);
        m_player->setVelocity({0.0f, 0.0f});
        // Same-level teleport: nothing else reveals the screen again.
        ScreenTransitionManager::getInstance().fadeIn(0.3f);
    }
}

void PlayingState::beginPSwitch(float seconds) {
    // Pressing it again while it runs restarts the clock rather than swapping a
    // second time — the second swap would turn the coins back into bricks and
    // the effect would read as "the switch cancelled itself".
    if (m_pSwitchActive) {
        m_pSwitchTimer = seconds;
        return;
    }

    m_pSwitchActive = true;
    m_pSwitchTimer = seconds;
    m_pSwitchSwaps.clear();

    // Brick becomes coin, coin becomes brick — the whole level, not just what is
    // on screen, so walking into a room mid-effect finds it already swapped.
    for (int y = 0; y < m_tileMap.getHeight(); ++y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            const TileType current = m_tileMap.getTileType(x, y);
            TileType swapped = TileType::Empty;
            if (current == TileType::Brick)     swapped = TileType::Coin;
            else if (current == TileType::Coin) swapped = TileType::Brick;
            else continue;

            m_pSwitchSwaps.push_back({x, y, current});
            m_tileMap.setTile(x, y, swapped);
        }
    }

    std::cout << "[PlayingState] P-Switch: swapped " << m_pSwitchSwaps.size()
              << " tile(s) for " << seconds << "s." << std::endl;
}

void PlayingState::updatePSwitch(float dt) {
    if (!m_pSwitchActive) return;
    m_pSwitchTimer -= dt;
    if (m_pSwitchTimer <= 0.0f) endPSwitch();
}

void PlayingState::endPSwitch() {
    if (!m_pSwitchActive) return;

    // Restored from the record, not by swapping back: a coin the player picked
    // up during the effect left an Empty tile behind, and a blind reverse-swap
    // would turn that hole into a brick out of thin air.
    for (const SwappedTile& swap : m_pSwitchSwaps) {
        m_tileMap.setTile(swap.x, swap.y, swap.original);
    }
    m_pSwitchSwaps.clear();
    m_pSwitchActive = false;
    m_pSwitchTimer = 0.0f;
    std::cout << "[PlayingState] P-Switch expired." << std::endl;
}

void PlayingState::syncVoidPlane() {
    // The deepest solid tile anywhere in the level: below that there is nothing
    // to land on however far you fall, so it is the honest place to stop
    // simulating the fall. One tile of slack under it so a player skimming the
    // last ledge is not killed by the geometry they just cleared.
    int deepestFloorRow = -1;
    for (int y = m_tileMap.getHeight() - 1; y >= 0 && deepestFloorRow < 0; --y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            if (TileMap::getInfo(m_tileMap.getTileType(x, y)).isSolid) {
                deepestFloorRow = y;
                break;
            }
        }
    }

    const float mapBottom = m_tileMap.getHeight() * Constants::TILE_SIZE;
    if (deepestFloorRow < 0) {
        m_voidPlaneY = mapBottom + Constants::TILE_SIZE;
        return;
    }

    // Never above the bottom of the deepest floor — a player standing on the
    // lowest tile in the level must not be inside the void.
    const float belowDeepestFloor =
        static_cast<float>(deepestFloorRow + 2) * Constants::TILE_SIZE;
    m_voidPlaneY = std::min(belowDeepestFloor, mapBottom + Constants::TILE_SIZE);
}

void PlayingState::refreshWorldLabel(const std::string& levelPath) {
    if (m_isProcedural) {
        m_worldLabel = "RANDOM";
        return;
    }
    // An authored level has no world number and never will; falling through to
    // the catalogue match below would print "WORLD ?" over somebody's own level.
    if (!m_customLevelPath.empty()) {
        m_worldLabel = "CUSTOM";
        return;
    }

    // Match the loaded path against the campaign catalogue, so the label comes
    // from the same table the level select reads and cannot drift from it.
    std::string display;
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        const std::string& catalogPath = LevelCatalog::pathFor(i);
        // Suffix match: the loader tries several roots ("assets/levels/...",
        // "../assets/levels/...") so the path it succeeded with is not always
        // the catalogue's spelling of it.
        if (levelPath.size() >= catalogPath.size() &&
            levelPath.compare(levelPath.size() - catalogPath.size(),
                              catalogPath.size(), catalogPath) == 0) {
            display = LevelCatalog::nameFor(i);
            break;
        }
    }

    // A sub-level is not in the catalogue — it is reached through a pipe, not
    // the level select — so name it after the level it belongs to.
    const bool isSub = levelPath.find("_sub") != std::string::npos;
    if (display.empty()) {
        for (int i = 0; i < LevelCatalog::count(); ++i) {
            std::string stem = LevelCatalog::pathFor(i);
            const std::size_t dot = stem.rfind(".json");
            if (dot != std::string::npos) stem = stem.substr(0, dot);
            const std::size_t slash = stem.rfind('/');
            if (slash != std::string::npos) stem = stem.substr(slash + 1);
            if (levelPath.find(stem + "_sub") != std::string::npos) {
                display = LevelCatalog::nameFor(i);
                break;
            }
        }
    }

    if (display.empty()) {
        m_worldLabel = "WORLD ?";
        return;
    }

    // "Bonus 1" is not a world number and must not be printed as one.
    std::string label = (display.rfind("Bonus", 0) == 0) ? display : "WORLD " + display;
    if (isSub) label += " SUB";
    std::transform(label.begin(), label.end(), label.begin(), ::toupper);
    m_worldLabel = label;
}

float PlayingState::floorBelow(float worldX, float fromWorldY) const {
    const int gx = static_cast<int>(worldX / Constants::TILE_SIZE);
    if (gx < 0 || gx >= m_tileMap.getWidth()) return -1.0f;

    const int firstRow = std::max(0, static_cast<int>(fromWorldY / Constants::TILE_SIZE));
    for (int y = firstRow; y < m_tileMap.getHeight(); ++y) {
        if (TileMap::getInfo(m_tileMap.getTileType(gx, y)).isSolid) {
            return static_cast<float>(y) * Constants::TILE_SIZE;
        }
    }
    return -1.0f;
}

namespace {
// A small player is 32x32 (Mario's constructor). Deliberately the SMALLEST
// form: a spawn that a small player fits is the weakest claim worth making, and
// the level restores the saved power-up form after the spawn anyway.
constexpr float SPAWN_BOX = 32.0f;
}

bool PlayingState::isSpawnUsable(sf::Vector2f topLeft) const {
    const float mapRight  = m_tileMap.getWidth()  * Constants::TILE_SIZE;
    const float mapBottom = m_tileMap.getHeight() * Constants::TILE_SIZE;
    if (topLeft.x < 0.0f || topLeft.x + SPAWN_BOX > mapRight ||
        topLeft.y < 0.0f || topLeft.y + SPAWN_BOX > mapBottom) {
        return false;
    }

    // The spawn's OWN box, not just the column beneath it.
    //
    // The old guard asked only "is anything solid below?" and began that scan at
    // the spawn point itself, so a spawn buried in a solid tile hit that tile on
    // its first sample and was reported standable. All three sub-levels shipped
    // a spawn point inside their own entrance pipe on the strength of that
    // answer, masked only because every route in overrode it (R21-D2).
    for (float dy = 4.0f; dy < SPAWN_BOX; dy += 12.0f) {
        for (float dx = 4.0f; dx < SPAWN_BOX; dx += 12.0f) {
            if (TileMap::getInfo(m_tileMap.getTileAt(topLeft.x + dx, topLeft.y + dy)).isSolid) {
                return false;
            }
        }
    }

    // Starting below the box, for the same reason.
    return floorBelow(topLeft.x + SPAWN_BOX * 0.5f, topLeft.y + SPAWN_BOX) >= 0.0f;
}

sf::Vector2f PlayingState::usableSpawnNear(sf::Vector2f desired, const std::string& levelPath) const {
    if (isSpawnUsable(desired)) return desired;

    // Sideways along the same row rather than downwards: a level's spawn row is
    // chosen to be above its floor, and stepping out of whatever the author
    // buried it in is a smaller lie than dropping it into the room below.
    const float mapRight = m_tileMap.getWidth() * Constants::TILE_SIZE;
    for (float step = Constants::TILE_SIZE; step < mapRight; step += Constants::TILE_SIZE) {
        for (const float candidateX : {desired.x + step, desired.x - step}) {
            const sf::Vector2f candidate{candidateX, desired.y};
            if (isSpawnUsable(candidate)) {
                std::cerr << "[PlayingState] Spawn (" << desired.x << ", " << desired.y
                          << ") in " << levelPath
                          << " is inside solid geometry or has no floor; moved to ("
                          << candidate.x << ", " << candidate.y << ")." << std::endl;
                return candidate;
            }
        }
    }

    std::cerr << "[PlayingState] Spawn (" << desired.x << ", " << desired.y
              << ") in " << levelPath
              << " is unusable and no position on its row is any better; using it anyway."
              << std::endl;
    return desired;
}

void PlayingState::settleEndOfLevelScenery() {
    for (const auto& entity : m_entities) {
        if (!entity) continue;

        if (auto* flagpole = dynamic_cast<Flagpole*>(entity.get())) {
            const sf::Vector2f at = flagpole->getPosition();
            // Probed from the middle of the pole, not its left edge: the sprite
            // is 24px wide and its left edge can sit over the last empty column
            // before the floor starts.
            const float floorTop = floorBelow(at.x + 12.0f, at.y);
            if (floorTop < 0.0f) continue;   // nothing under it; leave it alone

            // The pole's foot goes on the floor, so its top-left corner is one
            // pole-height above that.
            const sf::Vector2f settled{at.x, floorTop - flagpole->getPoleHeight()};
            flagpole->setPosition(settled);
            std::cout << "[PlayingState] Flagpole settled onto the floor: y "
                      << at.y << " -> " << settled.y << std::endl;
            continue;
        }

        if (auto* castle = dynamic_cast<Castle*>(entity.get())) {
            // Styled before it is settled, because setFrame() can change the
            // box width the floor probe below is centred on.
            const BackgroundTheme theme = m_background.getTheme();
            castle->setFrame((theme == BackgroundTheme::Ice || theme == BackgroundTheme::Castle)
                                 ? Castle::STONE_FRAME
                                 : Castle::DEFAULT_FRAME);

            const sf::Vector2f at = castle->getPosition();
            const float floorTop = floorBelow(at.x + castle->getBoundingBox().width * 0.5f, at.y);
            if (floorTop < 0.0f) continue;
            castle->setPosition({at.x, floorTop - Castle::HEIGHT_TILES * Constants::TILE_SIZE});
        }
    }
}

void PlayingState::chopBridge() {
    // What counts as "the bridge": inside the boss's arena, every solid tile in
    // a column that has lava underneath it. Derived rather than declared, so a
    // level does not have to list its bridge tiles and the map editor cannot
    // produce a bridge the axe does not know about. Columns with no lava — the
    // ledges either side — keep their floor, which is what the player lands on.
    const AABB span = (m_activeBoss && m_activeBoss->hasArena())
        ? m_activeBoss->getArena()
        : AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE,
                           m_tileMap.getHeight() * Constants::TILE_SIZE};

    const int firstX = std::max(0, static_cast<int>(span.x / Constants::TILE_SIZE));
    const int lastX  = std::min(m_tileMap.getWidth() - 1,
                                static_cast<int>((span.x + span.width) / Constants::TILE_SIZE));

    int dropped = 0;
    for (int x = firstX; x <= lastX; ++x) {
        int topLavaRow = -1;
        for (int y = 0; y < m_tileMap.getHeight(); ++y) {
            if (m_tileMap.getTileType(x, y) == TileType::Lava) { topLavaRow = y; break; }
        }
        if (topLavaRow < 0) continue;   // no lava under this column: not bridge

        for (int y = 0; y < topLavaRow; ++y) {
            if (!TileMap::getInfo(m_tileMap.getTileType(x, y)).isSolid) continue;
            m_tileMap.setTile(x, y, TileType::Empty);
            ++dropped;
        }
    }

    // The floor is gone; so is whoever was standing on it. Routed through the
    // boss's own defeat so the score, the event and the animation are the same
    // ones a fifth stomp would have produced.
    //
    // Ordering is load-bearing: the tiles are cleared and the defeat is taken
    // in the SAME call, before the next physics tick, so the boss is never
    // simulated standing on a floor that no longer exists. Boss.hpp's
    // returnToArenaSpawn() comment records the consequence of the alternative —
    // losing that floor by any route "other than the axe (chopBridge(), which
    // calls defeatNow() first)" dropped Bowser clean out of the world. Anything
    // that defers the defeat past this function has to answer that case.
    //
    // R21: the chop no longer deletes him. beginLavaDeath() drops him off the
    // stump under ordinary gravity, and once he is in the lava it burns his
    // health down one point at a time before routing through the very same
    // defeatNow() — so the score, the event and the animation are unchanged and
    // the player watches him destroyed instead of vanishing on the axe swing.
    //
    // The ordering note above still holds, and beginLavaDeath() answers it: the
    // boss stops being physics-driven the moment he reaches the lava, so he is
    // never carried through the two lava tiles and past the void plane, and
    // Boss::onLeftLevel() refuses while isDyingInLava() so the wandered-off
    // guard cannot resurrect him at full health mid-fall.
    //
    // releaseBossArena() is deliberately NOT called on this path. syncBossHud()
    // reports bossActive = false whenever the arena is unlocked, so releasing
    // here would take the health bar off screen at the exact moment it starts
    // draining — hiding the thing this change exists to show. updateBossArena()
    // already releases the instant the boss is actually defeated, so nothing is
    // lost by waiting. The else branch keeps the old behaviour for a chop with
    // no live boss, which is the map editor's case.
    if (m_activeBoss && !m_activeBoss->isDefeated()) {
        m_activeBoss->beginLavaDeath();
    } else {
        releaseBossArena();
    }

    m_camera.triggerScreenShake(12.0f, 0.6f);
    SoundManager::getInstance().playSound("bowserfall");
    std::cout << "[PlayingState] Bridge chopped: " << dropped
              << " tile(s) dropped into the lava." << std::endl;
}

void PlayingState::detonatePOW() {
    // The POW block's whole point: everything standing on a surface anywhere on
    // screen is knocked over at once. Off-screen enemies are spared, so it is a
    // tool for the fight in front of you rather than a level-wide delete.
    const AABB view = m_camera.getVisibleBounds();
    int flipped = 0;

    for (const auto& entity : m_entities) {
        auto* enemy = dynamic_cast<Enemy*>(entity.get());
        if (!enemy || !enemy->isActive() || enemy->isDeadOrDying()) continue;

        const AABB box = enemy->getBoundingBox();
        const bool onScreen = box.x + box.width  > view.x &&
                              box.x              < view.x + view.width &&
                              box.y + box.height > view.y &&
                              box.y              < view.y + view.height;
        if (!onScreen) continue;

        // A boss is not "an enemy standing on the floor" — it has a health bar
        // and a fight, and one POW would end it. It takes a hit like any other
        // heavy blow instead.
        if (auto* boss = dynamic_cast<Boss*>(enemy)) {
            boss->onStomped();
            ++flipped;
            continue;
        }

        // Airborne enemies are untouched: the shockwave travels through the
        // ground, which is why a POW cannot clear a Paratroopa mid-hop.
        if (!enemy->isOnGround()) continue;

        enemy->onStomped();
        ++flipped;
    }

    if (m_player) m_player->addScore(flipped * 200);
    std::cout << "[PlayingState] POW block: flipped " << flipped << " enemy(ies)." << std::endl;
}

void PlayingState::updateFreeCamera(float dt) {
    // Bounds off while detached, so the frame can sit past the edge of the level
    // — otherwise the pan stops dead at the last column and half the point of a
    // free camera (framing scenery, the flagpole, the sky above a level) is
    // lost. update() re-asserts the invariant the moment the cheat goes off, the
    // same way it already does after the map editor unclamps it (audit C-3).
    if (m_camera.isBoundsEnabled()) m_camera.setBoundsEnabled(false);

    // Deliberately the same keys and the same 650 px/s as
    // MapEditor::handlePanning(): the editor's free camera is the one this
    // project already has, and a second set of pan keys to remember would be a
    // worse tool, not a better one.
    constexpr float PAN_SPEED = 650.0f;
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    InputManager& input = InputManager::getInstance();
    sf::Vector2f pan{0.0f, 0.0f};
    if (input.isHeld(sf::Keyboard::Key::A) || input.isHeld(sf::Keyboard::Key::Left))  pan.x -= 1.0f;
    if (input.isHeld(sf::Keyboard::Key::D) || input.isHeld(sf::Keyboard::Key::Right)) pan.x += 1.0f;
    if (input.isHeld(sf::Keyboard::Key::W) || input.isHeld(sf::Keyboard::Key::Up))    pan.y -= 1.0f;
    if (input.isHeld(sf::Keyboard::Key::S) || input.isHeld(sf::Keyboard::Key::Down))  pan.y += 1.0f;
    if (pan.x != 0.0f || pan.y != 0.0f) {
        m_camera.move(pan * PAN_SPEED * dt);
    }
}

void PlayingState::clearOnScreenEnemies() {
    // Same on-screen test and the same "a boss is not a Goomba" carve-out as
    // detonatePOW(), for the same reason: one button must not silently end a
    // fight that has a health bar and a win condition attached to it.
    const AABB view = m_camera.getVisibleBounds();
    int cleared = 0;

    for (const auto& entity : m_entities) {
        auto* enemy = dynamic_cast<Enemy*>(entity.get());
        if (!enemy || !enemy->isActive() || enemy->isDeadOrDying()) continue;
        if (dynamic_cast<Boss*>(enemy)) continue;

        const AABB box = enemy->getBoundingBox();
        const bool onScreen = box.x + box.width  > view.x &&
                              box.x              < view.x + view.width &&
                              box.y + box.height > view.y &&
                              box.y              < view.y + view.height;
        if (!onScreen) continue;

        // onStomped() rather than destroy(): the defeat animation, the sound and
        // the shell-vs-Goomba difference are what a demo is being recorded to
        // show. Unlike detonatePOW() this awards no score — a cleared frame is
        // staging, not play.
        enemy->onStomped();
        ++cleared;
    }
    std::cout << "[Cheats] Cleared " << cleared << " on-screen enemy(ies)." << std::endl;
}

void PlayingState::syncBackdropGround() {
    // Find the top of the ground the player actually walks on, and hand it to the
    // backdrop so the hills and bushes stand on it.
    //
    // Scanned rather than assumed: the campaign levels put their surface on tile
    // row 21 of 23, but a generated level or a sub-level can be any height, and a
    // hardcoded screen line was wrong for all of them.
    //
    // "Lowest row holding any solid tile" was the wrong scan. In every shipped
    // level the floor is a two-row slab (rows 21 and 22), so that answer was row
    // 22 and the hills, bushes and fences stood on the *bottom* of the slab —
    // one full tile buried in the ground. What the backdrop wants is the row
    // most of the ground is on, and then the TOP of that slab.
    //
    // So: count solid tiles per row, take the widest row IN THE LOWER HALF of the
    // map as the floor, and walk upwards while the rows above are still part of
    // the same slab (at least 60% as wide). The top of the last such row is
    // where a decoration's feet belong.
    const int height = m_tileMap.getHeight();
    const int width  = m_tileMap.getWidth();
    if (height <= 0 || width <= 0) {
        m_background.setWorldGroundY(0.0f);
        return;
    }

    std::vector<int> solidPerRow(static_cast<std::size_t>(height), 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (TileMap::getInfo(m_tileMap.getTileType(x, y)).isSolid) {
                ++solidPerRow[static_cast<std::size_t>(y)];
            }
        }
    }

    // Only the lower half of the map is a candidate, and this is not a heuristic
    // for tidiness — it is what stops a CEILING from winning.
    //
    // Level 1-3 is a castle: rows 0 and 1 are a solid slab running the full 200
    // tiles, wider than its 170-tile floor. "The widest solid row" therefore
    // answered row 0, the backdrop's ground line came out as world y 0, and the
    // renderer silently fell back to its old hardcoded constant. A floor a player
    // walks on is always in the lower half of a level; a ceiling never is.
    const int firstCandidateRow = height / 2;

    int floorRow = -1;
    int widest = 0;
    // Ties go to the LOWER row: a two-row slab of equal width is one floor, and
    // its lower row is the one whose top we then climb to.
    for (int y = firstCandidateRow; y < height; ++y) {
        if (solidPerRow[static_cast<std::size_t>(y)] >= widest &&
            solidPerRow[static_cast<std::size_t>(y)] > 0) {
            widest = solidPerRow[static_cast<std::size_t>(y)];
            floorRow = y;
        }
    }
    if (floorRow < 0) {
        m_background.setWorldGroundY(0.0f);   // no floor at all: fall back
        return;
    }

    const int slabThreshold = std::max(1, (widest * 3) / 5);
    int surfaceRow = floorRow;
    while (surfaceRow > firstCandidateRow &&
           solidPerRow[static_cast<std::size_t>(surfaceRow - 1)] >= slabThreshold) {
        --surfaceRow;
    }

    m_background.setWorldGroundY(static_cast<float>(surfaceRow) * Constants::TILE_SIZE);
}

void PlayingState::render(sf::RenderTarget& target) {
    // Parallax backdrop first, in screen space: each layer is offset by a
    // fraction of the camera position, so it has to be drawn before the world
    // view is applied (task 5.5).
    target.setView(target.getDefaultView());
    m_background.render(target, m_camera.getVisibleBounds());

    // Set view to camera view for scrolling world space rendering
    target.setView(m_camera.getView());

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

    // 1. Draw the tilemap tiles. The two hundred lines of TileType-to-atlas-frame
    // switch that used to live here are TileMapRenderer's now, because the level
    // editor draws the same world and a second copy of them would drift.
    m_tileMapRenderer.setSpriteSheet(m_scenerySheet.get());
    m_tileMapRenderer.setTheme(m_background.getTheme());
    m_tileMapRenderer.render(target, m_tileMap, view, m_tileAnimTimer);

    // 2. Draw all active entities. Players are drawn last so a large entity
    // they are walking toward (Bowser, Boom Boom) can never paint over them —
    // adoptPlayer() inserts the player at the FRONT of m_entities, which used
    // to mean the opposite: the player was drawn (and hidden) first.
    //
    // Going down a pipe is the one case that wants the opposite: the slide
    // carries the player into the pipe's own art, and drawn on top of it they
    // would sink through the front of the pipe in full view rather than
    // disappearing into it. For those frames only, they go first.
    const bool playerBehindEntities = m_pipeEntry.active;
    if (playerBehindEntities) {
        if (m_player  && m_player->isActive())  m_player->render(target);
        if (m_player2 && m_player2->isActive()) m_player2->render(target);
    }
    for (auto& entity : m_entities) {
        if (entity && entity->isActive() &&
            entity.get() != static_cast<Entity*>(m_player) &&
            entity.get() != static_cast<Entity*>(m_player2)) {
            entity->render(target);
        }
    }
    if (!playerBehindEntities) {
        if (m_player  && m_player->isActive())  m_player->render(target);
        if (m_player2 && m_player2->isActive()) m_player2->render(target);
    }

    // 3. Draw entity death effects (flying trajectories) and impact particles.
    // Both are world-space, so they must be drawn while the camera view is still active.
    EntityDeathEffect::getInstance().render(target);
    target.draw(ParticleSystem::getInstance());

    // 3b. Bonus D — the dynamic light pass.
    //
    // This seam is the whole reason the effect works: everything drawn so far is
    // the world, and everything after it is either a developer overlay or the
    // screen-space HUD. Darkening here dims the cave without dimming the score
    // bar, the minimap or the pause menu, which is what a darkness overlay drawn
    // last would have done. It sits ahead of the AABB overlay deliberately too —
    // a debug outline you cannot see is not a debug tool.
    //
    // Outside the HUD block on purpose: Debug > Cheats' HIDE HUD takes the
    // interface away for clean capture, and the lighting is world, not
    // interface. Hiding the HUD to film a cave must not switch the cave's
    // lighting off with it.
    renderLightPass(target);

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

    // Draw the screen-space HUD overlay.
    //
    // Debug > Cheats' HIDE HUD takes the whole screen-space layer away — score
    // bar, minimap, match line, the rewind vignette — rather than only the score
    // bar, because "clean capture for b-roll" means nothing of the interface is
    // in the frame.
    if (m_hud && !Game::getInstance().debugCheats().hidesHud()) {
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
    // R21: and suppressed entirely unless the player asked for debug mode in
    // Options. This panel was never gated at all -- it drew over ordinary play
    // in a release build, which is also how the duplicate achievement toast
    // shipped: the newer SFML toast was added because the ImGui one supposedly
    // "only appeared while the dev overlay was up", and the overlay was always
    // up. The map editor is deliberately NOT behind this flag; it is a shipped
    // feature reachable from the main menu.
    if (!m_suspended && Game::getInstance().getDebugMode()) {
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
    // with a matching hardcoded level count in advanceToNextLevel(). A custom
    // level is not in that order and is addressed by path instead; it is the
    // only way an authored level can be played at all.
    std::string levelPath = m_customLevelPath.empty()
                                ? LevelCatalog::pathFor(m_selectedLevelIndex)
                                : m_customLevelPath;


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
        // Guarded for the same reason the warp path is: this is the flagpole ->
        // next level route and the level-select route, and a level file's own
        // spawnPoint is exactly as trustworthy as a pipe's exit.
        const sf::Vector2f spawnPos = usableSpawnNear(levelData.spawnPoint, chosenPath);
        m_levelSpawnPoint = spawnPos;
        m_hasCheckpoint = false;   // a new level invalidates the old checkpoint
        spawnSelectedPlayer(spawnPos);

        // Transfer all loaded items/entities and wire their animations
        for (auto& entity : levelData.entities) {
            admitEntity(entity.get());
            m_entities.push_back(std::move(entity));
        }

        // Set camera bounds matching the level size
        m_camera.setBounds(AABB{0.0f, 0.0f, levelData.width * Constants::TILE_SIZE, levelData.height * Constants::TILE_SIZE});
        // A boss arena from the level just left behind can leave the camera
        // Locked with a stale position; every new level starts Free and
        // centred on its own spawn point, never carrying either over from
        // whatever the previous level last did with the camera.
        m_camera.setScrollMode(Camera::ScrollMode::Free);
        m_camera.snapTo(spawnPos);
        m_background.setTheme(levelData.theme);
        syncBackdropGround();
        syncVoidPlane();
        settleEndOfLevelScenery();
        refreshWorldLabel(m_isProcedural ? std::string() : chosenPath);
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
        m_camera.setScrollMode(Camera::ScrollMode::Free);
        m_camera.snapTo(m_levelSpawnPoint);
    }

    spawnMatchParticipants();

    findActiveBoss();

    if (m_isAttractDemo) {
        // F5 attract mode (SPEC 10.2): play the bundled demo instead of
        // recording this run. Bundled under assets/ rather than saves/replays/
        // because assets/ ships with the build and saves/ is gitignored
        // per-player data that would not exist on a fresh checkout (see
        // ReplayRecorder::loadFromFile's doc comment). Tried from a few
        // relative roots for the same reason setupTestScene's own level-path
        // lookup above does: ctest, a build/ launch and a repo-root launch all
        // have different working directories.
        static const std::vector<std::string> kDemoCandidates = {
            "assets/replays/attract_demo.json",
            "SuperMarioGame/assets/replays/attract_demo.json",
            "../assets/replays/attract_demo.json",
            "../SuperMarioGame/assets/replays/attract_demo.json"
        };
        ReplayRecorder& replay = ReplayRecorder::getInstance();
        replay.clear();
        bool started = false;
        for (const auto& candidate : kDemoCandidates) {
            if (replay.loadFromFile(candidate)) {
                started = replay.startPlayback();
                break;
            }
        }
        if (!started) {
            // No demo to show is not a state worth sitting in — go straight
            // back rather than leaving an attract "demo" that plays nothing.
            std::cerr << "[PlayingState] Attract demo replay missing or empty; "
                         "returning to menu." << std::endl;
            Game::getInstance().changeState(std::make_unique<MenuState>());
        }
        return;
    }

    // Always recording, so "replay save" after something interesting happens
    // actually has the interesting thing in it. Bounded by kMaxFrames.
    ReplayRecorder::getInstance().startRecording(
        m_isProcedural ? "procedural" : LevelCatalog::nameFor(m_selectedLevelIndex));
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
    // Anything a handler queued on the frame the level was torn down belongs to
    // the level that is going away, not the one being built.
    m_pendingSpawns.clear();
    // The swap record points at cells in the tilemap that is about to be
    // replaced. Carrying it across a warp would write bricks into the new level.
    m_pSwitchSwaps.clear();
    m_pSwitchActive = false;
    m_pSwitchTimer = 0.0f;
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
    // A slide belongs to the pipe and the player it started with, both of which
    // this teardown destroys. Leaving it active would freeze the next level on
    // its first frame: update() would keep returning early to advance a slide
    // whose endpoints are pixels in a level that is gone.
    m_pipeEntry = PipeEntry{};
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
    // the map into open air. Rather than trust it, fall back to the level's own
    // spawn point when the exit is unusable — and then check THAT too, because a
    // level's own spawn point can be just as wrong (all three sub-levels'
    // were). Landing at the start of a room is a visible oddity; landing in the
    // void, or inside a pipe, is an unrecoverable one.
    if (!isSpawnUsable(spawnPos)) {
        std::cerr << "[PlayingState] Pipe exit (" << spawnPos.x << ", " << spawnPos.y
                  << ") is not a usable spawn in " << chosenPath
                  << "; using the level's own spawn point." << std::endl;
        spawnPos = levelData.spawnPoint;
    }
    spawnPos = usableSpawnNear(spawnPos, chosenPath);

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
    if (m_player) {
        m_camera.snapTo(m_player->getBoundingBox().getCenter());
    }
    m_background.setTheme(levelData.theme);
    syncBackdropGround();
    syncVoidPlane();
    settleEndOfLevelScenery();
    refreshWorldLabel(chosenPath);
    findActiveBoss();

    std::cout << "[PlayingState] Loaded sub-level / main level: " << chosenPath << " at spawn (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return true;
}


void PlayingState::regenerateProceduralLevel() {
    m_lastLevelUnverified = !MapGenerator::generateSolvable(m_tileMap, m_entities, m_genConfig);
    if (m_lastLevelUnverified) {
        std::cerr << "[PlayingState] WARNING: regenerated level shipped unverified — "
                     "every solvability reseed attempt failed" << std::endl;
    }
    m_background.setTheme(backdropForGeneratedTheme(m_genConfig.theme));
    syncBackdropGround();
    syncVoidPlane();
    settleEndOfLevelScenery();
    refreshWorldLabel(std::string());
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
    // The old level's boss was just deleted with the rest of m_entities; the new
    // one's has to be found, for the same reasons enter() gives.
    findActiveBoss();
    if (m_minimap) m_minimap->initialize(m_tileMap);
}

void PlayingState::extendEndlessLevelIfNeeded() {
    if (!m_player) return;

    const float lookaheadPx = static_cast<float>(ENDLESS_LOOKAHEAD_TILES) * Constants::TILE_SIZE;
    const float currentWidthPx = static_cast<float>(m_tileMap.getWidth()) * Constants::TILE_SIZE;
    if (m_player->getPosition().x + lookaheadPx < currentWidthPx) return;

    // Generated into an ISOLATED tilemap and entity list, never into m_tileMap
    // or m_entities directly: MapGenerator::generate() clears whatever entity
    // vector it is given (it is meant to build a whole level from nothing),
    // which would delete the player already alive in m_entities.
    ++m_endlessChunkIndex;
    MapGeneratorConfig chunkConfig = m_genConfig;
    chunkConfig.width = ENDLESS_CHUNK_TILES;
    chunkConfig.seed = 0;   // fresh randomness per chunk, not a repeat of chunk 0
    // Rising difficulty with distance, capped so it stays a platformer and not
    // a wall — SPEC's "escalating difficulty curve" for this feature.
    // A chunk, not a level: no player, no checkpoint, no flagpole, no castle,
    // and content across its whole width instead of a 45% dead margin. See
    // MapGeneratorConfig::isChunk.
    chunkConfig.isChunk = true;
    // Escalation is applied to what the player ASKED for, not to a hardcoded
    // base. These four lines used to overwrite the generator sliders the player
    // set on the PROCEDURAL LEVEL menu with a fixed formula, so raising ENEMIES
    // to maximum and starting an Endless run gave the same 0.15 as leaving it at
    // minimum. coinClusterRate was left out of the escalation entirely, which
    // mattered more than it looks: until this session every ground enemy spawn
    // was nested inside the coin-cluster branch.
    chunkConfig.pitProbability  = std::min(0.35f, chunkConfig.pitProbability  + 0.02f * m_endlessChunkIndex);
    chunkConfig.enemySpawnRate  = std::min(0.35f, chunkConfig.enemySpawnRate  + 0.02f * m_endlessChunkIndex);
    chunkConfig.coinClusterRate = std::min(0.45f, chunkConfig.coinClusterRate + 0.02f * m_endlessChunkIndex);
    chunkConfig.roughness       = std::min(1.0f,  chunkConfig.roughness       + 0.05f * m_endlessChunkIndex);
    chunkConfig.difficulty = m_endlessChunkIndex >= 6 ? MapDifficulty::Hard
                            : m_endlessChunkIndex >= 3 ? MapDifficulty::Medium
                                                        : MapDifficulty::Easy;

    // A boss every ENDLESS_BOSS_CHUNK_INTERVAL chunks, and never anywhere else.
    //
    // The difficulty ramp above turns Hard from chunk 6 onwards, and the
    // generator drops a Bowser into every Hard map — so an Endless run past ~600
    // tiles spliced ANOTHER Bowser into the level every hundred tiles, each one
    // breathing fire on sight, against a findActiveBoss() whose own comment says
    // "one boss per level". A boss is a milestone here: it gives the run a
    // rhythm, it is the only thing in Endless Mode that is not more of the same,
    // and meeting one has to be an event rather than the weather.
    chunkConfig.bossArena = (m_endlessChunkIndex % ENDLESS_BOSS_CHUNK_INTERVAL) == 0;

    TileMap chunkMap;
    std::vector<std::unique_ptr<Entity>> chunkEntities;
    if (!MapGenerator::generateSolvable(chunkMap, chunkEntities, chunkConfig)) {
        m_lastLevelUnverified = true;
        std::cerr << "[PlayingState] WARNING: Endless Mode chunk " << m_endlessChunkIndex
                  << " shipped unverified — every solvability reseed attempt failed" << std::endl;
    }

    const int offsetTiles = m_tileMap.getWidth();
    const float offsetPx = static_cast<float>(offsetTiles) * Constants::TILE_SIZE;

    m_tileMap.expandToFit(offsetTiles + chunkConfig.width, chunkMap.getHeight());
    for (int y = 0; y < chunkMap.getHeight(); ++y) {
        for (int x = 0; x < chunkConfig.width; ++x) {
            m_tileMap.setTile(offsetTiles + x, y, chunkMap.getTileType(x, y));
        }
    }
    // A flat safety bridge across the seam: each chunk's terrain is generated
    // independently, so nothing guarantees chunk N's left edge lines up with
    // chunk N-1's right edge. Without this, a seam could open onto a pit the
    // player had no way to see coming.
    const int seamGroundY = m_tileMap.getHeight() - 2;
    for (int x = offsetTiles - 2; x < offsetTiles + 4 && x < m_tileMap.getWidth(); ++x) {
        for (int y = seamGroundY; y < m_tileMap.getHeight(); ++y) {
            m_tileMap.setTile(x, y, TileType::Ground);
        }
    }

    // Splice entities, shifted by the chunk's offset. The generator always
    // builds its own Mario and, near its own local exitX, a flagpole/castle —
    // none of which belong in a middle-of-nowhere chunk: the real player
    // already exists, and Endless Mode has no "end" to reach.
    bool splicedBoss = false;
    int splicedCount = 0;
    int splicedFirstTile = m_tileMap.getWidth();
    int splicedLastTile = 0;
    for (auto& entity : chunkEntities) {
        if (!entity) continue;
        const std::string type = entity->getTypeName();
        if (dynamic_cast<Player*>(entity.get())) continue;
        if (type == "flagpole" || type == "castle" || type == "pipe") continue;

        // An axe belongs to the fight it was generated with. If a fight is
        // already live the boss below is dropped, so its axes have to go with
        // it: left in the world they are landmarks that publish BridgeChopped
        // against a DIFFERENT arena's counter, and reaching one would chop the
        // live fight's bridge from two chunks away. Was already true of the one
        // axe buildBossArena() used to place; it now places three.
        if (type == "bridge_axe" && m_activeBoss) continue;

        // One fight at a time. findActiveBoss() takes the first Boss it finds
        // and its own comment says "one boss per level"; a second one spliced in
        // while a fight is live would be invisible to the HUD, unlockable by the
        // camera and unkillable by the axe, while still breathing fire.
        if (auto* boss = dynamic_cast<Boss*>(entity.get())) {
            if (m_activeBoss) {
                std::cout << "[PlayingState] Endless Mode: dropped a second "
                          << boss->getDisplayName() << " from chunk "
                          << m_endlessChunkIndex << "; a fight is already live."
                          << std::endl;
                continue;
            }
            splicedBoss = true;
        }

        // translate(), not setPosition(): several of these classes cache their
        // spawn point at construction and drive themselves back to it, so a
        // plain setPosition() left them teleporting to their chunk-LOCAL
        // coordinates on their first update — i.e. back near the start of the
        // world. See Entity::translate for the full list. This is the defect
        // that made appended chunks look empty (R21 D7).
        entity->translate(sf::Vector2f(offsetPx, 0.0f));
        ++splicedCount;
        const int tileX = static_cast<int>(entity->getPosition().x / Constants::TILE_SIZE);
        splicedFirstTile = std::min(splicedFirstTile, tileX);
        splicedLastTile  = std::max(splicedLastTile, tileX);
        admitEntity(entity.get());
        m_entities.push_back(std::move(entity));
    }

    // Adopt the new fight, exactly as loading a level would. Guarded on
    // m_activeBoss being null because findActiveBoss() also clears m_arenaLocked,
    // which would release the camera in the middle of a live fight.
    if (splicedBoss && !m_activeBoss) {
        findActiveBoss();
    }

    const AABB worldBounds{0.0f, 0.0f,
                           static_cast<float>(m_tileMap.getWidth()) * Constants::TILE_SIZE,
                           static_cast<float>(m_tileMap.getHeight()) * Constants::TILE_SIZE};
    if (m_arenaLocked) {
        // A boss arena sits at the END of its chunk, so the player is well
        // inside the lookahead when the fight starts and the next chunk is
        // appended DURING it. Writing the world bounds onto the camera here
        // would silently undo the arena lock mid-fight, and releaseBossArena()
        // would then restore a snapshot taken before this chunk existed —
        // clamping the camera to a world two hundred tiles shorter than the one
        // the player is standing in. Update what the release will restore
        // instead, and leave the camera on the arena.
        m_preArenaCameraBounds = worldBounds;
    } else {
        m_camera.setBounds(worldBounds);
    }
    if (m_minimap) m_minimap->initialize(m_tileMap);

    // The entity count and world-tile span are the two facts worth having here:
    // "the far chunks have no entities in them" (R21 D7) was reported from
    // play, and a chunk that splices nothing, or splices everything back at
    // world tile 20-80, is visible in this one line.
    std::cout << "[PlayingState] Endless Mode: appended chunk " << m_endlessChunkIndex
              << (chunkConfig.bossArena ? " (BOSS)" : "")
              << ": " << splicedCount << " entities"
              << (splicedCount > 0 ? " across world tiles " + std::to_string(splicedFirstTile)
                                      + "-" + std::to_string(splicedLastTile)
                                   : std::string())
              << "; tilemap now " << m_tileMap.getWidth() << " tiles wide"
              // The census is the whole evidence for the off-camera gate. A run
              // never discards a chunk, so `live` is what the ungated loop used
              // to update every single frame; `thinking` is what it costs now.
              // Printed on the append line rather than per frame because that
              // is the only moment the number changes by more than churn.
              << "; entities live " << (m_entitiesThought + m_entitiesFrozen)
              << ", thinking " << m_entitiesThought
              << ", frozen off-camera " << m_entitiesFrozen
              << ", exempt off-camera " << m_entitiesExempt << "." << std::endl;
}

void PlayingState::saveToSlot(int slot) {
    if (!m_player) return;
    // See the checkpoint autosave's comment above (D30): this used to hardcode
    // level 1 too, so the pause menu's manual Save and DevPanel's Save button
    // shared the exact same bug.
    const int levelId = m_selectedLevelIndex + 1;
    const std::string levelName = m_isProcedural ? "Procedural"
                                                  : LevelCatalog::nameFor(m_selectedLevelIndex);
    Serializer::saveGame(slot, *m_player, levelId, levelName, Constants::LEVEL_TIME,
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

    // Switch to the level the save recorded (D30). This instance was already
    // running some level before this call — MenuState's LOAD GAME picker
    // builds a throwaway World 1-1 PlayingState specifically to give this
    // method something to overwrite (see the constructor's pendingLoadSlot
    // doc), and DevPanel's Load button calls this on whatever level a
    // developer happened to be testing — so lvlId (1-based, same convention
    // saveToSlot()/the checkpoint autosave write) has to be trusted over
    // whatever level is already loaded. Without this, the player's saved
    // world-space position gets applied against the wrong tile map: LOAD GAME
    // always resumed into World 1-1 no matter which level the slot held.
    // Procedural/endless levels have no catalog entry to switch to and never
    // wrote anything but level 1 to begin with, so they are left alone.
    const int targetIndex = lvlId - 1;
    if (!m_isProcedural && LevelCatalog::isValidIndex(targetIndex) &&
        targetIndex != m_selectedLevelIndex) {
        if (loadLevelByPath(LevelCatalog::pathFor(targetIndex))) {
            m_selectedLevelIndex = targetIndex;
            SoundManager::getInstance().playLevelBGM(m_selectedLevelIndex);
        } else {
            std::cerr << "[PlayingState] loadFromSlot: could not switch to level "
                      << lvlId << "; resuming in the level already loaded." << std::endl;
        }
    }

    if (starCoins.size() >= 3) {
        m_starCoinsCollected = {starCoins[0], starCoins[1], starCoins[2]};
    }
    // adoptPlayer refreshes m_player, InputManager and Game together; assigning
    // m_entities[0] directly here is what left m_player dangling (audit A-3).
    // Runs after the level switch above: loadLevelByPath() spawns its own
    // throwaway player at the fresh level's spawn point, and adoptPlayer()
    // replaces it with the one just deserialized (already at its saved
    // position from Serializer::loadGame, which restores it independently of
    // checkpointX/Y below).
    adoptPlayer(std::move(loadedPlayer));

    // The checkpoint the save recorded, so a death in the resumed session
    // respawns at the save point rather than the level's own start (which
    // loadLevelByPath(), above, would otherwise have left as the only option).
    m_checkpointPosition = sf::Vector2f(checkX, checkY);
    m_hasCheckpoint = true;

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
    // recorded this frame can never also be consumed this frame. The clock
    // advances with the simulation and stops with it, which a wall clock would
    // not. It used to be derived from the level countdown (LEVEL_TIME minus
    // m_levelTimer), which stops in Endless Mode and under the FREEZE TIMER
    // cheat — freezing the shadow's whole replay timeline with it.
    m_shadow->recordFrame(m_runElapsed, *m_player);
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

void PlayingState::renderLightPass(sf::RenderTarget& target) {
    // The level editor paints its own overlay over this same world and its
    // author needs to see what they are placing. Darkening the level being
    // edited would be a tool regression dressed up as an effect. EditorState is
    // a separate state that never calls this render(), so the editor is covered
    // from both directions.
    if (m_mapEditor.isActive()) return;

    // Sizes in world px. A ~9-tile lamp leaves a cave readable while still
    // hiding what is coming; a fireball is a thrown ember, not a torch. The
    // defaults and the reasoning for each now live on LightingTunables, so
    // Debug > Lighting can move them without a rebuild.
    const LightingTunables& tune = m_lightingTunables;
    const auto tintFromSlider = [](const float rgb[3]) {
        return sf::Color(static_cast<std::uint8_t>(rgb[0] * 255.0f),
                         static_cast<std::uint8_t>(rgb[1] * 255.0f),
                         static_cast<std::uint8_t>(rgb[2] * 255.0f));
    };

    std::vector<LightingRenderer::Light> lights;
    lights.reserve(LightingRenderer::MAX_LIGHTS);

    auto addPlayerLight = [&](Player* who) {
        if (!who || !who->isActive()) return;
        const AABB bb = who->getBoundingBox();
        LightingRenderer::Light light;
        light.worldPosition = {bb.x + bb.width * 0.5f, bb.y + bb.height * 0.5f};
        // A perfectly steady disc reads as a vignette bug. A slow 5% breathe
        // reads as a carried lamp. Driven off m_runElapsed rather than a render
        // clock so a replay lights the same way it was recorded.
        light.radius    = tune.playerRadius *
                          (1.0f + tune.playerBreathe * std::sin(m_runElapsed * 3.7f));
        light.intensity = 1.0f;
        // A warm near-neutral SHADOW colour, not a lamp colour -- see
        // LightingRenderer::Light::shadowTint. Anything brighter than the scene
        // here draws a bright ring instead of a lamp.
        light.shadowTint = tintFromSlider(tune.playerShadowTint);
        lights.push_back(light);
    };
    addPlayerLight(m_player);
    addPlayerLight(m_player2);

    // Free camera (F9) detaches the view from the player, who can then be
    // anywhere — including off-screen, leaving the operator panning a black
    // rectangle. A lamp on the camera itself keeps the cheat usable in exactly
    // the levels it is most useful for.
    if (Game::getInstance().debugCheats().detachesCamera()) {
        LightingRenderer::Light light;
        light.worldPosition = m_camera.getView().getCenter();
        light.radius        = tune.freeCameraRadius;
        light.intensity     = 1.0f;
        light.shadowTint    = sf::Color(44, 50, 62);
        lights.push_back(light);
    }

    // Fireballs are already ordinary entities in m_entities with their own
    // lifetimes and their own pool. Reading them here means no second registry
    // to keep in sync and no way for a light to outlive the thing casting it —
    // a released fireball is inactive on the very next frame.
    for (const auto& entity : m_entities) {
        if (lights.size() >= LightingRenderer::MAX_LIGHTS) break;
        if (!entity || !entity->isActive()) continue;

        const bool isPlayerShot = dynamic_cast<Fireball*>(entity.get()) != nullptr;
        const bool isBossShot   = dynamic_cast<BossFireball*>(entity.get()) != nullptr;
        if (!isPlayerShot && !isBossShot) continue;

        const AABB bb = entity->getBoundingBox();
        LightingRenderer::Light light;
        light.worldPosition = {bb.x + bb.width * 0.5f, bb.y + bb.height * 0.5f};
        light.radius        = tune.fireballRadius;
        // Short of 1.0 on purpose: a fireball that cleared the darkness
        // completely would be a hole indistinguishable from the player's, and
        // the tint below only shows in shadow the lamp has NOT cleared.
        light.intensity     = tune.fireballIntensity;
        // Ember, not flame: the shadow a fireball fails to clear goes warm, and
        // that warmth is what separates its pool of light from the player's.
        light.shadowTint    = isBossShot ? sf::Color(104, 34, 10)
                                         : sf::Color(96, 44, 14);
        lights.push_back(light);
    }

    // Debug > Lighting can hold the cycle at a chosen phase. Feeding the
    // renderer a synthetic elapsed time rather than adding a parameter keeps the
    // whole feature inside the caller that owns the clock: dayNightPhase() is
    // periodic, so phase p is reached at p * DAY_NIGHT_PERIOD seconds.
    const float lightingClock =
        m_lightingTunables.clockFrozen
            ? m_lightingTunables.frozenPhase * LightingRenderer::DAY_NIGHT_PERIOD
            : m_runElapsed;
    m_lighting.render(target, m_camera.getView(), lights,
                      m_background.getTheme(), lightingClock);
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

    // Rewinding out of a death has to actually cancel the death.
    //
    // Falling into the void starts a death sequence and puts the player into
    // m_dying, which zeroes their controls and drives them through the floor.
    // The rewind path restored the POSITION from the snapshot and nothing else,
    // so the player reappeared above the pit still dying, still uncontrollable,
    // and fell straight back in — R looked like it did nothing. The Memento is
    // meant to undo the last few seconds, and dying is part of what happened in
    // them.
    //
    // A player already eliminated is left alone: they have no lives to come back
    // to and the run is over for them.
    for (Player* who : {m_player, m_player2}) {
        if (!who) continue;
        DeathState* death = deathStateFor(who);
        if (!death || death->eliminated) continue;
        if (death->phase == DeathPhase::None) continue;
        // GameOver has already started a screen transition whose callback
        // replaces this state; there is nothing left here to rewind into.
        if (death->phase == DeathPhase::GameOver) continue;

        death->phase = DeathPhase::None;
        death->timer = 0.0f;
        who->endDeathFall();
        std::cout << "[PlayingState] Rewound out of a death." << std::endl;
    }

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
    // Player 1 too. The prune never removes it, so this case never mattered —
    // until the level editor gained a working Erase tool, which can delete the
    // player the same as anything else. m_player, InputManager and Game all hold
    // the same raw pointer and all three have to let go together (audit A-3).
    if (entity == m_player) {
        m_player = nullptr;
        InputManager::getInstance().registerPlayer(nullptr, 0);
        Game::getInstance().setPlayer(nullptr);
    }
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
    if (Player* fighter = activeParticipant()) {
        m_camera.snapTo(fighter->getBoundingBox().getCenter());
    }
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

    // The axe roster is part of the fight this level has, and it has to be
    // sized against the arena this call just found. See configureBridgeAxes().
    configureBridgeAxes();
}

int PlayingState::axeQuotaForDifficulty() {
    const std::string tier = Game::getInstance().difficulty().getId();
    if (tier == "easy") return 1;
    if (tier == "hard") return 3;
    return 2;   // normal, and anything config.json has been hand-edited into
}

void PlayingState::configureBridgeAxes() {
    m_axesTotal = 0;
    m_axesRemaining = 0;

    // Only the live fight's axes. Endless Mode splices chunk after chunk into
    // one tilemap, so an uncollected axe from a fight two chunks back is still
    // in m_entities and must not be counted towards this one.
    const bool haveArena = m_activeBoss && m_activeBoss->hasArena();
    const AABB arena = haveArena ? m_activeBoss->getArena() : AABB{};

    std::vector<BridgeAxe*> axes;
    for (const auto& entity : m_entities) {
        auto* axe = dynamic_cast<BridgeAxe*>(entity.get());
        if (!axe || !axe->isActive() || axe->isSwung()) continue;
        if (haveArena) {
            const float x = axe->getPosition().x;
            if (x < arena.x || x >= arena.x + arena.width) continue;
        }
        axes.push_back(axe);
    }
    if (axes.empty()) return;

    std::sort(axes.begin(), axes.end(), [](const BridgeAxe* a, const BridgeAxe* b) {
        return a->getPosition().x < b->getPosition().x;
    });

    const int quota = std::min(axeQuotaForDifficulty(), static_cast<int>(axes.size()));

    // Which axes survive, and why in this order:
    //   - the RIGHTMOST always, because it is the one the arena was built
    //     around — past Bowser, past the far post, standing on the ledge the
    //     player lands on once the bridge is gone. Easy is exactly the fight
    //     this level has always shipped.
    //   - the LEFTMOST next, which is "left + right" for Normal.
    //   - then the interior ones, left to right, which is "left + middle +
    //     right" for Hard.
    // A level (or a generated chunk) carrying fewer axes than the quota simply
    // runs with what it has rather than having axes invented for it.
    std::vector<bool> keep(axes.size(), false);
    keep.back() = true;
    int kept = 1;
    if (kept < quota) {
        keep.front() = true;
        ++kept;
    }
    for (std::size_t i = 1; i + 1 < axes.size() && kept < quota; ++i) {
        keep[i] = true;
        ++kept;
    }

    for (std::size_t i = 0; i < axes.size(); ++i) {
        if (keep[i]) continue;
        // Destroyed rather than hidden: a surplus axe left inert in the world
        // is a landmark the player walks to and nothing happens at.
        axes[i]->destroy();
        forgetEntity(axes[i]);
    }

    m_axesTotal = kept;
    m_axesRemaining = kept;
    std::cout << "[PlayingState] Bridge axes for difficulty "
              << Game::getInstance().difficulty().getId() << ": " << kept
              << " of " << axes.size() << " placed; all must be reached."
              << std::endl;
}

void PlayingState::updateFootstep(Player* who, float& timer, float dt) {
    if (!who || !who->isActive() || who->isDying()) { timer = 0.0f; return; }
    if (timer > 0.0f) timer -= dt;

    // Grounded and actually moving. Airborne, standing still or wall-sliding
    // (which has its own dust cue, above) all skip this — a footstep on the way
    // up or while parked mid-air reads as a bug, not a cadence.
    if (!who->isOnGround() || std::abs(who->getVelocity().x) < 20.0f) return;
    if (timer > 0.0f) return;

    const bool running = std::abs(who->getVelocity().x) > Constants::WALK_SPEED + 10.0f;
    timer = running ? 0.18f : 0.30f;

    const AABB box = who->getBoundingBox();
    const TileType underfoot = m_tileMap.getTileSurfaceType(
        box.x + box.width * 0.5f, box.y + box.height + Constants::GROUND_CHECK_OFFSET);

    // Only three footstep WAVs are loaded (SoundManager::loadAllSounds()); SPEC
    // 11.4 lists four surfaces (grass/ground, stone/brick, ice, metal). Ice and
    // every other solid, non-ground tile share footstep_floor rather than
    // inventing a fourth asset that was never delivered. Bowser's Castle
    // (levelIndex 2, see SoundManager::playLevelBGM's comment for the mapping)
    // is the one case with a real "Metal" identity in the SPEC, so it overrides
    // by level rather than by tile.
    std::string sfx = "footstep_grass";
    float vol = 0.25f;
    if (m_selectedLevelIndex == 2) {
        sfx = "footstep_metalcap";
        vol = 0.10f; // Soft ambient volume for metal walk sfx
    } else if (underfoot != TileType::Ground) {
        sfx = "footstep_floor";
        vol = 0.20f;
    }
    // Lower volume scale for footstep audio so it remains a subtle ambient cue
    SoundManager::getInstance().playSound(sfx, 1.0f, vol);
}

void PlayingState::updateBossArena() {
    // The boss entity is owned by m_entities and pruned when it deactivates, so
    // the pointer has to be dropped in the same frame it stops being active.
    if (m_activeBoss && (!m_activeBoss->isActive() || m_activeBoss->isDefeated())) {
        releaseBossArena();
        if (!m_activeBoss->isActive()) {
            m_activeBoss = nullptr;
        }
        return;
    }

    // The survivor of a two-player match, not m_player: an eliminated Player 1
    // nulls m_player, and this whole method then returned — no arena lock, no
    // escape clamp and no battle music for the player still fighting.
    Player* fighter = activeParticipant();
    if (!m_activeBoss || !fighter || !m_activeBoss->hasArena()) return;

    const AABB arena = m_activeBoss->getArena();

    if (!m_arenaLocked) {
        // Lock once the player is properly inside, not the instant they clip the
        // edge, so the camera does not snap while they are still walking in.
        const sf::Vector2f p = fighter->getPosition();
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
    const AABB box = fighter->getBoundingBox();
    sf::Vector2f p = fighter->getPosition();
    if (p.x < arena.x) {
        p.x = arena.x;
    } else if (p.x + box.width > arena.x + arena.width) {
        p.x = arena.x + arena.width - box.width;
    }
    fighter->setPosition(p);
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
    hudData.bossStaggered = m_activeBoss->isStaggered();
    // Only Bowser has the fire-stagger route; -1 means "this boss has no such
    // mechanic" and the HUD draws nothing rather than a misleading zero.
    if (const auto* bowser = dynamic_cast<const Bowser*>(m_activeBoss)) {
        hudData.bossFireHitsToStagger = bowser->getFireHitsToStagger();
    } else {
        hudData.bossFireHitsToStagger = -1;
    }
    // The axe route's progress, for the same reason the stagger count is here.
    hudData.bossAxesTotal   = m_axesTotal;
    hudData.bossAxesReached = m_axesTotal - m_axesRemaining;
}

Player* PlayingState::activeParticipant() const {
    return m_player ? m_player : m_player2;
}

void PlayingState::rememberEliminatedIdentity(const Player* who, DeathState& death) const {
    if (!who) return;
    death.characterName = who->getCharacterName();
    death.finalCoins    = who->getCoins();
    death.finalScore    = who->getScore();
    // Matches the label the live block builds, so the badge does not rename a
    // CPU opponent "P2" the moment it is knocked out.
    death.badgeLabel = m_aiController ? std::string("CPU") : std::string("P2");
}

void PlayingState::applyEliminatedBadges(HudData& hudData) const {
    if (m_death.eliminated && !m_death.characterName.empty()) {
        hudData.eliminated    = true;
        hudData.characterName = m_death.characterName;
        hudData.lives         = 0;
        hudData.coins         = m_death.finalCoins;
        hudData.score         = m_death.finalScore;
        // Without this the badge showed setupTestScene()'s mock 102520/57/9:
        // Game::getPlayer() is null once Player 1 is gone, so update()'s sync
        // fell into its "running the test scene" fallback for a real run.
        hudData.comboCount = 1;
        hudData.comboTimer = 0.0f;
    }

    if (m_match.hasSecondPlayer() && m_death2.eliminated &&
        !m_death2.characterName.empty()) {
        // hasSecondPlayer stays TRUE for a player who is out. The badge is how
        // the survivor is told the match is now one-sided; removing it says
        // nothing, which is what it used to do.
        hudData.hasSecondPlayer     = true;
        hudData.secondEliminated    = true;
        hudData.secondCharacterName = m_death2.characterName;
        hudData.secondLives         = 0;
        hudData.secondCoins         = m_death2.finalCoins;
        hudData.secondPlayerLabel   = m_death2.badgeLabel;
    }
}

PlayingState::DeathState* PlayingState::deathStateFor(const Player* who) {
    if (who && who == m_player2) return &m_death2;
    if (who && who != m_player)  return nullptr;   // the shadow, or a stale pointer
    return &m_death;
}

bool PlayingState::anyDeathInProgress() const {
    return m_death.phase != DeathPhase::None || m_death2.phase != DeathPhase::None;
}

sf::Vector2f PlayingState::rescueDestination(const Player* who) const {
    const float mapRight = m_tileMap.getWidth() * Constants::TILE_SIZE;
    const AABB box = who->getBoundingBox();

    // (a) The column they fell from. Clamped into the map first, so a player who
    // left sideways is put back at the nearest real column rather than dropped
    // straight to (b) — "where I was" is still the best answer available.
    const float columnX = std::clamp(who->getPosition().x, 0.0f,
                                     mapRight - Constants::TILE_SIZE);
    // From the top of the map down: the FIRST solid tile in the column is the
    // surface they were walking on before they left it. Scanning from their
    // current y would find whatever is under the pit instead, which is the one
    // place they must not be put back.
    const float floorTop = floorBelow(columnX + Constants::TILE_SIZE * 0.5f, 0.0f);
    if (floorTop >= 0.0f &&
        isSpawnUsable({columnX, floorTop - Constants::TILE_SIZE})) {
        // Placed by the FEET, not by the 32px probe box: a Super or Cape player
        // is taller than the box isSpawnUsable() tests with, and landing them by
        // the box would bury them by the difference.
        return {columnX, floorTop - box.height};
    }

    // (b) A genuine bottomless pit — nothing solid anywhere in that column. The
    // last checkpoint is the next-best claim on "where they were".
    if (m_hasCheckpoint && isSpawnUsable(m_checkpointPosition)) {
        return m_checkpointPosition;
    }

    // (c) The level's own spawn, nudged onto something usable if the level file
    // buried it (the same treatment every other transition into this level got).
    return usableSpawnNear(m_levelSpawnPoint, "<immortal rescue>");
}

void PlayingState::rescuePlayer(Player* who, const char* reason) {
    if (!who) return;

    const sf::Vector2f destination = rescueDestination(who);
    who->setPosition(destination);
    who->setVelocity({0.0f, 0.0f});
    who->setGrounded(false);

    // Deliberately NOT touched: lives, coins, score, the power-up form, the
    // combo and m_levelTimer. "No lost progress" is the whole point — a rescue
    // that cost the fire flower would still ruin the take it was meant to save.
    //
    // A second of i-frames, because the rescue can land next to whatever was
    // chasing them. Under 1.7s so it does not trip the hurt animation
    // (Player::update reads the same timer to pick that frame). No enemy sweep:
    // makeSpawnSafe() deletes enemies near a respawn, and deleting level content
    // out from under a recording is exactly the kind of surprise this must not
    // spring.
    who->setInvincible(1.0f);

    if (who == m_player && !Game::getInstance().debugCheats().detachesCamera()) {
        m_camera.snapTo(destination);
    }

    std::cout << "[Cheats] IMMORTAL: rescued " << (who == m_player2 ? "Player 2" : "Player 1")
              << " (" << reason << ") to (" << destination.x << ", " << destination.y
              << "). Lives unchanged at " << who->getLives() << "." << std::endl;
}

void PlayingState::killPlayer(Player* who, const char* reason) {
    // Null means Player 1: the debug key and the level-timer path have no
    // particular player in mind.
    if (!who) who = m_player;
    if (!who) return;

    // Debug > Cheats' IMMORTAL, checked at the single door every lethal event in
    // the game comes through — the void plane, the level clock, lava, a crush, a
    // hit that finishes Small Mario. Nothing downstream of here runs: no death
    // fall, no life spent, no game over.
    if (Game::getInstance().debugCheats().rescueInsteadOfKill()) {
        rescuePlayer(who, reason);
        return;
    }

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
        // Before destroy(): this is the last frame on which anything can be
        // asked of `who`, and the HUD needs to keep naming them afterwards.
        rememberEliminatedIdentity(who, *death);

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

        // A boss is an Enemy by category but is not a spawn hazard to be swept
        // away like a wandering Goomba: destroying it here does not just remove
        // one enemy, it silently ends the fight. forgetEntity() treats a boss's
        // destruction as a real death — it drops m_activeBoss and releases the
        // arena (camera bounds, scroll mode, BGM) — so a boss standing near the
        // checkpoint while the player respawns used to end the encounter with
        // no win recorded and nothing left to fight (D29). The arena's own
        // "no escape until defeated" clamp (updateBossArena) already keeps the
        // player inside with the boss, and the 1.5s landing invincibility below
        // still protects against a boss attack landing on the respawn frame, so
        // skipping the boss here costs nothing the spawn-camping fix needed.
        if (dynamic_cast<Boss*>(entity.get())) continue;

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

sf::Vector2f PlayingState::findSafeRespawn(sf::Vector2f preferred, const Player* nextTo) const {
    const AABB view = m_camera.getVisibleBounds();
    // A margin in from the view edge, so the respawned player is visibly inside
    // the frame rather than clipping its border — and, in versus, is not
    // instantly grabbed by the tether that shoves whoever is at the edge.
    const float margin = Constants::TILE_SIZE * 2.0f;
    const float leftLimit  = std::max(Constants::TILE_SIZE, view.x + margin);
    const float rightLimit = std::min(m_tileMap.getWidth() * Constants::TILE_SIZE
                                          - Constants::TILE_SIZE * 2.0f,
                                      view.x + view.width - margin);

    auto standable = [this](float x, float fromY) -> float {
        const float floorTop = floorBelow(x, fromY);
        if (floorTop < 0.0f) return -1.0f;
        // Two tiles of headroom, so the player does not materialise inside the
        // underside of a platform and get pushed out sideways.
        const int gx = static_cast<int>(x / Constants::TILE_SIZE);
        const int floorRow = static_cast<int>(floorTop / Constants::TILE_SIZE);
        for (int y = floorRow - 2; y < floorRow; ++y) {
            if (y < 0) continue;
            if (TileMap::getInfo(m_tileMap.getTileType(gx, y)).isSolid) return -1.0f;
        }
        return floorTop;
    };

    // Start from wherever makes most sense and walk outwards: beside the
    // surviving player when there is one, otherwise the preferred point.
    const float originX = nextTo ? nextTo->getPosition().x : preferred.x;
    const float searchTop = std::max(0.0f, view.y);

    // Beside the partner first — one tile clear, then further out — so the two
    // start the next life together rather than at opposite ends of the frame.
    for (int step = 1; step <= 12; ++step) {
        for (const float dir : {1.0f, -1.0f}) {
            const float x = originX + dir * step * Constants::TILE_SIZE;
            if (x < leftLimit || x > rightLimit) continue;
            const float floorTop = standable(x, searchTop);
            if (floorTop < 0.0f) continue;
            // One tile above the floor: dropped in, not embedded in it.
            return {x, floorTop - Constants::TILE_SIZE * 1.5f};
        }
    }

    // Nothing on screen has a floor. The preferred point at least has the level
    // designer's intent behind it, which beats a guess.
    return preferred;
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

    // In a two-player match the camera frames both players, so the checkpoint is
    // the wrong destination whenever the survivor has moved on from it: it puts
    // the respawned player off screen, or drags the frame back across the level
    // to fetch them. Come back next to the partner instead, inside the view and
    // on a floor.
    Player* const partner = isFirst ? m_player2 : m_player;
    const bool partnerPlaying = m_match.hasSecondPlayer() && partner &&
                                partner->isActive() && !partner->isDying();
    if (partnerPlaying) {
        respawn = findSafeRespawn(respawn, partner);
    } else {
        // Single player, or the last one standing. The checkpoint is right, but
        // it still has to have a floor under it — a checkpoint taken over a pit
        // used to respawn the player straight back into the pit, once per life.
        if (floorBelow(respawn.x, respawn.y) < 0.0f) {
            std::cerr << "[PlayingState] Respawn point has no floor beneath it; "
                         "looking for one on screen." << std::endl;
            respawn = findSafeRespawn(respawn, nullptr);
        }
    }

    who->setPosition(respawn);
    who->setVelocity({0.0f, 0.0f});
    who->setGrounded(false);
    // A corpse must not still be carrying a shell it picked up before dying.
    who->dropHeldEntity();

    // A death costs the power-up, always.
    //
    // Taking a hit steps the form down — Fire to Super to Small — so a damage
    // death already arrives here Small. Falling into a pit does not go through
    // takeDamage() at all, so a Fire Mario who missed a jump came back still
    // holding the flower: the pit was the one hazard in the game with no cost
    // beyond the life. The combo goes with it for the same reason.
    who->setForm(Player::Form::Small);
    who->resetCombo();
    makeSpawnSafe(who, respawn);

    if (!isFirst) {
        std::cout << "[PlayingState] Player 2 respawned at (" << respawn.x << ", "
                  << respawn.y << "). Lives remaining: " << who->getLives() << std::endl;
        return;
    }

    // Only snap the camera when it is following this player alone. In a match it
    // frames the midpoint of both, and snapping to one of them fights that.
    if (!m_match.hasSecondPlayer() || !m_player2 || !m_player2->isActive()) {
        m_camera.snapTo(respawn);
    }

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
    m_runElapsed = 0.0f;
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
    summary.isEndless = m_isEndless;
    summary.endlessDistanceTiles = static_cast<int>(m_endlessBestDistanceTiles);
    summary.starCoins = static_cast<int>(std::count(m_starCoinsCollected.begin(),
                                                    m_starCoinsCollected.end(), true));
    summary.match = m_match;
    summary.cause = m_death.reason;
    summary.caughtByShadow = m_shadow && m_shadow->caughtPlayerRecently();
    // Told GameOverState this run was an editor playtest, the same way
    // m_isPlaytest already tells leaveToCallingScreen() to pop instead of
    // changing to the main menu on a non-death exit.
    summary.isPlaytest = m_isPlaytest;
    if (m_player) {
        summary.score         = m_player->getScore();
        summary.coins         = m_player->getCoins();
        summary.characterName = m_player->getCharacterName();
        // Endless Mode has no time bonus and no flagpole height bonus to end
        // on, so distance is what the score actually measures — 10 points per
        // tile travelled, on top of whatever coins/stomps were picked up
        // along the way.
        if (m_isEndless) {
            summary.score += summary.endlessDistanceTiles * 10;
        }
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
    m_runElapsed = 0.0f;
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

void PlayingState::leaveToCallingScreen() {
    Game& game = Game::getInstance();
    if (m_isPlaytest) {
        // Pop, not change: EditorState is directly underneath with the author's
        // level still loaded, and replacing this state would strand it under a
        // main menu it can never be reached from.
        game.popState();
        return;
    }
    game.changeState(std::make_unique<MenuState>());
}

void PlayingState::advanceToNextLevel() {
    // Campaign order matches the level dropdown: 1-1, 1-1 sub, 1-2, 1-2 sub,
    // 1-3, 1-3 sub, bonus. Finishing the last one returns to the menu.
    const int nextIndex = m_selectedLevelIndex + 1;

    // A custom level is a one-off, not position zero of the campaign: advancing
    // from it would drop the player into World 1-2 and, worse, credit them with
    // a New Game+ cycle for finishing a level they wrote themselves.
    if (m_isProcedural || !m_customLevelPath.empty() || nextIndex >= LevelCatalog::count()) {
        if (!m_isProcedural && m_customLevelPath.empty()) {
            // Finishing the last level opens the next New Game+ cycle: the level
            // flags reset, the counter and the unlocks do not (task 11.3).
            MetaGame::advanceNewGamePlus();
            std::cout << "[PlayingState] New Game+ level is now "
                      << MetaGame::newGamePlusLevel() << "." << std::endl;
        }
        std::cout << "[PlayingState] Campaign complete — returning to menu." << std::endl;
        // The bool is captured by value, not `this`: the callback fires 0.8s
        // later and must not depend on this state still existing.
        ScreenTransitionManager::getInstance().fadeOut(0.8f, [playtest = m_isPlaytest]() {
            if (playtest) Game::getInstance().popState();
            else Game::getInstance().changeState(std::make_unique<MenuState>());
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
    m_runElapsed = 0.0f;
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
    // And the form carries too. Reaching the flagpole as Fire Mario and starting
    // the next level Small threw away the thing the player spent the level
    // earning; the series has always let you keep it between stages. Deaths
    // still take it — see respawnPlayer() — so it is kept by surviving, which is
    // what makes it worth having.
    Player::Form oldForm = Player::Form::Small;
    if (m_player) {
        oldCoins = m_player->getCoins();
        oldScore = m_player->getScore();
        oldLives = m_player->getLives();
        oldForm  = m_player->getForm();
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
    // setStartingForm, not setForm: the box is sized to the form without the
    // feet-planting shift a mid-life change performs, which would drop the
    // player 2px into the floor on every level transition.
    newPlayer->setStartingForm(oldForm);

    adoptPlayer(std::move(newPlayer));
}

std::unique_ptr<Entity> PlayingState::spawnProjectile(int entityType, sf::Vector2f position,
                                                      sf::Vector2f velocity) {
    // Pool acquisition for the three projectile types that churn. The mapping
    // from EntityType to pool lives here rather than in a switch because only
    // three types are pooled and the factory is the fallback for everything else.
    const auto et = static_cast<EntityType>(entityType);
    if (et == EntityType::Hammer)
        return m_hammerPool.acquire(position, velocity);
    if (et == EntityType::BossFireball)
        return m_bossFireballPool.acquire(position, velocity);

    // Not a pooled type — the factory stays the single construction point.
    return EntityFactory::create(et, position);
}

void PlayingState::recycleEntity(std::unique_ptr<Entity> entity) {
    if (!entity) return;

    // The pool-tag virtual replaces the three sequential dynamic_casts that used
    // to live here. One v-table lookup instead of three RTTI walks, and a new
    // pooled type only needs a PoolTag override, not another cast block.
    switch (entity->poolTag()) {
        case Entity::PoolTag::Fireball:
            m_fireballPool.release(std::unique_ptr<Fireball>(static_cast<Fireball*>(entity.release())));
            return;
        case Entity::PoolTag::Hammer:
            m_hammerPool.release(std::unique_ptr<Hammer>(static_cast<Hammer*>(entity.release())));
            return;
        case Entity::PoolTag::BossFireball:
            m_bossFireballPool.release(
                std::unique_ptr<BossFireball>(static_cast<BossFireball*>(entity.release())));
            return;
        case Entity::PoolTag::None:
            break; // Everything else dies here, exactly as before pooling existed.
    }
}

void PlayingState::queueSpawn(std::unique_ptr<Entity> entity) {
    if (!entity) return;
    // Wired now rather than at flush time: the spawner may set velocity or read
    // the sprite size straight after this call, and a half-built entity would
    // give it the wrong answer.
    admitEntity(entity.get());
    m_pendingSpawns.push_back(std::move(entity));
}

void PlayingState::flushPendingSpawns() {
    if (m_pendingSpawns.empty()) return;
    // Moved out first: admitting an entity cannot itself spawn one today, but if
    // it ever does, that spawn lands in a fresh queue for the next flush rather
    // than growing the vector this loop is walking.
    std::vector<std::unique_ptr<Entity>> ready;
    ready.swap(m_pendingSpawns);
    m_entities.reserve(m_entities.size() + ready.size());
    for (auto& entity : ready) {
        if (entity) m_entities.push_back(std::move(entity));
    }
}

AABB PlayingState::thinkingRegion() const {
    const AABB visible = m_camera.getVisibleBounds();
    // Half a view in each direction. Two independent things have to be true of
    // this margin and both are satisfied by tying it to the live view:
    //
    //  - Nothing frozen may be DRAWN. render() culls to the visible bounds plus
    //    one tile, so any positive margin covers that; half a view leaves the
    //    whole of a screen's worth of slack on top.
    //  - Nothing frozen may become visible before it has thought again. The
    //    camera can jump — a pipe warp, a checkpoint respawn, applySnapshot()
    //    during a time rewind — and each of those lands the view somewhere new
    //    in one frame. Half a view means such a jump has to exceed a whole
    //    screen width before it could expose an entity that has not yet ticked,
    //    and even then it ticks on the very next frame.
    //
    // Concretely, at the default 1280x720 view: 20 tiles of margin horizontally,
    // 11 vertically. The level is only 22.5 tiles tall, so in practice this is a
    // horizontal gate, which is exactly the axis Endless Mode grows along.
    const float marginX = visible.width  * 0.5f;
    const float marginY = visible.height * 0.5f;
    return AABB{visible.x - marginX, visible.y - marginY,
                visible.width + marginX * 2.0f, visible.height + marginY * 2.0f};
}

bool PlayingState::freezableOffCamera(const Entity& entity) const {
    // A WHITELIST, deliberately: a category added later is safe by default and
    // has to be opted in by someone who has thought about it. Enemies and items
    // are the two a generated chunk produces by the dozen, and the three this
    // leaves out are each left out for a concrete reason:
    //
    //  - Player. Shadow Mario trails the human by up to 8 s of recorded input,
    //    which at run speed is 2400 px — 75 tiles, far past any margin this
    //    function could pick. Freezing it stalls the leash the whole mode is
    //    built on. The same applies to a rival or CPU player 2, who in versus is
    //    routinely the far one of the pair.
    //  - Projectile. Its update() is what expires it and hands it back to
    //    m_fireballPool / m_hammerPool / m_bossFireballPool. A frozen shot never
    //    expires, so it flies on forever and its pool slot never returns — a
    //    leak, not a saving.
    //  - Block. MovingPlatform, FallingPlatform and ConveyorBelt sweep a
    //    parametric path the player rides and returns to, and terrain lives in
    //    TileMap rather than in m_entities, so the whole category is a handful
    //    of near-free update()s. Nothing to win, real damage to do.
    const EntityCategory category = entity.getCategory();
    if (category != EntityCategory::Enemy && category != EntityCategory::Item) return false;

    // An entity that does not collide with tiles has nothing in the world to
    // stop it. Freezing only stops the entity's *brain*: PhysicsEngine::update()
    // still integrates whatever velocity was left in it, because gating that
    // too would need the same reasoning in a class that has no camera. So a Boo,
    // a Lakitu, a Bullet Bill or a Piranha Plant — all of which return false
    // here — would keep coasting on a stale velocity with nothing to correct it
    // and nothing to hit, and a Lakitu the player merely outran would fly off
    // for good instead of catching up. A tile-colliding enemy is bounded: it
    // walks into a wall, or falls into a pit and dies on the void plane, both of
    // which happen with or without this gate.
    if (!entity.collidesWithTiles()) return false;

    // The boss is one entity and its fight is the only thing in Endless Mode
    // that is not more of the same. The arena lock keeps it on camera anyway, so
    // this costs one pointer compare and removes a whole class of "the fight
    // stopped while I was at the edge of the arena" report.
    if (&entity == static_cast<const Entity*>(m_activeBoss)) return false;

    return true;
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

    // A flagpole has to know whether a touch would actually finish the level,
    // because the LevelComplete handler REFUSES one while a boss is alive and
    // the flagpole used to spend its single trigger on that refusal -- leaving a
    // level nobody could finish. Installed at the same single door the
    // difficulty scale uses, so an editor-placed flagpole and a level-loaded one
    // are gated identically and neither has to remember to ask.
    //
    // Deliberately the same predicate the handler tests, in one place: two
    // copies of "may the level end now" would drift, and this batch has already
    // paid for that class of duplication more than once.
    if (auto* flagpole = dynamic_cast<Flagpole*>(entity)) {
        flagpole->setCompletionGate([this]() { return levelMayComplete(); });
    }
}

bool PlayingState::levelMayComplete() const {
    // "No escape until defeated" (SPEC 6.4) applies to FINISHING the level, not
    // just to leaving the arena. A defeated boss is pruned and forgetEntity()
    // nulls this pointer, so once the fight is won the gate opens on its own.
    return !(m_activeBoss && m_activeBoss->isActive());
}

void PlayingState::wireEntityAnimations(Entity* entity) {
    // The class-to-atlas routing itself lives in EntityArtBinder, because the
    // level editor screen has to make exactly the same decision and a second
    // copy of it would drift (g-rule-22). The sheets are pushed in here rather
    // than in enter(), so an entity admitted before the atlases finished
    // loading still gets bound with whatever is available now.
    m_artBinder.setSheets(m_playerSheet.get(), m_enemySheet.get(),
                          m_itemSheet.get(), m_scenerySheet.get());
    m_artBinder.bind(entity);
}

