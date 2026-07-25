#pragma once

#include <string>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>

struct KeyFrame {
    std::string frameName;
    float duration;
};

struct Animation {
    Animation() = default;
    Animation(std::string name) : name(name) {}
    std::string name;
    std::vector<KeyFrame> frameList;
    bool isLooping = true;
};

