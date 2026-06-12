#include "Core/RunCommand.hpp"
#include "Entities/Player.hpp"

void RunCommand::execute(Character& character) {
    if (auto* player = dynamic_cast<Player*>(&character)) {
        player->run();
    }
}
