#include "Core/InputManager.hpp"
#include <string>
#include "Entities/Player.hpp"

// Include all commands
#include "Core/JumpCommand.hpp"
#include "Core/MoveLeftCommand.hpp"
#include "Core/MoveRightCommand.hpp"
#include "Core/FireCommand.hpp"
#include "Core/RunCommand.hpp"
#include "Core/CrouchCommand.hpp"
#include "Core/GroundPoundCommand.hpp"
#include "Core/WallJumpCommand.hpp"

#include <vector>
#include <iostream>

// Simple CompositeCommand class to execute multiple commands (e.g. Jump and WallJump on same key)
class CompositeCommand : public ICommand {
public:
    void addCommand(std::shared_ptr<ICommand> cmd) {
        m_commands.push_back(cmd);
    }

    void execute(Character& character) override {
        for (auto& cmd : m_commands) {
            cmd->execute(character);
        }
    }

private:
    std::vector<std::shared_ptr<ICommand>> m_commands;
};

InputManager::InputManager() {
    loadDefaultBindings();
}

InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

void InputManager::loadDefaultBindings() {
    // Create shared instances of commands to reuse them
    auto jumpCmd = std::make_shared<JumpCommand>();
    auto leftCmd = std::make_shared<MoveLeftCommand>();
    auto rightCmd = std::make_shared<MoveRightCommand>();
    auto fireCmd = std::make_shared<FireCommand>();
    auto runCmd = std::make_shared<RunCommand>();
    auto crouchCmd = std::make_shared<CrouchCommand>();
    auto gpCmd = std::make_shared<GroundPoundCommand>();
    auto wjCmd = std::make_shared<WallJumpCommand>();

    // Composite jump command combining regular jump and wall jump
    auto compositeJumpCmd = std::make_shared<CompositeCommand>();
    compositeJumpCmd->addCommand(jumpCmd);
    compositeJumpCmd->addCommand(wjCmd);

    // Action tables, so applyBindings() can move a command to a different key
    // without needing to know how it was constructed.
    m_commandsByAction = {
        {"jump", compositeJumpCmd}, {"fire", fireCmd}, {"groundpound", gpCmd},
        {"left", leftCmd}, {"right", rightCmd}, {"crouch", crouchCmd}, {"run", runCmd},
    };
    m_heldActions = {"left", "right", "crouch", "run"};

    // --- PLAYER 1 BINDINGS (WASD) ---
    // Press mappings (one-shot actions)
    m_pressMappings[0][sf::Keyboard::Key::W] = compositeJumpCmd;
    m_pressMappings[0][sf::Keyboard::Key::Space] = compositeJumpCmd;
    m_pressMappings[0][sf::Keyboard::Key::F] = fireCmd;
    m_pressMappings[0][sf::Keyboard::Key::Q] = gpCmd; // Press Q to ground pound (S is crouch)

    // Hold mappings (continuous actions)
    m_holdMappings[0][sf::Keyboard::Key::A] = leftCmd;
    m_holdMappings[0][sf::Keyboard::Key::D] = rightCmd;
    m_holdMappings[0][sf::Keyboard::Key::S] = crouchCmd; // Hold S to crouch
    m_holdMappings[0][sf::Keyboard::Key::LShift] = runCmd;

    // --- PLAYER 2 BINDINGS (Arrow keys) ---
    // Press mappings (one-shot actions)
    m_pressMappings[1][sf::Keyboard::Key::Up] = compositeJumpCmd;
    // RShift is Player 2's second jump key, mirroring Player 1's Space above.
    // Player 1 had two and Player 2 had one, which on a shared keyboard reads as
    // Player 2's controls being incomplete — the arrow cluster has no thumb key.
    m_pressMappings[1][sf::Keyboard::Key::RShift] = compositeJumpCmd;
    m_pressMappings[1][sf::Keyboard::Key::Period] = fireCmd;
    m_pressMappings[1][sf::Keyboard::Key::RControl] = fireCmd;
    m_pressMappings[1][sf::Keyboard::Key::Slash] = gpCmd; // Press Slash to ground pound (Down is crouch)

    // Hold mappings (continuous actions)
    m_holdMappings[1][sf::Keyboard::Key::Left] = leftCmd;
    m_holdMappings[1][sf::Keyboard::Key::Right] = rightCmd;
    m_holdMappings[1][sf::Keyboard::Key::Down] = crouchCmd; // Hold Down arrow to crouch
    m_holdMappings[1][sf::Keyboard::Key::N] = runCmd;

    // Remember which key each action landed on, per player.
    m_boundKey[0] = {{"jump", sf::Keyboard::Key::W}, {"fire", sf::Keyboard::Key::F},
                     {"groundpound", sf::Keyboard::Key::Q}, {"left", sf::Keyboard::Key::A},
                     {"right", sf::Keyboard::Key::D}, {"crouch", sf::Keyboard::Key::S},
                     {"run", sf::Keyboard::Key::LShift}};
    m_boundKey[1] = {{"jump", sf::Keyboard::Key::Up}, {"fire", sf::Keyboard::Key::Period},
                     {"groundpound", sf::Keyboard::Key::Slash}, {"left", sf::Keyboard::Key::Left},
                     {"right", sf::Keyboard::Key::Right}, {"crouch", sf::Keyboard::Key::Down},
                     {"run", sf::Keyboard::Key::N}};
}

