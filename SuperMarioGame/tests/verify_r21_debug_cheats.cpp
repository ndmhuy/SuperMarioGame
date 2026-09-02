// verify_r21_debug_cheats.cpp — R21: the Debug > Cheats recording aids.
//
// The request was for toggles a person can flip while recording an instruction
// video — immortality, invincibility, infinite lives, a frozen clock, slow
// motion, a hidden HUD, a free camera. The design constraint came from the user
// correcting the first attempt at immortality:
//
//     "hmm but if void don't kill, then it will fall forever, so a respawn
//      might not be that bad"
//
// Suppressing the kill on its own is WORSE than the death it replaces: the
// player drops through the floor and keeps falling with no way back. So IMMORTAL
// rescues instead of killing, and the centre of this harness is proving that on
// a live PlayingState — the player walks off into a pit, crosses the void plane,
// and comes back standing on the ground of the column they fell from with their
// life count untouched.
//
// Run via:  ctest -R r21_debug_cheats --output-on-failure
//
// Reachability note: DebugCheats is owned by Game, armed by OPTIONS > DEBUG
// MODE, read by Player::takeDamage/loseLife, PlayingState::killPlayer and
// Game::run's accumulator, and written by DevPanel's "Debug > Cheats" window and
// the F2-F10 shortcuts in PlayingState::handleInput. This harness drives the
// same PlayingState the game constructs, not a mock.

#include "Core/AchievementManager.hpp"
#include "Core/DebugCheats.hpp"
#include "Core/EventBus.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/Game.hpp"
#include "Core/PlayingState.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"

#include <imgui.h>

#include <cmath>
#include <iostream>
#include <string>

#include "TestSaveSandbox.hpp"

// Reaches PlayingState's private level state. Declared a friend there for the
// same reason LevelCompletionCameraTestHooks is: the void plane, the level
// clock and rescueDestination() are not worth public getters that only a test
// would ever call.
class DebugCheatsTestHooks {
public:
    static Player* player(const PlayingState& state) { return state.m_player; }
    static float voidPlaneY(const PlayingState& state) { return state.m_voidPlaneY; }
    static float levelTimer(const PlayingState& state) { return state.m_levelTimer; }
    static bool deathInProgress(const PlayingState& state) { return state.anyDeathInProgress(); }
    static sf::Vector2f cameraPosition(const PlayingState& state) {
        return state.m_camera.getPosition();
    }
    static sf::Vector2f rescueDestination(const PlayingState& state) {
        return state.rescueDestination(state.m_player);
    }
    static sf::Vector2f levelSpawn(const PlayingState& state) { return state.m_levelSpawnPoint; }
    static void setCheckpoint(PlayingState& state, sf::Vector2f at) {
        state.m_checkpointPosition = at;
        state.m_hasCheckpoint = true;
    }
    // Digs a genuine bottomless pit: no solid tile anywhere in the column, which
    // is the case rescueDestination() has to fall back out of.
    static void carveColumn(PlayingState& state, int gx) {
        for (int y = 0; y < state.m_tileMap.getHeight(); ++y) {
            state.m_tileMap.setTile(gx, y, TileType::Empty);
        }
    }
    static float floorTopIn(const PlayingState& state, float worldX) {
        return state.floorBelow(worldX, 0.0f);
    }
};

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!condition) ++g_failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

DebugCheats& cheats() { return Game::getInstance().debugCheats(); }

// Runs the level's own update loop at the game's fixed timestep.
void tick(PlayingState& state, int frames) {
    for (int i = 0; i < frames; ++i) state.update(Constants::FIXED_TIMESTEP);
}

// ---------------------------------------------------------------------------

void testTheArmedGate() {
    section("cheats do nothing at all unless OPTIONS > DEBUG MODE armed them");

    cheats().arm(false);
    cheats().resetForNewRun();
    cheats().set(DebugCheats::Cheat::Immortal, true);
    check(!cheats().isOn(DebugCheats::Cheat::Immortal),
          "a flag set while disarmed still answers off");
    check(!cheats().rescueInsteadOfKill(),
          "and the decision the simulation asks about answers off too");
    cheats().setTimeScale(0.25f);
    check(cheats().simulationTimeScale() == 1.0f,
          "the simulation clock stays at 1x while disarmed");

    cheats().arm(true);
    check(cheats().isOn(DebugCheats::Cheat::Immortal),
          "arming reveals what was already set rather than forgetting it");
    check(std::abs(cheats().simulationTimeScale() - 0.25f) < 0.001f,
          "and the time scale with it");

    // Game::setDebugMode is the only thing that arms them in the real game.
    Game::getInstance().setDebugMode(false);
    check(!cheats().isArmed(),
          "turning DEBUG MODE off in Options disarms the cheats with it");
    Game::getInstance().setDebugMode(true);
    check(cheats().isArmed(), "and turning it back on re-arms them");

    cheats().resetForNewRun();
}

