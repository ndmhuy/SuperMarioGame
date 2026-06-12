#include "Core/GroundPoundCommand.hpp"
#include "Entities/Player.hpp"

void GroundPoundCommand::execute(Character& character) {
    if (auto* player = dynamic_cast<Player*>(&character)) {
        player->groundPound();
    }
}
