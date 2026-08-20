#include "Utils/InputScript.hpp"

#include "Core/InputManager.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

// Builds the synthetic event. SFML 3's sf::Event is a variant over the concrete
// event structs, so a KeyPressed is constructed rather than assigned into a tag.
sf::Event keyEvent(sf::Keyboard::Key key, bool pressed) {
    if (pressed) {
        sf::Event::KeyPressed event;
        event.code = key;
        event.scancode = sf::Keyboard::Scancode::Unknown;
        event.alt = false;
        event.control = false;
        event.shift = false;
        event.system = false;
        return sf::Event(event);
    }
    sf::Event::KeyReleased event;
    event.code = key;
    event.scancode = sf::Keyboard::Scancode::Unknown;
    event.alt = false;
    event.control = false;
    event.shift = false;
    event.system = false;
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
    // Training-speed controls, so a scripted run can wind the simulation up
    // past realtime the same way a human would.
    if (name == "Equal")     { out = sf::Keyboard::Key::Equal;     return true; }
    if (name == "Hyphen")    { out = sf::Keyboard::Key::Hyphen;    return true; }
    return false;
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
            }
        } else if (verb == "shot") {
            step.kind = Step::Kind::Shot;
            if (!(tokens >> step.text)) return fail("shot needs a name");
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
            case Step::Kind::Press:
                out.push_back(keyEvent(step.key, true));
                out.push_back(keyEvent(step.key, false));
                break;
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
            case Step::Kind::Quit:
                return Effect::Quit;
        }
    }
    return Effect::None;
}
