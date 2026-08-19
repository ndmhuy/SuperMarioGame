#pragma once

#include <string>
#include <vector>

// Task 10.4 — the Command pattern applied to a text console.
//
// The gameplay commands (Jump, Move, Fire, ...) already implement ICommand,
// whose receiver is a Character. A console command has a different receiver —
// the game as a whole — and returns text rather than acting on one character, so
// it gets its own interface rather than being forced through the other one.
//
// Everything a console command needs is reachable through the singletons
// (Game, EventBus, SoundManager) or through Game::getPlayer(), so no command
// needs a handle on PlayingState and the console stays independent of whatever
// state is on top.
class IConsoleCommand {
public:
    virtual ~IConsoleCommand() = default;

    // What the user types.
    virtual std::string name() const = 0;
    // One line, shown by `help`.
    virtual std::string help() const = 0;
    // Runs the command and returns what to print. Errors are returned as text,
    // not thrown: a typo in a console is not exceptional.
    virtual std::string execute(const std::vector<std::string>& args) = 0;
};
