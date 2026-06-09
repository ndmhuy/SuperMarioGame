#pragma once

#include <SFML/System/Vector2.hpp>

namespace MathUtils {
    // Clamping helper
    template <typename T>
    T clamp(T value, T min, T max);

    // Linear interpolation helper
    template <typename T>
    T lerp(T start, T end, float t);

    // Sign helper (returns -1, 0, or 1)
    template <typename T>
    int sign(T value);

    // Vector operations
    float magnitude(const sf::Vector2f& vector);
    sf::Vector2f normalize(const sf::Vector2f& vector);
    float distance(const sf::Vector2f& p1, const sf::Vector2f& p2);
}

// Template implementations in header for utility functions
template <typename T>
T MathUtils::clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

template <typename T>
T MathUtils::lerp(T start, T end, float t) {
    return start + static_cast<T>((end - start) * t);
}

template <typename T>
int MathUtils::sign(T value) {
    return (T(0) < value) - (value < T(0));
}
