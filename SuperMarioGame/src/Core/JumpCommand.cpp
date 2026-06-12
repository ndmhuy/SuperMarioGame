#include "Core/JumpCommand.hpp"

void JumpCommand::execute(Character& character) {
    character.jump();
}
