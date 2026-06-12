#pragma once

#include "Core/ICommand.hpp"

class GroundPoundCommand : public ICommand {
public:
    void execute(Character& character) override;
};
