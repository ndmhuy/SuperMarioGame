#include "Utils/InputScript.hpp"

#include "Core/InputManager.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

// Builds the synthetic event. SFML 3's sf::Event is a variant over the concrete
// event structs, so a KeyPressed is constructed rather than assigned into a tag.
struct Modifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool system = false;
};

sf::Event keyEvent(sf::Keyboard::Key key, bool pressed, Modifiers mods = {}) {
    if (pressed) {
        sf::Event::KeyPressed event;
        event.code = key;
        event.scancode = sf::Keyboard::Scancode::Unknown;
        event.alt = mods.alt;
        event.control = mods.ctrl;
        event.shift = mods.shift;
        event.system = mods.system;
        return sf::Event(event);
    }
    sf::Event::KeyReleased event;
    event.code = key;
    event.scancode = sf::Keyboard::Scancode::Unknown;
    event.alt = mods.alt;
    event.control = mods.ctrl;
    event.shift = mods.shift;
    event.system = mods.system;
    return sf::Event(event);
}

} // namespace

bool InputScript::parseKey(const std::string& name, sf::Keyboard::Key& out) {
    // InputManager's table first, so a script names controls exactly as
    // config.json does and the two cannot drift apart.
    if (InputManager::parseKeyName(name, out)) return true;

    // The few keys the bindable table deliberately excludes but a script driving
    // menus needs.
    if (name == "Escape")    { out = sf::Keyboard::Key::Escape;    return true; }
    if (name == "Backspace") { out = sf::Keyboard::Key::Backspace; return true; }
    if (name == "Tab")       { out = sf::Keyboard::Key::Tab;       return true; }
    if (name == "Grave")     { out = sf::Keyboard::Key::Grave;     return true; }
    // F1 toggles the level editor, whose free camera is the only way to look at
    // a part of a level the player has not walked to — which is exactly what a
    // verification script needs to photograph the end of a stage.
    if (name == "F1")        { out = sf::Keyboard::Key::F1;        return true; }
    // F5 is attract mode from the menu and Playtest from the level editor.
    if (name == "F5")        { out = sf::Keyboard::Key::F5;        return true; }
    return false;
}

bool InputScript::parseButton(const std::string& name, sf::Mouse::Button& out) {
    if (name == "left")   { out = sf::Mouse::Button::Left;   return true; }
    if (name == "right")  { out = sf::Mouse::Button::Right;  return true; }
    if (name == "middle") { out = sf::Mouse::Button::Middle; return true; }
    return false;
}

bool InputScript::buttonDown(sf::Mouse::Button button) const {
    const auto index = static_cast<std::size_t>(button);
    return index < m_buttons.size() && m_buttons[index];
}

