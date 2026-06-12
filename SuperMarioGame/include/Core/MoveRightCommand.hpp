#pragma once

#include "Core/ICommand.hpp"

class MoveRightCommand : public ICommand {
public:
    void execute(Character& character) override;
};