void testTimeScaleClampAndTaint() {
    section("time scale clamps to the slider's range; any cheat taints the run");

    cheats().arm(true);
    cheats().resetForNewRun();
    check(!cheats().tainted(), "a fresh run is untainted");

    cheats().setTimeScale(9.0f);
    check(cheats().simulationTimeScale() == DebugCheats::MAX_TIME_SCALE,
          "a time scale above the slider's range clamps to 2.0x");
    cheats().setTimeScale(0.0f);
    check(cheats().simulationTimeScale() == DebugCheats::MIN_TIME_SCALE,
          "and below it clamps to 0.1x rather than freezing the simulation");
    check(cheats().tainted(), "changing the clock taints the run");

    cheats().resetForNewRun();
    cheats().set(DebugCheats::Cheat::HideHud, true);
    cheats().set(DebugCheats::Cheat::HideHud, false);
    check(cheats().tainted(),
          "switching a cheat back off does not un-cheat the run it already affected");

    // Leaving a level disengages everything but REMEMBERS that it happened:
    // GameStateManager runs PlayingState::exit() before GameOverState::enter(),
    // which is where the high-score refusal reads this.
    cheats().set(DebugCheats::Cheat::Immortal, true);
    cheats().disengageAll();
    check(!cheats().isOn(DebugCheats::Cheat::Immortal),
          "leaving the level switches every cheat off");
    check(cheats().simulationTimeScale() == 1.0f, "and puts the clock back to 1x");
    check(cheats().tainted(), "but the end screens can still see the run was cheated");

    cheats().resetForNewRun();
    check(!cheats().tainted(), "starting a new run clears the record");
}

void testCheatsCannotEarnAnything() {
    section("a cheated run writes no achievement");

    AchievementManager& achievements = AchievementManager::getInstance();
    achievements.init();
    achievements.reset();

    cheats().arm(true);
    cheats().resetForNewRun();
    cheats().set(DebugCheats::Cheat::InfiniteLives, true);

    achievements.unlockAchievement("first_stomp");
    check(!achievements.isUnlocked("first_stomp"),
          "unlockAchievement is a no-op while the run is tainted");

    // The lifetime counters go the same way. They are persisted to the profile
    // and shown on the Game Over panel; a demo take must not inflate them —
    // including totalDeaths, which a lethal hit under IMMORTAL still publishes
    // on its way to being rescued.
    StatisticsTracker& stats = StatisticsTracker::getInstance();
    stats.init();
    stats.reset();
    EventBus::getInstance().publish({EventType::CoinCollected, 10});
    EventBus::getInstance().publish({EventType::PlayerDied, 0});
    check(stats.getStats().totalCoinsCollected == 0 && stats.getStats().totalDeaths == 0,
          "lifetime statistics ignore a tainted run");

    cheats().resetForNewRun();
    achievements.unlockAchievement("first_stomp");
    check(achievements.isUnlocked("first_stomp"),
          "and works again on an honest run, so the gate is the taint and not a break");
    EventBus::getInstance().publish({EventType::CoinCollected, 10});
    check(stats.getStats().totalCoinsCollected == 10,
          "and so do the statistics, so that gate is the taint and not a break either");

    achievements.reset();
    stats.reset();
}

