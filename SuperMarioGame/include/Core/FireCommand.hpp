#pragma once

#include "Core/ICommand.hpp"

class FireCommand : public ICommand {
public:
    void execute(Character& character) override;
};
