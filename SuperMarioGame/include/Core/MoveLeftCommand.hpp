#pragma once

#include "Core/ICommand.hpp"

class MoveLeftCommand : public ICommand {
public:
    void execute(Character& character) override;
};