void testPlayerLevelCheats() {
    section("INVINCIBLE, INFINITE LIVES, NOCLIP and INFINITE FIREBALLS on the Player");

    cheats().arm(true);
    cheats().resetForNewRun();

    Mario mario({100.0f, 100.0f});
    mario.setStartingForm(Player::Form::Super);

    // INVINCIBLE guards the one door all ten damage sources come through.
    cheats().set(DebugCheats::Cheat::Invincible, true);
    mario.takeDamage(1);
    check(mario.getForm() == Player::Form::Super,
          "INVINCIBLE: a hit does not step the power-up form down");
    check(mario.getInvincibilityTimer() <= 0.0f,
          "and it does it without borrowing the hurt-frames timer the way `god` used to");
    cheats().set(DebugCheats::Cheat::Invincible, false);
    mario.takeDamage(1);
    check(mario.getForm() == Player::Form::Small,
          "with it off, the same hit steps down as always");

    // INFINITE LIVES leaves the count exactly where the HUD is showing it.
    mario.restoreStats(3, 0, 0);
    cheats().set(DebugCheats::Cheat::InfiniteLives, true);
    mario.loseLife();
    check(mario.getLives() == 3, "INFINITE LIVES: loseLife does not decrement");
    cheats().set(DebugCheats::Cheat::InfiniteLives, false);
    mario.loseLife();
    check(mario.getLives() == 2, "and with it off a life is spent again");

    // NOCLIP has to reach every character, not only Mario: Luigi and Peach
    // override getGravityMultiplier and would otherwise keep falling.
    Luigi luigi({100.0f, 100.0f});
    check(mario.collidesWithTiles() && luigi.collidesWithTiles(),
          "tiles are solid to both characters by default");
    check(luigi.getGravityMultiplier() > 0.0f, "and Luigi's own gravity modifier still applies");
    cheats().set(DebugCheats::Cheat::Noclip, true);
    check(!mario.collidesWithTiles(), "NOCLIP: Mario passes through solid tiles");
    check(!luigi.collidesWithTiles(), "and so does Luigi");
    check(mario.getGravityMultiplier() == 0.0f, "gravity is off for Mario");
    check(luigi.getGravityMultiplier() == 0.0f,
          "and off for Luigi too, whose override chains to Player's");
    cheats().set(DebugCheats::Cheat::Noclip, false);
    check(luigi.getGravityMultiplier() > 0.0f, "and Luigi falls normally again afterwards");

    // INFINITE FIREBALLS lifts the throw cooldown. (PlayingState's handler owns
    // the other half — the two-on-screen cap — which the pit test below runs
    // through the real level.)
    mario.setForm(Player::Form::Fire);
    check(mario.canShootFireball(), "a Fire player can throw");
    mario.shootFireball();
    check(!mario.canShootFireball(), "and is then on cooldown");
    cheats().set(DebugCheats::Cheat::InfiniteFireballs, true);
    check(mario.canShootFireball(), "INFINITE FIREBALLS lifts the cooldown");

    cheats().resetForNewRun();
    Game::getInstance().setPlayer(nullptr);
}

