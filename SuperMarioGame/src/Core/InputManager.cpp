#include "Core/InputManager.hpp"
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

    // --- PLAYER 1 BINDINGS (WASD) ---
    // Press mappings (one-shot actions)
    m_pressMappings[0][sf::Keyboard::Key::W] = compositeJumpCmd;
    m_pressMappings[0][sf::Keyboard::Key::Space] = compositeJumpCmd;
    m_pressMappings[0][sf::Keyboard::Key::F] = fireCmd;
    m_pressMappings[0][sf::Keyboard::Key::S] = gpCmd; // Press S to ground pound

    // Hold mappings (continuous actions)
    m_holdMappings[0][sf::Keyboard::Key::A] = leftCmd;
    m_holdMappings[0][sf::Keyboard::Key::D] = rightCmd;
    m_holdMappings[0][sf::Keyboard::Key::S] = crouchCmd; // Hold S to crouch
    m_holdMappings[0][sf::Keyboard::Key::LShift] = runCmd;

    // --- PLAYER 2 BINDINGS (Arrow keys) ---
    // Press mappings (one-shot actions)
    m_pressMappings[1][sf::Keyboard::Key::Up] = compositeJumpCmd;
    m_pressMappings[1][sf::Keyboard::Key::M] = fireCmd;
    m_pressMappings[1][sf::Keyboard::Key::Down] = gpCmd; // Press Down arrow to ground pound

    // Hold mappings (continuous actions)
    m_holdMappings[1][sf::Keyboard::Key::Left] = leftCmd;
    m_holdMappings[1][sf::Keyboard::Key::Right] = rightCmd;
    m_holdMappings[1][sf::Keyboard::Key::Down] = crouchCmd; // Hold Down arrow to crouch
    m_holdMappings[1][sf::Keyboard::Key::N] = runCmd;
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

void InputManager::update(Character& character) {
    int pIdx = 0;
    if (&character == m_players[1]) {
        pIdx = 1;
    }

    for (auto& pair : m_holdMappings[pIdx]) {
        if (sf::Keyboard::isKeyPressed(pair.first)) {
            pair.second->execute(character);
        }
    }
}
