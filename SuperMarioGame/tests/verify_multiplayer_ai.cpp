// Regression harness for the multiplayer, AI-opponent and Shadow Mario work.
//
// Each test below pins a behaviour that was either broken or absent before this
// branch, and names it. The point is not that the classes construct — six
// subsystems once shipped "complete" on exactly that basis — but that the rules
// the modes are built on hold.

#include "Core/Game.hpp"
#include "Core/GameMode.hpp"
#include "Core/GameSnapshot.hpp"
#include "Core/InputManager.hpp"
#include "Entities/AIController.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Mario.hpp"
#include "Entities/ShadowMario.hpp"
#include "Utils/TileMap.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

namespace {

constexpr float kStep = 1.0f / 60.0f;

bool nearlyEqual(float a, float b, float epsilon = 0.5f) {
    return std::abs(a - b) < epsilon;
}

// A flat floor with a solid band along the bottom, which is all most of these
// tests need underfoot.
void buildFlatLevel(TileMap& tileMap, int width = 60, int height = 22) {
    tileMap.initialize(width, height);
    for (int x = 0; x < width; ++x) {
        tileMap.setTile(x, height - 2, TileType::Ground);
        tileMap.setTile(x, height - 1, TileType::Ground);
    }
}

// --- Shadow Mario ----------------------------------------------------------

void testShadowIsAHazardNotAParticipant() {
    std::cout << "testShadowIsAHazardNotAParticipant..." << std::endl;

    ShadowMario shadow({100.0f, 100.0f});
    Mario mario({100.0f, 100.0f});

    // The collision dispatcher routes on this one virtual. If it ever returns
    // false the shadow starts stomping Goombas and eating mushrooms on the
    // player's behalf, which silently trivialises the level it is chasing them
    // through.
    assert(shadow.isContactHazard() && "Shadow must report itself a contact hazard");
    assert(!mario.isContactHazard() && "An ordinary player must not");

    // Nothing hurts a shadow. takeDamage is overridden to a no-op rather than
    // guarded at every call site.
    const int livesBefore = shadow.getLives();
    shadow.takeDamage(1);
    shadow.takeDamage(99);
    assert(shadow.getLives() == livesBefore && "Shadow must be immune to damage");

    std::cout << "  ok" << std::endl;
}

void testShadowReplaysAfterItsDelay() {
    std::cout << "testShadowReplaysAfterItsDelay..." << std::endl;

    ShadowMario shadow({100.0f, 100.0f});
    Mario mario({100.0f, 100.0f});
    shadow.setDelay(1.0f);

    // Nothing has come due yet, so the shadow has not begun to move.
    for (int frame = 0; frame < 30; ++frame) {
        shadow.recordFrame(static_cast<float>(frame) * kStep, mario);
        shadow.update(kStep);
    }
    assert(!shadow.hasStarted() && "Shadow must not move before its delay elapses");
    assert(nearlyEqual(shadow.getPosition().x, 100.0f) &&
           "Shadow must sit at its spawn until the buffer fills");

    // Walk the recorded player away, then keep feeding frames until the first
    // packets come due.
    mario.setPosition({400.0f, 100.0f});
    for (int frame = 30; frame < 150; ++frame) {
        shadow.recordFrame(static_cast<float>(frame) * kStep, mario);
        shadow.update(kStep);
    }

    assert(shadow.hasStarted() && "Shadow must start replaying once packets are due");
    // The position correction is a lerp, not a teleport, so this asserts the
    // direction of travel rather than an exact landing spot: the shadow has to
    // be visibly chasing the recorded path.
    assert(shadow.getPosition().x > 100.0f &&
           "Shadow must move towards the position that was recorded for it");

    std::cout << "  ok (shadow reached x=" << shadow.getPosition().x << ")" << std::endl;
}

void testShadowDelayIsClamped() {
    std::cout << "testShadowDelayIsClamped..." << std::endl;

    ShadowMario shadow({0.0f, 0.0f});
    // The dev-panel slider is bounded, but nothing stops a caller passing zero,
    // and a zero delay makes the shadow occupy the player's own hitbox forever.
    shadow.setDelay(0.0f);
    assert(shadow.getDelay() > 0.0f && "A zero delay must be clamped away");
    shadow.setDelay(1000.0f);
    assert(shadow.getDelay() <= 10.0f && "An absurd delay must be clamped");

    std::cout << "  ok" << std::endl;
}

// --- Nearest-player targeting ----------------------------------------------

void testEnemiesTargetTheNearerPlayer() {
    std::cout << "testEnemiesTargetTheNearerPlayer..." << std::endl;

    Mario one({100.0f, 100.0f});
    Luigi two({500.0f, 100.0f});

    Game& game = Game::getInstance();
    game.setPlayer(&one);
    game.setSecondPlayer(&two);

    // This is the fix for "every enemy in the level chased Player 1 and walked
    // straight through Player 2".
    assert(game.getNearestPlayer({110.0f, 100.0f}) == &one && "Should pick player one");
    assert(game.getNearestPlayer({480.0f, 100.0f}) == &two && "Should pick player two");

    // A dying player is falling off the bottom of the screen. Chasing it drags
    // the level's enemies off the map behind it.
    two.beginDeathFall();
    assert(game.getNearestPlayer({480.0f, 100.0f}) == &one &&
           "A dying player must not be targeted");
    two.endDeathFall();

    // And with no second player registered it must behave exactly as the old
    // getPlayer() did.
    game.setSecondPlayer(nullptr);
    assert(game.getNearestPlayer({480.0f, 100.0f}) == &one &&
           "With one player, every query resolves to it");

    game.setPlayer(nullptr);
    std::cout << "  ok" << std::endl;
}

// --- Snapshots -------------------------------------------------------------

void testSnapshotCarriesBothPlayers() {
    std::cout << "testSnapshotCarriesBothPlayers..." << std::endl;

    Mario one({100.0f, 100.0f});
    Luigi two({200.0f, 100.0f});
    one.addScore(500);
    two.addScore(1200);

    GameSnapshot snapshot;
    snapshot.playerState = one.createSnapshot();
    snapshot.hasSecondPlayer = true;
    snapshot.secondPlayerState = two.createSnapshot();

    // Player 2's stats were absent from the snapshot entirely: its position rode
    // along in entityStates because it is an Entity, so a rewind looked correct
    // while quietly rolling back one player's score and not the other's.
    assert(snapshot.hasSecondPlayer && "Snapshot must record that there were two players");
    assert(snapshot.secondPlayerState.score == 1200 && "Player 2's score must be captured");

    two.addScore(3000);
    two.restoreMemento(snapshot.secondPlayerState);
    assert(two.getScore() == 1200 && "Restoring must roll Player 2's score back too");

    // A single-player recording replayed in a two-player level must not zero the
    // second player out of a default-constructed snapshot.
    GameSnapshot soloSnapshot;
    soloSnapshot.playerState = one.createSnapshot();
    assert(!soloSnapshot.hasSecondPlayer &&
           "A single-player snapshot must say so, so applySnapshot can skip it");

    std::cout << "  ok" << std::endl;
}

// --- Heuristic policy ------------------------------------------------------

// An observation of open, flat ground with the goal to the right.
AIObservation flatGroundObservation() {
    AIObservation obs;
    obs.grid.fill(AICellState::Empty);
    const int halfW = kAIVisionWidth / 2;
    for (int dx = -halfW; dx <= halfW; ++dx) {
        const int x = dx + halfW;
        const int y = 1 + kAIVisionHeight / 2;
        obs.grid[static_cast<std::size_t>(y) * kAIVisionWidth + x] = AICellState::Solid;
    }
    obs.dxToGoal = 1.0f;
    obs.onGround = true;
    obs.canJump = true;
    return obs;
}

void setCell(AIObservation& obs, int dx, int dy, AICellState state) {
    const int x = dx + kAIVisionWidth / 2;
    const int y = dy + kAIVisionHeight / 2;
    obs.grid[static_cast<std::size_t>(y) * kAIVisionWidth + x] = state;
}

void testSpeedrunnerRunsAtTheGoal() {
    std::cout << "testSpeedrunnerRunsAtTheGoal..." << std::endl;

    HeuristicPolicy policy(AIArchetype::Speedrunner);
    const AIAction action = policy.decide(flatGroundObservation());

    assert(action.moveRight && !action.moveLeft && "Speedrunner must head for the exit");
    assert(action.run && "On clear ground a speedrunner runs");
    assert(!action.jump && "Nothing to jump over on flat ground");

    std::cout << "  ok" << std::endl;
}

void testPolicyJumpsWallsAndGaps() {
    std::cout << "testPolicyJumpsWallsAndGaps..." << std::endl;

    HeuristicPolicy policy(AIArchetype::Speedrunner);

    // A wall directly ahead at foot height.
    AIObservation wall = flatGroundObservation();
    setCell(wall, 1, 0, AICellState::Solid);
    assert(policy.decide(wall).jump && "A wall ahead must be jumped");

    // The floor stops two tiles ahead.
    policy.reset();
    AIObservation gap = flatGroundObservation();
    setCell(gap, 1, 1, AICellState::Empty);
    setCell(gap, 2, 1, AICellState::Empty);
    const AIAction overGap = policy.decide(gap);
    assert(overGap.jump && "A gap ahead must be jumped");
    assert(!overGap.run && "Running blind into a gap is how a bot kills itself");

    std::cout << "  ok" << std::endl;
}

void testArchetypesDisagree() {
    std::cout << "testArchetypesDisagree..." << std::endl;

    // The opponent is behind the bot, and a coin is behind it too. All three
    // archetypes see exactly the same world; the whole point of the weights is
    // that they do different things about it.
    AIObservation obs = flatGroundObservation();
    obs.dxToOpponent = -0.05f;   // just behind, well inside the proximity falloff
    obs.dyToOpponent = 0.0f;
    // Coin, specifically: observation v3 split Reward into Coin and PowerUp,
    // and this test is about the Collector being drawn to a collectable, which
    // a coin is the plainest example of.
    setCell(obs, -3, 0, AICellState::Coin);

    HeuristicPolicy speedrunner(AIArchetype::Speedrunner);
    HeuristicPolicy hunter(AIArchetype::Hunter);
    HeuristicPolicy collector(AIArchetype::Collector);

    const AIAction runnerAction = speedrunner.decide(obs);
    const AIAction hunterAction = hunter.decide(obs);
    const AIAction collectorAction = collector.decide(obs);

    assert(runnerAction.moveRight &&
           "A speedrunner ignores both the opponent and the coin");
    assert(hunterAction.moveLeft &&
           "A hunter turns back for a nearby opponent");
    assert(collectorAction.moveLeft &&
           "A collector turns back for a reward");

    std::cout << "  ok" << std::endl;
}

// --- AIController ----------------------------------------------------------

void testDifficultyProfilesMatchTheSpec() {
    std::cout << "testDifficultyProfilesMatchTheSpec..." << std::endl;

    Luigi bot({100.0f, 100.0f});

    AIController easy(bot, AIDifficulty::Easy, AIArchetype::Speedrunner);
    AIController normal(bot, AIDifficulty::Normal, AIArchetype::Speedrunner);
    AIController hard(bot, AIDifficulty::Hard, AIArchetype::Speedrunner);

    // The numbers from docs/two_player_ai_plan.md §2A: 400ms / 120ms / frame
    // perfect, and clumsy / suboptimal / flawless.
    assert(nearlyEqual(easy.getReactionLatency(), 0.40f, 0.001f));
    assert(nearlyEqual(normal.getReactionLatency(), 0.12f, 0.001f));
    assert(nearlyEqual(hard.getReactionLatency(), 0.0f, 0.001f));

    assert(easy.getActionNoise() > normal.getActionNoise() &&
           "Easy must be noisier than Normal");
    assert(nearlyEqual(hard.getActionNoise(), 0.0f, 0.001f) &&
           "Hard executes flawlessly");

    assert(easy.getVisionRadius() < normal.getVisionRadius() &&
           normal.getVisionRadius() < hard.getVisionRadius() &&
           "Vision must widen with difficulty");

    // Changing difficulty at runtime — the dev panel does this — must re-derive
    // the whole profile, not just the label.
    easy.setDifficulty(AIDifficulty::Hard);
    assert(nearlyEqual(easy.getReactionLatency(), 0.0f, 0.001f) &&
           "setDifficulty must re-apply the profile");

    std::cout << "  ok" << std::endl;
}

void testControllerActuallyDrivesThePlayer() {
    std::cout << "testControllerActuallyDrivesThePlayer..." << std::endl;

    TileMap tileMap;
    buildFlatLevel(tileMap);

    // Standing on the floor of the flat level.
    const float floorY = static_cast<float>(22 - 2) * 32.0f;
    Luigi bot({100.0f, floorY - 32.0f});
    bot.setGrounded(true);

    std::vector<std::unique_ptr<Entity>> entities;

    // Hard difficulty: zero latency and zero noise, so one update is one
    // decision and the assertion cannot be flaky.
    AIController controller(bot, AIDifficulty::Hard, AIArchetype::Speedrunner);

    // This is the assertion that matters. A controller that scans and decides
    // but never actuates would pass every other test in this file while the bot
    // stood still for the whole match.
    controller.update(kStep, nullptr, tileMap, entities);
    assert(bot.isMoveRightRequested() &&
           "The controller must translate its decision into player intent");
    assert(!bot.isMoveLeftRequested() && "and not both directions at once");

    // A paused controller must not steer: the pause overlay stops the world, and
    // a bot that keeps deciding banks the paused time and spends it at once.
    bot.clearMovementRequests();
    controller.setPaused(true);
    controller.update(kStep, nullptr, tileMap, entities);
    assert(!bot.isMoveRightRequested() && "A paused controller must not act");
    controller.setPaused(false);

    // A dying bot must not steer either — it is falling through the level, and
    // driving it fights the death sequence.
    bot.clearMovementRequests();
    bot.beginDeathFall();
    controller.update(kStep, nullptr, tileMap, entities);
    assert(!bot.isMoveRightRequested() && "A dying controller must not act");
    bot.endDeathFall();

    std::cout << "  ok" << std::endl;
}

void testObservationSeesTheLevel() {
    std::cout << "testObservationSeesTheLevel..." << std::endl;

    TileMap tileMap;
    buildFlatLevel(tileMap);

    const float floorY = static_cast<float>(22 - 2) * 32.0f;
    Luigi bot({320.0f, floorY - 32.0f});
    bot.setGrounded(true);
    Mario human({480.0f, floorY - 32.0f});

    std::vector<std::unique_ptr<Entity>> entities;
    AIController controller(bot, AIDifficulty::Hard, AIArchetype::Hunter);
    controller.update(kStep, &human, tileMap, entities);

    const AIObservation& obs = controller.lastObservation();

    // The tile the agent's feet are in is empty; the one below is the floor.
    assert(obs.at(0, 1) == AICellState::Solid &&
           "The floor must be sensed directly underfoot");
    assert(obs.onGround && "A grounded bot must observe itself grounded");
    // The human is to the right, so the offset must be positive — a sign error
    // here would make every Hunter run away from its target.
    assert(obs.dxToOpponent > 0.0f && "Opponent to the right must read positive");
    // Normalization is a contract with the neural policy: everything in [-1, 1].
    assert(obs.dxToOpponent <= 1.0f && obs.dxToGoal <= 1.0f &&
           obs.dxToGoal >= -1.0f && "Scalars must stay normalized");

    // Off the map reads as solid, not as unexplored: the level boundary is a
    // wall, and a bot that treats it as open space walks into it forever.
    Luigi cornered(sf::Vector2f{0.0f, floorY - 32.0f});
    cornered.setGrounded(true);
    AIController edge(cornered, AIDifficulty::Hard, AIArchetype::Speedrunner);
    edge.update(kStep, nullptr, tileMap, entities);
    assert(edge.lastObservation().at(-3, 0) == AICellState::Solid &&
           "Off-map cells must read as solid");

    std::cout << "  ok" << std::endl;
}

// --- Match configuration ---------------------------------------------------

void testMatchConfigPredicates() {
    std::cout << "testMatchConfigPredicates..." << std::endl;

    // These four predicates are what the camera, the collision resolver and the
    // HUD branch on, so getting one wrong is a silent mode mix-up rather than a
    // crash.
    MatchConfig solo;
    assert(!solo.hasSecondPlayer() && !solo.isVersus() && !solo.isCoop() &&
           !solo.isCpuOpponent() && !solo.isShadowChase());

    MatchConfig versus{GameMode::VersusHuman};
    assert(versus.hasSecondPlayer() && versus.isVersus() && !versus.isCpuOpponent());

    MatchConfig cpu{GameMode::VersusCPU};
    assert(cpu.hasSecondPlayer() && cpu.isVersus() && cpu.isCpuOpponent());

    MatchConfig coop{GameMode::CoopHuman};
    assert(coop.hasSecondPlayer() && coop.isCoop() && !coop.isVersus());

    MatchConfig shadow{GameMode::ShadowChase};
    // The shadow is deliberately not a second *player*: counting it as one would
    // put it in the versus camera's midpoint and the versus HUD's score line.
    assert(!shadow.hasSecondPlayer() && shadow.isShadowChase());

    std::cout << "  ok" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== verify_multiplayer_ai ===" << std::endl;

    testShadowIsAHazardNotAParticipant();
    testShadowReplaysAfterItsDelay();
    testShadowDelayIsClamped();
    testEnemiesTargetTheNearerPlayer();
    testSnapshotCarriesBothPlayers();
    testSpeedrunnerRunsAtTheGoal();
    testPolicyJumpsWallsAndGaps();
    testArchetypesDisagree();
    testDifficultyProfilesMatchTheSpec();
    testControllerActuallyDrivesThePlayer();
    testObservationSeesTheLevel();
    testMatchConfigPredicates();

    std::cout << "=== all multiplayer/AI checks passed ===" << std::endl;
    return 0;
}
