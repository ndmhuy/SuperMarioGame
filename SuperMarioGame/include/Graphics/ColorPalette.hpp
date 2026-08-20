#pragma once

#include <SFML/Graphics/Color.hpp>

// Task 11.4 — the colourblind palette.
//
// Game has stored a colourblind flag since settings existed, Serializer persists
// it, the options screen and the dev panel both toggle it — and nothing read it.
// Exactly the shape of the difficulty bug: a setting that was saved and had no
// consumer.
//
// The thing it most needs to fix is the minimap, which drew the player in green
// and enemies in red. Red against green is the single most common form of colour
// blindness (deuteranopia, ~6% of men), so those two markers were indequately
// distinguishable for a substantial number of players — and the minimap exists
// precisely to tell you where the enemies are.
//
// The alternative palette is Okabe-Ito, which is designed to stay distinguishable
// under all three common types of colour blindness. Colours are requested by
// *meaning* rather than by name, so a caller cannot accidentally reintroduce a
// red/green pair.
class ColorPalette {
public:
    // Semantic roles, not colours. Add a role rather than reaching for a literal.
    enum class Role {
        Player,
        Enemy,
        Item,
        Block,
        Hazard,
        Accent      // UI highlight
    };

    // The colour for a role under the current setting. Reads
    // Game::getColorblindMode() on every call, so toggling it in the options
    // screen takes effect immediately rather than at the next level load.
    static sf::Color get(Role role);

    // Whether the alternative palette is in use, for callers that also want to
    // change shape rather than only colour.
    static bool isColorblindModeActive();
};