void InputManager::registerPlayer(Character* character, int playerIndex) {
    if (playerIndex >= 0 && playerIndex < 2) {
        m_players[playerIndex] = character;
    }
}

void InputManager::handleInput(const sf::Event& event, Character& character) {
    int pIdx = 0;
    if (&character == m_players[1]) {
        pIdx = 1;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        auto it = m_pressMappings[pIdx].find(keyPressed->code);
        if (it != m_pressMappings[pIdx].end()) {
            it->second->execute(character);
        }
    }
}

void InputManager::noteKeyEvent(const sf::Event& event) {
    if (const auto* pressed = event.getIf<sf::Event::KeyPressed>()) {
        m_heldKeys.insert(pressed->code);
    } else if (const auto* released = event.getIf<sf::Event::KeyReleased>()) {
        m_heldKeys.erase(released->code);
    } else if (event.is<sf::Event::FocusLost>()) {
        clearHeldKeys();
    }
}

bool InputManager::isHeld(sf::Keyboard::Key key) const {
    return m_heldKeys.count(key) > 0;
}

void InputManager::clearHeldKeys() {
    m_heldKeys.clear();
}

void InputManager::update(Character& character) {
    int pIdx = 0;
    if (&character == m_players[1]) {
        pIdx = 1;
    }

    for (auto& pair : m_holdMappings[pIdx]) {
        if (isHeld(pair.first)) {
            pair.second->execute(character);
        }
    }
}

// --- Key name <-> enum -------------------------------------------------------
// Only the keys a player can reasonably bind. Anything outside this table is
// rejected rather than guessed at.
namespace {
const std::pair<const char*, sf::Keyboard::Key> kKeyNames[] = {
    {"A", sf::Keyboard::Key::A}, {"B", sf::Keyboard::Key::B}, {"C", sf::Keyboard::Key::C},
    {"D", sf::Keyboard::Key::D}, {"E", sf::Keyboard::Key::E}, {"F", sf::Keyboard::Key::F},
    {"G", sf::Keyboard::Key::G}, {"H", sf::Keyboard::Key::H}, {"I", sf::Keyboard::Key::I},
    {"J", sf::Keyboard::Key::J}, {"K", sf::Keyboard::Key::K}, {"L", sf::Keyboard::Key::L},
    {"M", sf::Keyboard::Key::M}, {"N", sf::Keyboard::Key::N}, {"O", sf::Keyboard::Key::O},
    {"P", sf::Keyboard::Key::P}, {"Q", sf::Keyboard::Key::Q}, {"R", sf::Keyboard::Key::R},
    {"S", sf::Keyboard::Key::S}, {"T", sf::Keyboard::Key::T}, {"U", sf::Keyboard::Key::U},
    {"V", sf::Keyboard::Key::V}, {"W", sf::Keyboard::Key::W}, {"X", sf::Keyboard::Key::X},
    {"Y", sf::Keyboard::Key::Y}, {"Z", sf::Keyboard::Key::Z},
    {"Space",  sf::Keyboard::Key::Space},  {"LShift", sf::Keyboard::Key::LShift},
    {"RShift", sf::Keyboard::Key::RShift}, {"Left",   sf::Keyboard::Key::Left},
    {"Right",  sf::Keyboard::Key::Right},  {"Up",     sf::Keyboard::Key::Up},
    {"Down",   sf::Keyboard::Key::Down},   {"Enter",  sf::Keyboard::Key::Enter},
};
} // namespace

std::string InputManager::getActionForKey(const std::string& key, int playerIndex) const {
    if (playerIndex < 0 || playerIndex > 1) return "";
    sf::Keyboard::Key code;
    if (!parseKeyName(key, code)) return "";

    for (const auto& [action, bound] : m_boundKey[playerIndex]) {
        if (bound == code) return action;
    }
    return "";
}

bool InputManager::isActionHeld(const std::string& action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex > 1) return false;
    const auto& table = m_boundKey[playerIndex];
    auto it = table.find(action);
    if (it != table.end() && isHeld(it->second)) return true;

    // Check secondary keys (e.g. Space for Player 1 jump, RShift for Player 2 jump)
    if (action == "jump") {
        if (playerIndex == 0 && isHeld(sf::Keyboard::Key::Space)) return true;
        if (playerIndex == 1 && isHeld(sf::Keyboard::Key::RShift)) return true;
    }

    return false;
}

