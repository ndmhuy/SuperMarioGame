#include "Graphics/BackgroundRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <cmath>

namespace {

// Deterministic pick for a decoration slot. A hill must land in the same place
// every time the camera passes it; std::rand here would make the whole backdrop
// shimmer as you walk.
std::size_t slotHash(int slot, std::size_t modulo) {
    if (modulo == 0) return 0;
    const unsigned int x = static_cast<unsigned int>(slot) * 2654435761u;
    return static_cast<std::size_t>((x >> 16) % modulo);
}

constexpr float GROUND_LINE = 640.0f;   // screen y the layers sit on

} // namespace

BackgroundTheme BackgroundRenderer::parseThemeName(const std::string& themeName) {
    if (themeName == "underground" || themeName == "cave")   return BackgroundTheme::Underground;
    if (themeName == "castle" || themeName == "lava")        return BackgroundTheme::Castle;
    if (themeName == "ice" || themeName == "snow")           return BackgroundTheme::Ice;
    return BackgroundTheme::Overworld;
}

void BackgroundRenderer::setTheme(BackgroundTheme theme) {
    m_theme = theme;
}

void BackgroundRenderer::setTheme(const std::string& themeName) {
    m_theme = parseThemeName(themeName);
}

void BackgroundRenderer::update(float dt) {
    m_elapsed += dt;
}

sf::Color BackgroundRenderer::getSkyColor() const {
    switch (m_theme) {
        case BackgroundTheme::Underground: return sf::Color(20, 16, 48);
        case BackgroundTheme::Castle:      return sf::Color(28, 8, 12);
        case BackgroundTheme::Ice:         return sf::Color(148, 196, 236);
        case BackgroundTheme::Overworld:
        default:                           return sf::Color(92, 148, 252);
    }
}

const std::vector<BackgroundRenderer::Layer>& BackgroundRenderer::layersForTheme() const {
    // Built once per theme. Ordered back to front, so a later layer draws over
    // an earlier one.
    static const std::vector<Layer> kOverworld = {
        {{"mountain_tall", "mountain_short"}, 0.25f, GROUND_LINE, 420.0f, 2.0f, 0.0f,
         sf::Color(255, 255, 255)},
        {{"cloud_long", "cloud_short", "cloud_tiny"}, 0.40f, 210.0f, 340.0f, 2.0f, -8.0f,
         sf::Color(255, 255, 255)},
        {{"bush_dark_green_long", "bush_light_green_long", "bush_light_green_short"},
         0.65f, GROUND_LINE, 300.0f, 2.0f, 0.0f, sf::Color(255, 255, 255)},
    };
    static const std::vector<Layer> kUnderground = {
        // No sky to put clouds in, and no foliage underground. The depth cue is
        // fencing receding into the dark at two distances — which is close to
        // what the original games do down here, which is nearly nothing.
        {{"fence_long", "fence_medium"}, 0.30f, GROUND_LINE, 340.0f, 2.0f, 0.0f,
         sf::Color(90, 90, 130)},
        {{"fence_long"}, 0.60f, GROUND_LINE, 260.0f, 2.0f, 0.0f,
         sf::Color(140, 140, 180)},
    };
    static const std::vector<Layer> kCastle = {
        {{"castle_tower_brown"}, 0.25f, GROUND_LINE, 460.0f, 2.0f, 0.0f,
         sf::Color(150, 110, 110)},
        {{"fence_long"}, 0.55f, GROUND_LINE, 300.0f, 2.0f, 0.0f, sf::Color(120, 80, 80)},
    };
    static const std::vector<Layer> kIce = {
        // Deliberately no mountain_* here. Those frames are green hills, and the
        // tint is a multiply — it can darken a sprite but it cannot whiten one,
        // so an "ice" backdrop built from them just looks like a green field.
        // The atlas ships white trees; those are the snow-covered silhouettes.
        {{"tree_white_tall", "tree_white_long"}, 0.25f, GROUND_LINE, 300.0f, 3.2f, 0.0f,
         sf::Color(225, 235, 250)},
        {{"cloud_long", "cloud_short", "cloud_tiny"}, 0.40f, 190.0f, 320.0f, 2.0f, -6.0f,
         sf::Color(255, 255, 255)},
        {{"tree_white_short", "tree_white_tall"}, 0.65f, GROUND_LINE, 200.0f, 2.0f, 0.0f,
         sf::Color(255, 255, 255)},
    };

    switch (m_theme) {
        case BackgroundTheme::Underground: return kUnderground;
        case BackgroundTheme::Castle:      return kCastle;
        case BackgroundTheme::Ice:         return kIce;
        case BackgroundTheme::Overworld:
        default:                           return kOverworld;
    }
}

void BackgroundRenderer::render(sf::RenderTarget& target, const AABB& visibleBounds) const {
    const float screenW = static_cast<float>(Constants::WINDOW_WIDTH);
    const float screenH = static_cast<float>(Constants::WINDOW_HEIGHT);

    // Sky first, so the layers have something to sit on and no seam shows.
    sf::RectangleShape sky({screenW, screenH});
    sky.setFillColor(getSkyColor());
    target.draw(sky);

    if (m_drawGroundBand) {
        sf::RectangleShape earth({screenW, screenH - GROUND_LINE});
        earth.setPosition({0.0f, GROUND_LINE});
        // Darker than the layer tint so the silhouettes still read against it.
        earth.setFillColor(m_theme == BackgroundTheme::Ice ? sf::Color(198, 214, 232)
                                                           : sf::Color(88, 56, 24));
        target.draw(earth);
    }

    if (!m_sheet) return;

    for (const Layer& layer : layersForTheme()) {
        if (layer.frames.empty() || layer.spacing <= 0.0f) continue;

        // Where this layer's origin sits, given how far the camera has moved.
        const float layerX = visibleBounds.x * layer.parallax + m_elapsed * layer.drift;

        // Only the slots the screen can actually see, plus one either side so
        // decorations enter and leave smoothly.
        const int firstSlot = static_cast<int>(std::floor(layerX / layer.spacing)) - 1;
        const int lastSlot = static_cast<int>(std::floor((layerX + screenW) / layer.spacing)) + 1;

        for (int slot = firstSlot; slot <= lastSlot; ++slot) {
            const std::string& frameName =
                layer.frames[slotHash(slot, layer.frames.size())];
            if (!m_sheet->hasFrame(frameName)) continue;

            sf::Sprite sprite = m_sheet->getSprite(frameName);
            const auto bounds = sprite.getLocalBounds();
            if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) continue;

            sprite.setScale({layer.scale, layer.scale});
            sprite.setColor(layer.tint);

            // A second hash nudges the vertical placement so a row of identical
            // decorations does not read as a ruler.
            const float jitter = static_cast<float>(slotHash(slot * 7 + 1, 24));
            const float x = static_cast<float>(slot) * layer.spacing - layerX;
            const float y = layer.baselineY - bounds.size.y * layer.scale - jitter;
            sprite.setPosition({std::round(x), std::round(y)});
            target.draw(sprite);
        }
    }
}
