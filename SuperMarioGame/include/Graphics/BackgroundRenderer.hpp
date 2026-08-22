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

    // Fills everything below the layers' ground line with earth.
    //
    // Off by default: in a level the tilemap covers that area, and the ground
    // line is a *screen* coordinate, so a band would be wrong wherever the
    // camera is looking above the ground. The menus have no tilemap, which is
    // why their hills floated over bare sky.
    void setDrawGroundBand(bool enabled) { m_drawGroundBand = enabled; }

    // Drives the drifting cloud layer.
    void update(float dt);

    // Draws sky and layers for what the camera can see. Screen-space: call it
    // before the world view is applied.
    void render(sf::RenderTarget& target, const AABB& visibleBounds) const;

    // The sky colour for the current theme, so the window clear can match and
    // no seam shows above the layers.
    sf::Color getSkyColor() const;

    static BackgroundTheme parseThemeName(const std::string& themeName);

    // Tell the backdrop where the world's ground surface is, in WORLD
    // coordinates, so the ground-standing layers can be pinned to it rather than
    // to a hardcoded screen line that the camera makes wrong. Pass 0 (the
    // default) for surfaces with no tilemap behind them, such as the menu.
    void setWorldGroundY(float worldY) { m_worldGroundY = worldY; }

private:
    // One parallax layer: a set of frames placed along the ground line at a
    // fraction of camera speed.
    struct Layer {
        std::vector<std::string> frames;
        float parallax = 0.5f;   // 0 = painted on the screen, 1 = moves with the world
        float baselineY = 0.0f;  // screen y of the bottom edge, SKY LAYERS ONLY
        float spacing = 320.0f;  // world px between decorations
        float scale = 2.0f;
        float drift = 0.0f;      // px/s of independent motion, for clouds
        sf::Color tint = sf::Color::White;
        // Does this layer hang in the sky, or stand on the ground?
        //
        // This used to be inferred at draw time by comparing baselineY against
        // wherever the world's ground happened to render this frame. Ground
        // layers were authored with baselineY == GROUND_LINE == 640, so the
        // moment the real ground rendered below 641 — which is every campaign
        // level, where it lands at 656 — every hill, bush and fence was
        // reclassified as sky, pinned to the stale 640 constant and given the
        // vertical jitter meant for clouds. They drew a full tile above the
        // floor. Whether a decoration stands on the ground is a fact about the
        // decoration, so it is stated here rather than guessed from a number.
        bool sky = false;
    };

    const std::vector<Layer>& layersForTheme() const;

    BackgroundTheme m_theme = BackgroundTheme::Overworld;
    const SpriteSheet* m_sheet = nullptr;
    bool m_drawGroundBand = false;
    float m_elapsed = 0.0f;
    // World y of the ground surface, or 0 when unknown.
    float m_worldGroundY = 0.0f;
};
