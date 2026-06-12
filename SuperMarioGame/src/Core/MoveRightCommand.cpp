#include "Core/MoveRightCommand.hpp"

void MoveRightCommand::execute(Character& character) {
    character.moveRight();
}
