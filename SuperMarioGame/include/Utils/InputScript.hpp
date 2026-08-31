#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <deque>
#include <string>
#include <vector>

// Plays a text file of input back into the game as if it were typed.
//
// Why this exists
// ---------------
// AGENTS.md directive 10 says a gameplay change is not verified until the game
// has been run and the change observed. The multiplayer modes sit several
// keypresses deep in the menu, so "run it" used to mean either playing by hand
// or driving the OS keyboard from a script — and the latter hijacks the whole
// machine's focus, typing into whatever window happens to be in front. That is
// not an acceptable price for a verification pass.
//
// A script is an *independent* input stream: the events it produces are
// synthesised inside this process and pushed through exactly the path
// sf::Window::pollEvent's events take, so the real InputManager, the real
// command objects and the real states all see them. Nothing outside this process
// is touched, and the user's keyboard and focus are left alone.
//
// This is a development and verification tool, not a game feature. It is inert
// unless `--script <file>` is passed.
//
// Grammar — one command per line, `#` starts a comment:
//
//   wait <seconds>          let the game run untouched
//   press <key>             tap: press and release on the same frame
//   hold <key> <seconds>    press now, release after the given time
//   release <key>           release a key held by `hold`
//   shot <name>             save a PNG of the window to saves/shots/<name>.png
//   savereplay <name>       save ReplayRecorder's current frames to
//                           saves/replays/<name>.json (F5 attract mode's
//                           bundled demo is produced this way — see
//                           tests/scripts/record_attract_demo.txt — rather
//                           than through the debug console, whose ImGui text
//                           field this harness cannot type into: scripted
//                           events never reach ImGui::SFML::ProcessEvent, only
//                           InputManager and the state stack (Game.cpp))
//   quit                    close the game
//
// Key names are InputManager's own (`W`, `Space`, `Left`, `Enter`, ...) plus
// `Escape` and `Grave`, so a script names controls the same way config.json does.
class InputScript {
public:
    // What the host loop has to do for a step, beyond delivering events.
    enum class Effect { None, Screenshot, SaveReplay, Quit };

    // Parses `path`. Returns false and leaves this object inactive if the file
    // cannot be read or contains a line it cannot parse — a script that silently
    // skips a bad line would "verify" something other than what it says.
    bool load(const std::string& path);

    bool active() const { return m_active; }
    bool finished() const { return m_active && m_steps.empty() && m_pendingReleases.empty(); }

    // Advance by `dt`, appending any events that came due to `out`. The returned
    // effect is what the caller must do this frame; `shotName` names the file
    // when the effect is Screenshot or SaveReplay.
    Effect update(float dt, std::vector<sf::Event>& out, std::string& shotName);

private:
    struct Step {
        enum class Kind { Wait, Press, Hold, Release, Shot, SaveReplay, Quit } kind = Kind::Wait;
        float seconds = 0.0f;
        sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
        std::string text;   // Shot / SaveReplay: the file name
    };

    // A key held by `hold`, and how long until it is released.
    struct PendingRelease {
        sf::Keyboard::Key key;
        float remaining;
    };

    static bool parseKey(const std::string& name, sf::Keyboard::Key& out);

    bool m_active = false;
    std::deque<Step> m_steps;
    std::vector<PendingRelease> m_pendingReleases;
    // Time left on the step at the front of the queue.
    float m_waiting = 0.0f;
};
