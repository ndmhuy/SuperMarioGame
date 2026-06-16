#pragma once

#include "Core/ICommand.hpp"

class CrouchCommand : public ICommand {
public:
    void execute(Character& character) override;
};
