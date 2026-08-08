#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

class SpriteColorFilter {
public:
    // HSL (Hue: 0..360, Saturation: 0..1, Lightness: 0..1) to SFML sf::Color
    static sf::Color hslToRgb(float hue, float saturation, float lightness, std::uint8_t alpha = 255);

    // Dynamic rainbow color generation for Star Power and Star Kill FX (12Hz hue cycling default)
    static sf::Color getRainbowColor(float elapsedTime, float cycleSpeed = 600.0f);

    // Classic 6-stage Mario invincibility palette selector
    static sf::Color getMarioStarPaletteColor(float elapsedTime, float stepInterval = 0.08f);

    // Hit invincibility alpha flicker (15Hz) & optional initial red damage flash (0.15s)
    static sf::Color getHurtFlickerColor(float invincibilityTimer, float elapsedTime, float frequency = 15.0f);

    // Helper: Apply color tint to sf::Sprite
    static void applyColorFilter(sf::Sprite& sprite, const sf::Color& color);
};
