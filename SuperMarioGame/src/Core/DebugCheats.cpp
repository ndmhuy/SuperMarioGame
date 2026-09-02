#include "Core/DebugCheats.hpp"

#include <algorithm>

namespace {
std::size_t indexOf(DebugCheats::Cheat cheat) {
    return static_cast<std::size_t>(cheat);
}
} // namespace

const std::array<DebugCheats::Cheat, DebugCheats::COUNT>& DebugCheats::all() {
    static const std::array<Cheat, COUNT> kAll = {
        Cheat::Immortal, Cheat::Invincible, Cheat::InfiniteLives, Cheat::FreezeTimer,
        Cheat::HideHud, Cheat::Noclip, Cheat::FreeCamera, Cheat::InfiniteFireballs
    };
    return kAll;
}

const char* DebugCheats::label(Cheat cheat) {
    switch (cheat) {
        case Cheat::Immortal:          return "IMMORTAL";
        case Cheat::Invincible:        return "INVINCIBLE";
        case Cheat::InfiniteLives:     return "INFINITE LIVES";
        case Cheat::FreezeTimer:       return "FREEZE TIMER";
        case Cheat::HideHud:           return "HIDE HUD";
        case Cheat::Noclip:            return "NOCLIP";
        case Cheat::FreeCamera:        return "FREE CAMERA";
        case Cheat::InfiniteFireballs: return "INFINITE FIREBALLS";
    }
    return "?";
}

const char* DebugCheats::description(Cheat cheat) {
    switch (cheat) {
        case Cheat::Immortal:
            return "Nothing ends the run. A pit, a crush or the clock running out\n"
                   "puts you back on solid ground in the column you fell from -\n"
                   "lives, power-up, score and the clock are all left alone.";
        case Cheat::Invincible:
            return "Enemies and hazards cannot damage you, so the power-up form\n"
                   "survives. Separate from IMMORTAL: this one stops the hit, that\n"
                   "one stops the hit from ending the take.";
        case Cheat::InfiniteLives:
            return "Dying still costs the position and the power-up, but not a life.";
        case Cheat::FreezeTimer:
            return "The level countdown holds where it is.";
        case Cheat::HideHud:
            return "Hides the score bar, the minimap and the match line for a clean\n"
                   "capture. Collapse this window too before recording.";
        case Cheat::Noclip:
            return "Pass through solid tiles with gravity off. Jump and crouch nudge\n"
                   "you up and down; walking is unchanged.";
        case Cheat::FreeCamera:
            return "Detaches the camera from the player so a shot can be framed\n"
                   "without walking there. WASD or the arrow keys pan it; the\n"
                   "player holds still while it is on.";
        case Cheat::InfiniteFireballs:
            return "Lifts the two-fireballs-on-screen cap and the throw cooldown.";
    }
    return "";
}

bool DebugCheats::isOn(Cheat cheat) const {
    return m_armed && m_flags[indexOf(cheat)];
}

void DebugCheats::set(Cheat cheat, bool on) {
    m_flags[indexOf(cheat)] = on;
    // Taint on the way ON only. Switching a cheat back off does not un-cheat the
    // run that already happened, which is why this is never cleared here.
    if (on) m_tainted = true;
}

void DebugCheats::setTimeScale(float scale) {
    m_timeScale = std::clamp(scale, MIN_TIME_SCALE, MAX_TIME_SCALE);
    if (m_timeScale != 1.0f) m_tainted = true;
}

float DebugCheats::simulationTimeScale() const {
    return m_armed ? m_timeScale : 1.0f;
}

void DebugCheats::disengageAll() {
    m_flags.fill(false);
    m_timeScale = 1.0f;
}

void DebugCheats::resetForNewRun() {
    disengageAll();
    m_tainted = false;
}
