#include "Core/DifficultyStrategy.hpp"

std::unique_ptr<IDifficultyStrategy> IDifficultyStrategy::fromId(const std::string& id) {
    if (id == "easy") return std::make_unique<EasyDifficulty>();
    if (id == "hard") return std::make_unique<HardDifficulty>();
    // Normal is the fallback for anything unrecognised, because config.json is
    // hand-editable and a typo should not take the game down.
    return std::make_unique<NormalDifficulty>();
}
