// A flagpole touched during a boss fight used to leave the level unfinishable.
//
// Reported by a teammate after a Boom Boom fight: defeat him, walk away, come
// back to the flag later, and the level never ends. The reporter could not
// reproduce it on their own machine, which is the tell -- it depends on whether
// you happen to brush the pole while the fight is still on.
//
// The mechanism:
//   1. Flagpole::onPlayerCollision latched m_triggered, awarded the points and
//      published LevelComplete on the first touch, unconditionally.
//   2. PlayingState's handler REFUSES that completion while a boss is alive
//      ("no escape until defeated", SPEC 6.4).
//   3. m_triggered stayed set. The flagpole was spent. Every later touch hit
//      `if (m_triggered) return;` and published nothing, so the level could
//      never be completed -- a soft lock, with the boss already dead.
//
// Why it was easy to hit and easy to miss: level_2's Boom Boom arena is
// arenaX 176, arenaW 16 (tiles 176-192) and its flagpole stands at tile 193 --
// ONE tile past the clamp the player is pressed against during the fight. Fight
// from the left and you never touch it; get pushed right and the level is
// already unwinnable.
//
// The fix is that a refused touch leaves no trace at all: no latch, no score,
// no jingle. PlayingState installs the decision on every Flagpole through
// admitEntity, and both it and the LevelComplete handler ask the same
// levelMayComplete() so the rule exists in one place.
//
// These cases drive Flagpole directly rather than a whole PlayingState: the
// defect lives entirely in the latch-versus-gate ordering, and a bare flagpole
// with an installed gate reproduces it exactly while staying fast and headless.
#include "Core/EventBus.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Mario.hpp"
#include "Utils/Constants.hpp"
#include "TestSaveSandbox.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << std::endl;
    if (!condition) ++failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

// Counts LevelComplete publishes for the duration of one case.
class CompletionCounter {
public:
    CompletionCounter() {
        m_sub = EventBus::ScopedSubscription(
            EventType::LevelComplete, [this](const GameEvent&) { ++m_count; });
    }
    int count() const { return m_count; }

private:
    int m_count = 0;
    EventBus::ScopedSubscription m_sub;
};

// A flagpole seated the way level_2's is, and a player touching it mid-pole.
struct Rig {
    Flagpole pole{sf::Vector2f{193.0f * Constants::TILE_SIZE, 15.0f * Constants::TILE_SIZE}};
    Mario player{sf::Vector2f{193.0f * Constants::TILE_SIZE, 18.0f * Constants::TILE_SIZE}};

    void touch() {
        // Mid-pole, so the catch would score if it were allowed to.
        pole.onPlayerCollision(player, pole.getPosition().y + pole.getPoleHeight() * 0.5f);
    }
};

// THE REGRESSION. A refused touch must leave the flagpole exactly as it was.
void testRefusedTouchDoesNotSpendTheFlagpole() {
    section("a touch refused mid-fight leaves the flagpole usable");

    Rig rig;
    bool bossAlive = true;
    rig.pole.setCompletionGate([&bossAlive]() { return !bossAlive; });

    CompletionCounter counter;
    const int scoreBefore = rig.player.getScore();

    rig.touch();
    check(counter.count() == 0, "the refused touch publishes no LevelComplete");
    check(!rig.pole.isTriggered(), "and does NOT latch the flagpole");
    check(rig.player.getScore() == scoreBefore,
          "and awards no points for a completion that did not happen");

    // The fight is won. The gate opens exactly as forgetEntity() nulling
    // m_activeBoss opens it in the real game.
    bossAlive = false;

    rig.touch();
    check(counter.count() == 1, "the later touch finishes the level");
    check(rig.pole.isTriggered(), "and the flagpole latches this time");
    check(rig.player.getScore() > scoreBefore, "and pays the catch-height score");
}

// The latch still has to do its original job.
void testAnAllowedTouchStillFiresExactlyOnce() {
    section("an allowed flagpole still fires exactly once");

    Rig rig;
    CompletionCounter counter;   // no gate installed: always allowed

    rig.touch();
    rig.touch();
    rig.touch();
    check(counter.count() == 1, "three touches publish one LevelComplete");
    check(rig.pole.isTriggered(), "and the flagpole stays latched");
}

// An ungated flagpole -- a bare one in the editor or a test -- must work.
void testNoGateMeansAlwaysAllowed() {
    section("a flagpole with no gate installed completes normally");

    Rig rig;
    check(rig.pole.canCompleteNow(), "an ungated flagpole reports it may complete");

    CompletionCounter counter;
    rig.touch();
    check(counter.count() == 1, "and it does");
}

// The gate is asked every touch, not cached at install time.
void testTheGateIsConsultedEveryTouch() {
    section("the gate is asked on each touch rather than remembered");

    Rig rig;
    int asked = 0;
    bool allowed = false;
    rig.pole.setCompletionGate([&asked, &allowed]() { ++asked; return allowed; });

    CompletionCounter counter;
    rig.touch();
    rig.touch();
    check(asked == 2, "two refused touches asked the gate twice");
    check(counter.count() == 0, "and neither completed");

    allowed = true;
    rig.touch();
    check(asked == 3, "the third touch asked again");
    check(counter.count() == 1, "and completed");
}

}  // namespace

int main() {
    // Nothing here writes a save, but the harness convention is to seal the
    // process off regardless: a case that reaches a save path by accident must
    // not find the developer's real saves/ (g-rule-13).
    TestSaveSandbox sandbox("flagpole_softlock");

    std::cout << "R21 flagpole soft-lock harness\n";

    testRefusedTouchDoesNotSpendTheFlagpole();
    testAnAllowedTouchStillFiresExactlyOnce();
    testNoGateMeansAlwaysAllowed();
    testTheGateIsConsultedEveryTouch();

    std::cout << "\n----------------------------------------\n";
    if (failures > 0) {
        std::cout << failures << " FAILURE(S)" << std::endl;
        return 1;
    }
    std::cout << "ALL PASS" << std::endl;
    return 0;
}
