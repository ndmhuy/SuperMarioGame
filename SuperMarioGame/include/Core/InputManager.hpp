#pragma once

#include "Core/ICommand.hpp"
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>

class InputManager {
public:
    // Delete copy/move semantics for Singleton
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;

    // Singleton Instance
    static InputManager& getInstance();

    // Mapping key input event to command then sent to the character
    void handleInput(const sf::Event& event, Character& character);

    // Update held keys
    void update(Character& character);

    // Apply persisted bindings from config.json.
    //
    // Serializer has always read and written a keyBindings map and Game has
    // always held it, but nothing ever pushed it into the mappings — the player
    // kept the hardcoded defaults no matter what was configured (audit B-11,
    // GitHub issue #9).
    //
    // Keys are action names ("jump", "left", "right", "fire", "run", "crouch")
    // and values are key names as produced by keyName(). Unknown actions and
    // unparseable keys are ignored so a hand-edited config cannot brick input.
    void applyBindings(const std::unordered_map<std::string, std::string>& bindings,
                       int playerIndex = 0);

    // Human-readable name for a key, and its inverse. Used for persistence and
    // for showing the current binding in the options UI.
    // Key currently driving `action` for `playerIndex`, as a keyName() string,
    // or "" when the action is not bound. The options screen needs this to show
    // what a control is set to; Game only holds the bindings that were *changed*.
    std::string getBoundKeyName(const std::string& action, int playerIndex = 0) const;

    static std::string keyName(sf::Keyboard::Key key);
    static bool parseKeyName(const std::string& name, sf::Keyboard::Key& out);

    // Register characters to player indices (0 = P1, 1 = P2)
    void registerPlayer(Character* character, int playerIndex);
    Character* getPlayer(int playerIndex) const {
        if (playerIndex >= 0 && playerIndex < 2) return m_players[playerIndex];
        return nullptr;
    }

private:
    InputManager();
    ~InputManager() = default;

    void loadDefaultBindings();

    // Registered player pointers
    Character* m_players[2] = { nullptr, nullptr };

    // We support Player 1 (index 0) and Player 2 (index 1)
    std::unordered_map<sf::Keyboard::Key, std::shared_ptr<ICommand>> m_pressMappings[2];
    std::unordered_map<sf::Keyboard::Key, std::shared_ptr<ICommand>> m_holdMappings[2];

    // Action-name views over the same commands, so a binding can be moved to a
    // different key without rebuilding it.
    std::unordered_map<std::string, std::shared_ptr<ICommand>> m_commandsByAction;
    std::set<std::string> m_heldActions;                       // polled, not edge-triggered
    std::unordered_map<std::string, sf::Keyboard::Key> m_boundKey[2];
};