std::string InputManager::getBoundKeyName(const std::string& action, int playerIndex) const {
    if (playerIndex < 0 || playerIndex > 1) return "";
    const auto& table = m_boundKey[playerIndex];
    auto it = table.find(action);
    if (it == table.end()) return "";
    return keyName(it->second);
}

std::string InputManager::keyName(sf::Keyboard::Key key) {
    for (const auto& [name, value] : kKeyNames) {
        if (value == key) return name;
    }
    return "";
}

bool InputManager::parseKeyName(const std::string& name, sf::Keyboard::Key& out) {
    for (const auto& [text, value] : kKeyNames) {
        if (name == text) { out = value; return true; }
    }
    return false;
}

namespace {
// keyName() is static on the class; this keeps the log line readable.
std::string key_name_for_log(sf::Keyboard::Key key) {
    const std::string name = InputManager::keyName(key);
    return name.empty() ? "?" : name;
}
}

void InputManager::applyBindings(const std::unordered_map<std::string, std::string>& bindings,
                                 int playerIndex) {
    if (playerIndex < 0 || playerIndex > 1) return;

    for (const auto& [action, keyText] : bindings) {
        auto cmdIt = m_commandsByAction.find(action);
        if (cmdIt == m_commandsByAction.end()) continue;   // not a bindable action

        sf::Keyboard::Key key;
        if (!parseKeyName(keyText, key)) {
            std::cerr << "[InputManager] Ignoring unknown key \"" << keyText
                      << "\" bound to \"" << action << "\"" << std::endl;
            continue;
        }

        const bool held = m_heldActions.count(action) > 0;
        auto& mappings = held ? m_holdMappings[playerIndex] : m_pressMappings[playerIndex];

        // Release the key this action currently occupies, then take the new one.
        auto boundIt = m_boundKey[playerIndex].find(action);
        if (boundIt != m_boundKey[playerIndex].end()) {
            mappings.erase(boundIt->second);
        }

        // If another action already owns this key, hand it the key we are about
        // to vacate instead of leaving it dead. Binding "jump" to A used to
        // silently unbind "left", and the only way back was editing config.json.
        std::string displaced;
        for (const auto& [otherAction, otherKey] : m_boundKey[playerIndex]) {
            if (otherAction != action && otherKey == key) {
                displaced = otherAction;
                break;
            }
        }

        const bool haveOldKey = (boundIt != m_boundKey[playerIndex].end());
        // Re-applying the key an action already holds is not a conflict. Crouch
        // and ground pound ship on the same key deliberately — holding and
        // tapping are different gestures — so a default re-apply would otherwise
        // "swap" S onto S and log a conflict that does not exist.
        if (!displaced.empty() && haveOldKey && boundIt->second != key) {
            const sf::Keyboard::Key vacated = boundIt->second;
            const bool displacedHeld = m_heldActions.count(displaced) > 0;
            auto& displacedMappings = displacedHeld ? m_holdMappings[playerIndex]
                                                    : m_pressMappings[playerIndex];
            auto displacedCmd = m_commandsByAction.find(displaced);
            if (displacedCmd != m_commandsByAction.end()) {
                displacedMappings.erase(key);
                displacedMappings[vacated] = displacedCmd->second;
                m_boundKey[playerIndex][displaced] = vacated;
                std::cout << "[InputManager] \"" << key_name_for_log(key) << "\" was on \""
                          << displaced << "\"; swapped it onto \"" << key_name_for_log(vacated)
                          << "\" rather than leaving it unbound." << std::endl;
            }
        }

        mappings[key] = cmdIt->second;
        m_boundKey[playerIndex][action] = key;
    }
}

std::unordered_map<std::string, std::string> InputManager::resetBindingsToDefaults(int playerIndex) {
    std::unordered_map<std::string, std::string> defaults;
    if (playerIndex < 0 || playerIndex > 1) return defaults;

    // The built-in layout, named once here and applied through the same path a
    // user rebind takes, so defaults cannot drift from what applyBindings does.
    if (playerIndex == 0) {
        defaults = {{"left", "A"}, {"right", "D"}, {"jump", "W"}, {"run", "LShift"},
                    {"crouch", "S"}, {"fire", "F"}, {"groundpound", "Q"}};
    } else {
        defaults = {{"left", "Left"}, {"right", "Right"}, {"jump", "Up"}, {"run", "N"},
                    {"crouch", "Down"}, {"fire", "M"}, {"groundpound", "Slash"}};
    }

    applyBindings(defaults, playerIndex);
    return defaults;
}
