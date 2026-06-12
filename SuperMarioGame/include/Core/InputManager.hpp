#pragma once

#include "Core/ICommand.hpp"
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>

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

    // Register characters to player indices (0 = P1, 1 = P2)
    void registerPlayer(Character* character, int playerIndex);

private:
    InputManager();
    ~InputManager() = default;

    void loadDefaultBindings();

    // Registered player pointers
    Character* m_players[2] = { nullptr, nullptr };

    // We support Player 1 (index 0) and Player 2 (index 1)
    std::unordered_map<sf::Keyboard::Key, std::shared_ptr<ICommand>> m_pressMappings[2];
    std::unordered_map<sf::Keyboard::Key, std::shared_ptr<ICommand>> m_holdMappings[2];
};