#pragma once

#include "Core/ICommand.hpp"

class JumpCommand : public ICommand {
public:
    void execute(Character& character) override;
};
