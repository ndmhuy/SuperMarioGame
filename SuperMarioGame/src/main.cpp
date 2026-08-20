#include "Core/Game.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

// Entry point.
//
// `--script <file>` plays a recorded input script instead of waiting for a
// human, and is how the multiplayer modes get verified without anyone having to
// sit and play them. See InputScript.hpp. A normal launch takes no arguments and
// behaves exactly as before.
int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
            if (!Game::getInstance().loadInputScript(argv[++i])) return 1;
        } else if (std::strcmp(argv[i], "--train") == 0) {
            // Straight into the training view. Optional level index follows;
            // episodes rotate through the whole campaign from there.
            int level = 0;
            if (i + 1 < argc && argv[i + 1][0] != '-') level = std::atoi(argv[++i]);
            Game::getInstance().setLaunchTraining(level);
        } else {
            std::cerr << "[main] Ignoring unrecognised argument '" << argv[i] << "'."
                      << std::endl;
        }
    }

    Game::getInstance().run();
    return 0;
}
