#pragma once

#include "Core/ICommand.hpp"

class WallJumpCommand : public ICommand {
public:
    void execute(Character& character) override;
};
