#include "Core/MoveLeftCommand.hpp"

void MoveLeftCommand::execute(Character& character) {
    character.moveLeft();
}