// The one the user's correction is about.
void testImmortalRescuesOutOfAPitInsteadOfKilling() {
    section("IMMORTAL: falling into the void rescues instead of killing");

    cheats().arm(true);

    // A real campaign level, entered exactly as the game enters it.
    PlayingState level(false, false, MapGeneratorConfig(), 0, 0);
    level.enter();

    Player* player = DebugCheatsTestHooks::player(level);
    check(player != nullptr, "level 1 has an active player");
    if (!player) { level.exit(); return; }

    const float voidY = DebugCheatsTestHooks::voidPlaneY(level);
    const sf::Vector2f startedAt = player->getPosition();

    // --- Control: without the cheat, the same fall costs a life ---------------
    cheats().resetForNewRun();
    player->restoreStats(5, 0, 0);
    player->setPosition({startedAt.x + 320.0f, voidY + 64.0f});
    tick(level, 1);
    check(DebugCheatsTestHooks::deathInProgress(level),
          "control: crossing the void plane normally starts the death sequence");

    // Let the death fall run out so the state is clean for the real case.
    tick(level, 130);
    check(player->getLives() == 4, "control: and it costs exactly one life");

    // --- IMMORTAL: the same fall, rescued ------------------------------------
    cheats().set(DebugCheats::Cheat::Immortal, true);
    player->restoreStats(4, 0, 0);
    player->setForm(Player::Form::Fire);

    const float fellFromX = startedAt.x + 320.0f;
    const float expectedFloor = DebugCheatsTestHooks::floorTopIn(level, fellFromX + 16.0f);
    check(expectedFloor >= 0.0f, "the column being fallen from does have ground in it");

    player->setPosition({fellFromX, voidY + 64.0f});
    player->setVelocity({0.0f, 900.0f});
    tick(level, 1);

    check(!DebugCheatsTestHooks::deathInProgress(level),
          "IMMORTAL: no death sequence starts");
    check(player->getLives() == 4, "lives are untouched");
    check(player->getForm() == Player::Form::Fire, "the power-up form survives");
    check(player->getPosition().y < voidY,
          "the player is back above the void plane rather than still falling");
    check(std::abs(player->getPosition().x - fellFromX) < Constants::TILE_SIZE,
          "and back in the COLUMN they fell from, keeping their place in the level");
    check(std::abs(player->getPosition().y + player->getBoundingBox().height - expectedFloor)
              < 2.0f,
          "standing on that column's ground, feet on the surface");
    check(player->getVelocity().y == 0.0f && player->getVelocity().x == 0.0f,
          "with the fall's velocity zeroed rather than carried into the landing");

    // And it keeps working: a rescue must be repeatable for a whole take.
    player->setPosition({fellFromX, voidY + 64.0f});
    tick(level, 1);
    check(player->getLives() == 4 && !DebugCheatsTestHooks::deathInProgress(level),
          "a second fall is rescued the same way");

    // --- A genuine bottomless pit falls back to the checkpoint ---------------
    const int emptyColumn = static_cast<int>(fellFromX / Constants::TILE_SIZE) + 4;
    DebugCheatsTestHooks::carveColumn(level, emptyColumn);
    const sf::Vector2f checkpoint = DebugCheatsTestHooks::levelSpawn(level);
    DebugCheatsTestHooks::setCheckpoint(level, checkpoint);

    player->setPosition({emptyColumn * Constants::TILE_SIZE, voidY + 64.0f});
    const sf::Vector2f destination = DebugCheatsTestHooks::rescueDestination(level);
    check(std::abs(destination.x - checkpoint.x) < 1.0f &&
          std::abs(destination.y - checkpoint.y) < 1.0f,
          "a column with no ground anywhere in it falls back to the last checkpoint");

    tick(level, 1);
    check(player->getLives() == 4,
          "and a bottomless pit still costs no life");

    // --- FREEZE TIMER --------------------------------------------------------
    const float before = DebugCheatsTestHooks::levelTimer(level);
    cheats().set(DebugCheats::Cheat::FreezeTimer, true);
    tick(level, 30);
    check(DebugCheatsTestHooks::levelTimer(level) == before,
          "FREEZE TIMER holds the level clock through half a second of updates");
    cheats().set(DebugCheats::Cheat::FreezeTimer, false);
    tick(level, 30);
    check(DebugCheatsTestHooks::levelTimer(level) < before,
          "and it runs again once the switch is off");

    // --- FREE CAMERA ---------------------------------------------------------
    cheats().set(DebugCheats::Cheat::FreeCamera, true);
    tick(level, 2);
    const sf::Vector2f parked = DebugCheatsTestHooks::cameraPosition(level);
    player->setPosition({checkpoint.x + 1200.0f, checkpoint.y});
    tick(level, 20);
    check(DebugCheatsTestHooks::cameraPosition(level) == parked,
          "FREE CAMERA: the camera stays where it was framed while the player moves away");
    cheats().set(DebugCheats::Cheat::FreeCamera, false);
    tick(level, 20);
    check(DebugCheatsTestHooks::cameraPosition(level) != parked,
          "and starts following again when it is switched off");

    level.exit();
    check(!cheats().isOn(DebugCheats::Cheat::Immortal),
          "leaving the level switches the cheats off so none of them follows the player out");

    cheats().resetForNewRun();
    cheats().arm(false);
    Game::getInstance().setDebugMode(false);
}

} // namespace

int main() {
    TestSaveSandbox sandbox("r21_debug_cheats");

    std::cout << "R21 — Debug > Cheats (recording aids)\n";
    std::cout << "----------------------------------------\n";

    // PlayingState::update() asks ImGui::GetIO() whether a text field has the
    // keyboard before it hands the keys to the player, and ImGui asserts on a
    // missing context. A bare context is enough — nothing here calls NewFrame or
    // draws, only reads io flags — and it needs no window or GL, which is what
    // lets this harness drive a real level headlessly.
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Constants::WINDOW_WIDTH),
                                        static_cast<float>(Constants::WINDOW_HEIGHT));

    testTheArmedGate();
    testTimeScaleClampAndTaint();
    testCheatsCannotEarnAnything();
    testPlayerLevelCheats();
    testImmortalRescuesOutOfAPitInsteadOfKilling();

    SoundManager::getInstance().shutdown();
    ResourceManager::getInstance().clear();
    ImGui::DestroyContext();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
