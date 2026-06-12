#pragma once

#include "Core/ICommand.hpp"

class RunCommand : public ICommand {
public:
    void execute(Character& character) override;
};
