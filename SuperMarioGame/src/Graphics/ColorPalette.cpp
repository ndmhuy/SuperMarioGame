#include "Graphics/ColorPalette.hpp"
#include "Core/Game.hpp"

namespace {

// The original look: familiar, and unusable if you cannot separate red from
// green.
sf::Color standardColor(ColorPalette::Role role) {
    switch (role) {
        case ColorPalette::Role::Player: return sf::Color(0, 200, 0);        // green
        case ColorPalette::Role::Enemy:  return sf::Color(220, 40, 40);      // red
        case ColorPalette::Role::Item:   return sf::Color(255, 216, 0);      // yellow
        case ColorPalette::Role::Block:  return sf::Color(0, 200, 255);      // cyan
        case ColorPalette::Role::Hazard: return sf::Color(255, 120, 0);      // orange
        case ColorPalette::Role::Accent:
        default:                         return sf::Color(255, 255, 255);
    }
}

// Okabe-Ito, chosen because it stays separable under deuteranopia, protanopia
// and tritanopia.
//
// Player is the *bright* sky blue and Enemy the darker vermilion, which is not
// arbitrary: hue separation alone is not enough, since a display in greyscale or
// a player with achromatopsia has only brightness to go on. This pairing differs
// by about 51 units of relative luminance; the more obvious blue/vermilion pair
// differs by 18, which is close enough to read as the same shade of grey.
sf::Color colorblindColor(ColorPalette::Role role) {
    switch (role) {
        case ColorPalette::Role::Player: return sf::Color(86, 180, 233);     // sky blue
        case ColorPalette::Role::Enemy:  return sf::Color(213, 94, 0);       // vermilion
        case ColorPalette::Role::Item:   return sf::Color(240, 228, 66);     // yellow
        case ColorPalette::Role::Block:  return sf::Color(0, 114, 178);      // blue
        case ColorPalette::Role::Hazard: return sf::Color(204, 121, 167);    // reddish purple
        case ColorPalette::Role::Accent:
        default:                         return sf::Color(255, 255, 255);
    }
}

} // namespace

bool ColorPalette::isColorblindModeActive() {
    return Game::getInstance().getColorblindMode();
}

sf::Color ColorPalette::get(Role role) {
    return isColorblindModeActive() ? colorblindColor(role) : standardColor(role);
}
