#include "Utils/MathUtils.hpp"
#include <cmath>

namespace MathUtils {
    float magnitude(const sf::Vector2f& vector) {
        // TODO: Implement skeleton / stub
        return std::sqrt(vector.x * vector.x + vector.y * vector.y);
    }

    sf::Vector2f normalize(const sf::Vector2f& vector) {
        float mag = magnitude(vector);
        if (mag != 0.0f) {
            return vector / mag;
        }
        return sf::Vector2f{0.0f, 0.0f};
    }

    float distance(const sf::Vector2f& p1, const sf::Vector2f& p2) {
        return magnitude(p2 - p1);
    }
}
