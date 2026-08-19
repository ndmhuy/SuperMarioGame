#pragma once

#include "Physics/AABB.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <string>
#include <vector>

class SpriteSheet;

// Which backdrop a level gets. Parsed from the level file's "theme" field.
enum class BackgroundTheme {
    Overworld,
    Underground,
    Castle,
    Ice
};

// Task 5.5 — the parallax background.
//
// Before this there was no background at all: Game::run cleared the window to a
// flat cornflower blue and the tilemap was drawn straight onto it, so every
// level looked the same behind the geometry regardless of its theme.
//
// The world atlas has always shipped the pieces — mountains, bushes, clouds,
// fences, trees, castles — they were simply never drawn. Each theme picks a sky
// colour and a set of layers, and each layer scrolls at a fraction of the
// camera's speed, which is what sells depth.
//
// Decoration placement is derived from world position, not random: a hill has to
// be in the same place every time the camera passes it, or the background
// crawls.
class BackgroundRenderer {
public:
    void setTheme(BackgroundTheme theme);
    // Accepts the level file's spelling; unknown themes fall back to Overworld
    // rather than drawing nothing.
    void setTheme(const std::string& themeName);
    BackgroundTheme getTheme() const { return m_theme; }

    void setSpriteSheet(const SpriteSheet* sheet) { m_sheet = sheet; }

    // Drives the drifting cloud layer.
    void update(float dt);

    // Draws sky and layers for what the camera can see. Screen-space: call it
    // before the world view is applied.
    void render(sf::RenderTarget& target, const AABB& visibleBounds) const;

    // The sky colour for the current theme, so the window clear can match and
    // no seam shows above the layers.
    sf::Color getSkyColor() const;

    static BackgroundTheme parseThemeName(const std::string& themeName);

private:
    // One parallax layer: a set of frames placed along the ground line at a
    // fraction of camera speed.
    struct Layer {
        std::vector<std::string> frames;
        float parallax = 0.5f;   // 0 = painted on the screen, 1 = moves with the world
        float baselineY = 0.0f;  // screen y of the layer's bottom edge
        float spacing = 320.0f;  // world px between decorations
        float scale = 2.0f;
        float drift = 0.0f;      // px/s of independent motion, for clouds
        sf::Color tint = sf::Color::White;
    };

    const std::vector<Layer>& layersForTheme() const;

    BackgroundTheme m_theme = BackgroundTheme::Overworld;
    const SpriteSheet* m_sheet = nullptr;
    float m_elapsed = 0.0f;
};
