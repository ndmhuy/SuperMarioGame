#pragma once

#include "Core/ConsoleCommand.hpp"
#include <memory>
#include <string>
#include <vector>

// Task 10.4 — an in-game debug console.
//
// Registers IConsoleCommands by name and dispatches typed lines to them. Kept
// separate from DevPanel because the panels are bound to PlayingState's
// internals, while the console works through the public singletons and so runs
// from any state.
class DebugConsole {
public:
    static DebugConsole& getInstance();

    DebugConsole(const DebugConsole&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;

    // Registers the built-in command set. Safe to call more than once.
    void init();

    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    // Parses a line and runs it. Returns what was printed, so the tests can
    // assert on a command's effect without going near ImGui.
    std::string submit(const std::string& line);

    // Draws the console window. ImGui only — no game state is changed here.
    void draw();

    const std::vector<std::string>& getOutput() const { return m_output; }
    void clearOutput() { m_output.clear(); }

    // Registered commands, for `help` and for the tests.
    std::vector<std::string> commandNames() const;

private:
    DebugConsole() = default;
    ~DebugConsole() = default;

    void registerCommand(std::unique_ptr<IConsoleCommand> command);
    void print(const std::string& line);

    std::vector<std::unique_ptr<IConsoleCommand>> m_commands;
    std::vector<std::string> m_output;
    // Recall with Up/Down, because retyping "spawn goomba" forty times is how a
    // debug tool stops being used.
    std::vector<std::string> m_history;
    int m_historyCursor = -1;
    bool m_visible = false;
    bool m_initialised = false;
};
