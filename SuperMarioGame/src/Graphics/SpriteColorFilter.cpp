#include "Graphics/SpriteColorFilter.hpp"
#include <cmath>

sf::Color SpriteColorFilter::hslToRgb(float hue, float saturation, float lightness, std::uint8_t alpha) {
    // Normalize hue to [0, 360)
    while (hue < 0.0f) hue += 360.0f;
    while (hue >= 360.0f) hue -= 360.0f;

    float c = (1.0f - std::abs(2.0f * lightness - 1.0f)) * saturation;
    float hPrime = hue / 60.0f;
    float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));
    float m = lightness - c / 2.0f;

    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (hPrime >= 0.0f && hPrime < 1.0f) {
        r = c; g = x; b = 0.0f;
    } else if (hPrime >= 1.0f && hPrime < 2.0f) {
        r = x; g = c; b = 0.0f;
    } else if (hPrime >= 2.0f && hPrime < 3.0f) {
        r = 0.0f; g = c; b = x;
    } else if (hPrime >= 3.0f && hPrime < 4.0f) {
        r = 0.0f; g = x; b = c;
    } else if (hPrime >= 4.0f && hPrime < 5.0f) {
        r = x; g = 0.0f; b = c;
    } else if (hPrime >= 5.0f && hPrime < 6.0f) {
        r = c; g = 0.0f; b = x;
    }

    std::uint8_t redByte   = static_cast<std::uint8_t>(std::round((r + m) * 255.0f));
    std::uint8_t greenByte = static_cast<std::uint8_t>(std::round((g + m) * 255.0f));
    std::uint8_t blueByte  = static_cast<std::uint8_t>(std::round((b + m) * 255.0f));

    return sf::Color(redByte, greenByte, blueByte, alpha);
}

sf::Color SpriteColorFilter::getRainbowColor(float elapsedTime, float cycleSpeed) {
    float hue = std::fmod(elapsedTime * cycleSpeed, 360.0f);
    return hslToRgb(hue, 1.0f, 0.5f, 255);
}

sf::Color SpriteColorFilter::getMarioStarPaletteColor(float elapsedTime, float stepInterval) {
    static const sf::Color palette[] = {
        sf::Color(255, 255, 255), // White / Original
        sf::Color(255, 215, 0),   // Yellow / Gold
        sf::Color(255, 50, 50),   // Red
        sf::Color(50, 220, 50),   // Green
        sf::Color(50, 180, 255),  // Cyan / Blue
        sf::Color(240, 100, 240)  // Magenta / Purple
    };

    int index = static_cast<int>(elapsedTime / stepInterval) % 6;
    return palette[index];
}

sf::Color SpriteColorFilter::getHurtFlickerColor(float invincibilityTimer, float elapsedTime, float frequency) {
    // Initial hurt flash (e.g. 0.0s to 0.15s after hit, assuming invincibilityTimer decreases or elapsedTime is relative)
    // If invincibilityTimer is > 1.85s out of 2.0s (i.e. first 0.15s), show red hit tint.
    if (invincibilityTimer > 1.85f) {
        return sf::Color(255, 80, 80, 230);
    }

    // 15Hz square wave flicker between 128 (50% alpha) and 255 (100% alpha)
    int phase = static_cast<int>(elapsedTime * frequency * 2.0f) % 2;
    std::uint8_t alpha = (phase == 0) ? 128 : 255;
    return sf::Color(255, 255, 255, alpha);
}

void SpriteColorFilter::applyColorFilter(sf::Sprite& sprite, const sf::Color& color) {
    sprite.setColor(color);
}
