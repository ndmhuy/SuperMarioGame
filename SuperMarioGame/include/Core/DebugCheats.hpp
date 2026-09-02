#pragma once

#include <array>
#include <cstddef>

// The recording aids behind Debug > Cheats: immortality, invincibility, a slow
// motion dial, a clean-capture HUD switch and the rest.
//
// Why one object instead of flags on PlayingState
// -----------------------------------------------
// These answers are needed in five places that have no path to each other —
// Player::takeDamage (all ten damage sources funnel through it), Game::run's
// fixed-timestep accumulator (the time scale), PlayingState (the void rescue,
// the frozen clock, the hidden HUD), the debug console's `god` command, and the
// end screens deciding whether a run may write a high score. Public bools on
// PlayingState would have reached none of them, and a delegating getter/setter
// pair on Game for each of the nine would be exactly the pile of trivial
// accessors AGENTS.md directive 5 forbids. One owner, asked by name.
//
// Why the reader methods are named for decisions and not for flags
// ----------------------------------------------------------------
// A call site should say what it is deciding, not which switch it consulted:
// `if (cheats.rescueInsteadOfKill())` states the policy the user asked for —
// "immortal must mean no lost progress, not no consequence" — where
// `if (cheats.immortal)` would only state a bool. Which flags feed a decision
// is this class's business, so the mapping can change in one place.
//
// Nothing here has any effect unless OPTIONS > DEBUG MODE is on: every query
// goes through the armed gate below, so a cheat cannot leak into a release run
// even if a flag were somehow left set.
class DebugCheats {
public:
    enum class Cheat {
        Immortal,          // nothing may END the run: rescue instead of kill
        Invincible,        // enemies and hazards cannot damage you
        InfiniteLives,     // loseLife() does not decrement
        FreezeTimer,       // the level countdown holds
        HideHud,           // clean capture for b-roll
        Noclip,            // pass through solid tiles, gravity off
        FreeCamera,        // detach the camera from the player and pan it
        InfiniteFireballs  // lift the two-on-screen cap and the cooldown
    };
    static constexpr std::size_t COUNT = 8;

    // The declaration order above, for a panel that wants to lay them all out.
    static const std::array<Cheat, COUNT>& all();
    static const char* label(Cheat cheat);
    // One line saying what the cheat does, shown as the panel's tooltip.
    static const char* description(Cheat cheat);

    // --- What the panel and the console do -----------------------------------
    bool isOn(Cheat cheat) const;
    void set(Cheat cheat, bool on);
    void toggle(Cheat cheat) { set(cheat, !isOn(cheat)); }

    // Slow motion for demonstrating coyote time, wall jumps and ground pounds.
    // Applied to the simulation clock only — input polling and ImGui keep real
    // time, so the panel stays responsive at 0.1x.
    void setTimeScale(float scale);
    float simulationTimeScale() const;
    static constexpr float MIN_TIME_SCALE = 0.1f;
    static constexpr float MAX_TIME_SCALE = 2.0f;

    // --- What the simulation asks --------------------------------------------
    // A lethal event must put the player back on solid ground instead of ending
    // the run. Suppressing the kill on its own is worse than the death it
    // replaces: the player keeps falling forever with no way back.
    bool rescueInsteadOfKill() const { return isOn(Cheat::Immortal); }
    bool blocksDamage() const { return isOn(Cheat::Invincible); }
    bool preservesLives() const { return isOn(Cheat::InfiniteLives); }
    bool holdsLevelTimer() const { return isOn(Cheat::FreezeTimer); }
    bool hidesHud() const { return isOn(Cheat::HideHud); }
    bool passesThroughSolids() const { return isOn(Cheat::Noclip); }
    bool detachesCamera() const { return isOn(Cheat::FreeCamera); }
    bool liftsFireballCap() const { return isOn(Cheat::InfiniteFireballs); }

    // --- Lifetime -------------------------------------------------------------
    // Follows OPTIONS > DEBUG MODE. Disarming answers every query with "off"
    // without forgetting what was set, so turning debug mode back on during one
    // recording session does not lose the layout of switches.
    void arm(bool armed) { m_armed = armed; }
    bool isArmed() const { return m_armed; }

    // Switches every cheat off and puts the clock back to 1.0x, WITHOUT clearing
    // the taint. Called when a level run ends: nothing may follow the player out
    // of the level (slow motion would otherwise crawl the menus behind it),
    // while the end screens that come next still need to know the run was
    // cheated. GameStateManager runs exit() before the next state's enter(), so
    // this ordering is load-bearing, not incidental.
    void disengageAll();

    // Called when a level run begins: disengageAll() plus a clean taint. Cheats
    // are a per-take aid, never a property of a save — a run must start honest
    // even if the previous one was recorded with immortality on.
    void resetForNewRun();

    // True once anything has been switched on since the last reset — including a
    // cheat that has since been switched off again, because the run was already
    // affected by then. Read by the end screens and by the achievement manager:
    // a demo take must not write into saves/highscores.json or unlock anything.
    bool tainted() const { return m_tainted; }

private:
    std::array<bool, COUNT> m_flags{};
    float m_timeScale = 1.0f;
    bool m_armed = false;
    bool m_tainted = false;
};