bool InputScript::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[InputScript] Cannot open '" << path << "'." << std::endl;
        return false;
    }

    std::deque<Step> steps;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        // Strip comments and surrounding whitespace.
        const std::size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream tokens(line);
        std::string verb;
        if (!(tokens >> verb)) continue;   // blank line

        auto fail = [&](const std::string& why) {
            std::cerr << "[InputScript] " << path << ":" << lineNumber << ": " << why
                      << std::endl;
            return false;
        };

        Step step;
        if (verb == "wait") {
            step.kind = Step::Kind::Wait;
            if (!(tokens >> step.seconds)) return fail("wait needs a duration");
        } else if (verb == "press" || verb == "hold" || verb == "release") {
            std::string keyName;
            if (!(tokens >> keyName)) return fail(verb + " needs a key");
            if (!parseKey(keyName, step.key)) return fail("unknown key '" + keyName + "'");
            if (verb == "press")   step.kind = Step::Kind::Press;
            if (verb == "release") step.kind = Step::Kind::Release;
            if (verb == "hold") {
                step.kind = Step::Kind::Hold;
                if (!(tokens >> step.seconds)) return fail("hold needs a duration");
            } else {
                // Trailing modifier words, for the editor's Ctrl+Z / Ctrl+S.
                std::string modifier;
                while (tokens >> modifier) {
                    if      (modifier == "ctrl")   step.ctrl = true;
                    else if (modifier == "shift")  step.shift = true;
                    else if (modifier == "alt")    step.alt = true;
                    else if (modifier == "system") step.system = true;
                    else return fail("unknown modifier '" + modifier + "'");
                }
            }
        } else if (verb == "mouse") {
            step.kind = Step::Kind::MouseMove;
            if (!(tokens >> step.pixel.x >> step.pixel.y)) return fail("mouse needs x and y");
        } else if (verb == "mousedown" || verb == "mouseup") {
            std::string buttonName;
            if (!(tokens >> buttonName)) return fail(verb + " needs left/right/middle");
            if (!parseButton(buttonName, step.button)) {
                return fail("unknown mouse button '" + buttonName + "'");
            }
            step.kind = (verb == "mousedown") ? Step::Kind::MouseDown : Step::Kind::MouseUp;
        } else if (verb == "shot") {
            step.kind = Step::Kind::Shot;
            if (!(tokens >> step.text)) return fail("shot needs a name");
        } else if (verb == "savereplay") {
            step.kind = Step::Kind::SaveReplay;
            if (!(tokens >> step.text)) return fail("savereplay needs a name");
        } else if (verb == "quit") {
            step.kind = Step::Kind::Quit;
        } else {
            return fail("unknown command '" + verb + "'");
        }
        steps.push_back(step);
    }

    m_steps = std::move(steps);
    m_active = true;
    std::cout << "[InputScript] Loaded " << m_steps.size() << " steps from " << path
              << std::endl;
    return true;
}

InputScript::Effect InputScript::update(float dt, std::vector<sf::Event>& out,
                                        std::string& shotName) {
    if (!m_active) return Effect::None;

    // Releases owed from `hold` come first: a key's release must land even if the
    // script has since moved on to other steps.
    for (auto& pending : m_pendingReleases) {
        pending.remaining -= dt;
        if (pending.remaining <= 0.0f) {
            out.push_back(keyEvent(pending.key, false));
        }
    }
    m_pendingReleases.erase(
        std::remove_if(m_pendingReleases.begin(), m_pendingReleases.end(),
                       [](const PendingRelease& p) { return p.remaining <= 0.0f; }),
        m_pendingReleases.end());

    // A `wait` in progress blocks the queue but not the releases above.
    if (m_waiting > 0.0f) {
        m_waiting -= dt;
        return Effect::None;
    }

    // Drain every step that is not itself a wait, so a run of presses lands on
    // one frame rather than one per frame.
    while (!m_steps.empty()) {
        const Step step = m_steps.front();
        m_steps.pop_front();

        switch (step.kind) {
            case Step::Kind::Wait:
                m_waiting = step.seconds;
                return Effect::None;
            case Step::Kind::Press: {
                const Modifiers mods{step.ctrl, step.shift, step.alt, step.system};
                out.push_back(keyEvent(step.key, true, mods));
                out.push_back(keyEvent(step.key, false, mods));
                break;
            }
            case Step::Kind::Hold:
                out.push_back(keyEvent(step.key, true));
                m_pendingReleases.push_back({step.key, step.seconds});
                break;
            case Step::Kind::Release:
                out.push_back(keyEvent(step.key, false));
                m_pendingReleases.erase(
                    std::remove_if(m_pendingReleases.begin(), m_pendingReleases.end(),
                                   [&step](const PendingRelease& p) { return p.key == step.key; }),
                    m_pendingReleases.end());
                break;
            case Step::Kind::Shot:
                shotName = step.text;
                return Effect::Screenshot;
            case Step::Kind::SaveReplay:
                shotName = step.text;
                return Effect::SaveReplay;
            case Step::Kind::MouseMove:
                m_pointer = step.pixel;
                m_pointerSet = true;
                break;
            case Step::Kind::MouseDown:
                m_buttons[static_cast<std::size_t>(step.button)] = true;
                m_pointerSet = true;
                break;
            case Step::Kind::MouseUp:
                m_buttons[static_cast<std::size_t>(step.button)] = false;
                m_pointerSet = true;
                break;
            case Step::Kind::Quit:
                return Effect::Quit;
        }
    }
    return Effect::None;
}
