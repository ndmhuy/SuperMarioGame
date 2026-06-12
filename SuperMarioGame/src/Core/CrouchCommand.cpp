#include "Core/CrouchCommand.hpp"
#include "Entities/Player.hpp"

void CrouchCommand::execute(Character& character) {
    if (auto* player = dynamic_cast<Player*>(&character)) {
        player->crouch();
    }
}
