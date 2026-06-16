#pragma once

#include "Entities/Character.hpp"

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(Character& character) = 0;
};