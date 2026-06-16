#include "Core/WallJumpCommand.hpp"
#include "Entities/Player.hpp"

void WallJumpCommand::execute(Character& character) {
    if (auto* player = dynamic_cast<Player*>(&character)) {
        player->wallJump();
    }
}
