#include "Core/FireCommand.hpp"
#include "Entities/Player.hpp"

void FireCommand::execute(Character& character) {
    if (auto* player = dynamic_cast<Player*>(&character)) {
        player->shootFireball();
    }
}
