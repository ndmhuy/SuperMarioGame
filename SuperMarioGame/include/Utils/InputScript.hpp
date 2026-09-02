#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <array>
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
//   press <key> [mods...]   tap: press and release on the same frame. Modifiers
//                           are `ctrl`, `shift`, `alt`, `system` and ride on the
//                           synthesised event, so `press Z ctrl` is Ctrl+Z —
//                           without them the editor's Ctrl+Z / Ctrl+S / Ctrl+O
//                           shortcuts could not be verified by a script at all
//   hold <key> <seconds>    press now, release after the given time
//   release <key>           release a key held by `hold`
//   mouse <x> <y>           move the synthetic pointer to window pixel (x, y)
//   mousedown <button>      hold left / right / middle
//   mouseup <button>        release it
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

    // --- Synthetic pointer ------------------------------------------------
    //
    // The keyboard alone cannot verify the level editor: placing a tile or an
    // entity is a click, and MapEditor reads the button state by polling rather
    // than from events (a drag needs a level, not an edge). Driving the real OS
    // pointer would hijack the machine's focus, which is exactly what this class
    // exists to avoid — so the script carries a pointer of its own and Game
    // hands it to the editor in place of sf::Mouse.
    //
    // False until a script has actually moved the pointer, so a script that
    // never mentions the mouse leaves the real one in charge.
    bool hasPointer() const { return m_pointerSet; }
    sf::Vector2i pointerPixel() const { return m_pointer; }
    bool buttonDown(sf::Mouse::Button button) const;
    bool finished() const { return m_active && m_steps.empty() && m_pendingReleases.empty(); }

    // Advance by `dt`, appending any events that came due to `out`. The returned
    // effect is what the caller must do this frame; `shotName` names the file
    // when the effect is Screenshot or SaveReplay.
    Effect update(float dt, std::vector<sf::Event>& out, std::string& shotName);

private:
    struct Step {
        enum class Kind { Wait, Press, Hold, Release, Shot, SaveReplay, Quit,
                          MouseMove, MouseDown, MouseUp } kind = Kind::Wait;
        float seconds = 0.0f;
        sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
        sf::Mouse::Button button = sf::Mouse::Button::Left;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool system = false;
        sf::Vector2i pixel{0, 0};
        std::string text;   // Shot / SaveReplay: the file name
    };

    // A key held by `hold`, and how long until it is released.
    struct PendingRelease {
        sf::Keyboard::Key key;
        float remaining;
    };

    static bool parseKey(const std::string& name, sf::Keyboard::Key& out);
    static bool parseButton(const std::string& name, sf::Mouse::Button& out);

    bool m_active = false;
    std::deque<Step> m_steps;
    std::vector<PendingRelease> m_pendingReleases;
    // Time left on the step at the front of the queue.
    float m_waiting = 0.0f;

    bool m_pointerSet = false;
    sf::Vector2i m_pointer{0, 0};
    // Indexed by sf::Mouse::Button: Left, Right, Middle, Extra1, Extra2.
    std::array<bool, 5> m_buttons{};
};
