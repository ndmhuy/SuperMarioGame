// Render one map-generator genome to a level JSON. The single-purpose half of
// the evolutionary generator: tools/evolve.py owns selection, crossover and
// mutation; this owns nothing but genome -> level, so the search loop can call
// it hundreds of times without linking Python against the engine.
//
//   ./render_genome out.json --seed 42 --theme castle --difficulty hard \
//                   --pit 0.14 --pipe 0.05 --enemy 0.22 --coin 0.2 \
//                   --rough 0.45 --lava 1 --moving 1 --width 200
//
// Every knob of MapGeneratorConfig is a gene. The oracle (solvability.py)
// scores the result; nothing here judges the level.

#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/MapGenerator.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: render_genome <out.json> [--seed N --theme T "
                     "--difficulty D --width N --pit F --pipe F --enemy F "
                     "--coin F --rough F --lava 0|1 --moving 0|1 --name S]\n";
        return 2;
    }
    const std::string outPath = argv[1];
    MapGeneratorConfig config;
    std::string name = "Evolved Level";

    auto next = [&](int& i) -> const char* {
        return (i + 1 < argc) ? argv[++i] : nullptr;
    };
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        const char* v = nullptr;
        if (arg == "--seed"       && (v = next(i))) config.seed = static_cast<unsigned>(std::strtoul(v, nullptr, 10));
        else if (arg == "--width" && (v = next(i))) config.width = std::atoi(v);
        else if (arg == "--pit"   && (v = next(i))) config.pitProbability = std::strtof(v, nullptr);
        else if (arg == "--pipe"  && (v = next(i))) config.pipeFrequency = std::strtof(v, nullptr);
        else if (arg == "--enemy" && (v = next(i))) config.enemySpawnRate = std::strtof(v, nullptr);
        else if (arg == "--coin"  && (v = next(i))) config.coinClusterRate = std::strtof(v, nullptr);
        else if (arg == "--rough" && (v = next(i))) config.roughness = std::strtof(v, nullptr);
        else if (arg == "--lava"  && (v = next(i))) config.enableLava = std::atoi(v) != 0;
        else if (arg == "--moving"&& (v = next(i))) config.enableMovingPlatforms = std::atoi(v) != 0;
        else if (arg == "--name"  && (v = next(i))) name = v;
        else if (arg == "--theme" && (v = next(i))) {
            const std::string t = v;
            config.theme = t == "underground" ? MapTheme::Underground
                         : t == "castle"      ? MapTheme::Castle
                         : t == "ice"         ? MapTheme::Ice
                                              : MapTheme::Overworld;
        } else if (arg == "--difficulty" && (v = next(i))) {
            const std::string d = v;
            config.difficulty = d == "hard"   ? MapDifficulty::Hard
                              : d == "medium" ? MapDifficulty::Medium
                                              : MapDifficulty::Easy;
        } else {
            std::cerr << "[render_genome] unknown or valueless arg '" << arg << "'\n";
            return 2;
        }
    }

    TileMap map;
    std::vector<std::unique_ptr<Entity>> entities;
    MapGenerator::generate(map, entities, config);

    LevelLoader loader;
    if (!loader.saveLevel(outPath, map, entities, name)) {
        std::cerr << "[render_genome] could not write " << outPath << "\n";
        return 1;
    }
    std::cout << outPath << std::endl;
    return 0;
}
