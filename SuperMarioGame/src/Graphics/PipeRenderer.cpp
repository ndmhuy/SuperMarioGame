#include "Graphics/PipeRenderer.hpp"
#include <cmath>
#include <vector>

namespace {

// The atlas names its whole-pipe art differently from its quarter art: the
// quarters are pipe_green_*, the full sprites are pipe_dark_green_*. Mapping the
// caller's colour to the whole-sprite family keeps that naming split out of
// every call site.
std::string wholeFamilyFor(const std::string& color) {
    if (color == "green") return "dark_green";
    if (color == "white" || color == "white_black" || color == "grey") return "white_black";
    return color;
}

// The single frame that covers a box of this shape, or "" if the atlas has none.
std::string wholeFrameFor(const SpriteSheet* sheet, const std::string& color,
                          sf::Vector2f size) {
    const std::string family = wholeFamilyFor(color);
    std::vector<std::string> candidates;

    if (size.y >= size.x * 1.75f) {
        // Tall: a double-height pipe (62x128), then the 1-tile-wide verticals.
        candidates = {"pipe_" + family + "_long_l",
                      "pipe_" + family + "_long_up",
                      "pipe_" + family + "_slight_long_up"};
    } else if (size.x <= 40.0f) {
        candidates = {"pipe_" + family + "_up",
                      "pipe_" + family + "_long_up",
                      "pipe_" + family + "_short_l"};
    } else {
        // The common case: a 2x2 pipe entity, and pipe_dark_green_short_l is
        // exactly 64x64 — the same size as the hitbox.
        candidates = {"pipe_" + family + "_short_l",
                      "pipe_" + family + "_long_l"};
    }

    for (const std::string& name : candidates) {
        if (sheet->hasFrame(name)) return name;
    }
    return {};
}

} // namespace

void PipeRenderer::draw(sf::RenderTarget& target,
                       const SpriteSheet* scenerySheet,
                       sf::Vector2f position,
                       sf::Vector2f size,
                       float rotationDegrees,
                       bool hasHead,
                       const std::string& color) {
    if (!scenerySheet) return;

    const sf::Vector2f center = position + sf::Vector2f(size.x * 0.5f, size.y * 0.5f);

    // --- One sprite for one hitbox ------------------------------------------
    //
    // A Pipe entity is a single 64x64 collider (see Pipe's constructor), but it
    // used to be *drawn* as four 16x16 quarter frames, each with its own origin,
    // its own 4x per-axis scale and — when rotated — its own rotation about its
    // own centre. That is what produced the hairline seams between quadrants and
    // the visibly mismatched corners on a rotated pipe: four independently
    // transformed sprites cannot stay flush.
    //
    // The atlas already contains whole-pipe art, so the thing with one hitbox is
    // now drawn as one sprite, transformed once.
    if (hasHead) {
        const std::string whole = wholeFrameFor(scenerySheet, color, size);
        if (!whole.empty()) {
            sf::Sprite sprite = scenerySheet->getSprite(whole);
            const sf::FloatRect bounds = sprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                sprite.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
                // Filled to the collider deliberately: the sprite and the thing
                // the player collides with should be the same rectangle. For the
                // 64x64 case this is exactly 1:1.
                sprite.setScale({size.x / bounds.size.x, size.y / bounds.size.y});
                sprite.setPosition(center);
                sprite.setRotation(sf::degrees(rotationDegrees));
                target.draw(sprite);
                return;
            }
        }
    }

    // --- Fallback: the old quarter assembly ---------------------------------
    //
    // Reached for a headless body segment, which has no whole-sprite equivalent
    // in the atlas, and for any colour whose full art is missing. Kept so a
    // different or partial atlas still renders something rather than nothing.
    const std::string key_head_l = "pipe_" + color + "_head_left";
    const std::string key_head_r = "pipe_" + color + "_head_right";
    const std::string key_body_l = "pipe_" + color + "_body_left";
    const std::string key_body_r = "pipe_" + color + "_body_right";

    std::vector<sf::Sprite> sprites;
    sprites.reserve(4);
    if (hasHead) {
        sprites.push_back(scenerySheet->getSprite(key_head_l));
        sprites.push_back(scenerySheet->getSprite(key_head_r));
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
    } else {
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
    }

    // Quadrant center offsets relative to tile center (0-degree base offsets)
    // Top-Left, Top-Right, Bottom-Left, Bottom-Right
    sf::Vector2f offsets[4] = {
        {-size.x * 0.25f, -size.y * 0.25f},
        { size.x * 0.25f, -size.y * 0.25f},
        {-size.x * 0.25f,  size.y * 0.25f},
        { size.x * 0.25f,  size.y * 0.25f}
    };

    float angleRad = rotationDegrees * (3.1415926535f / 180.0f);
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    sf::Vector2f halfSize(size.x * 0.5f, size.y * 0.5f);

    for (int i = 0; i < 4; ++i) {
        sf::FloatRect bounds = sprites[i].getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            sprites[i].setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
            sprites[i].setScale(sf::Vector2f(halfSize.x / bounds.size.x, halfSize.y / bounds.size.y));

            float rx = offsets[i].x * cosA - offsets[i].y * sinA;
            float ry = offsets[i].x * sinA + offsets[i].y * cosA;

            sprites[i].setPosition(center + sf::Vector2f(rx, ry));
            sprites[i].setRotation(sf::degrees(rotationDegrees));

            target.draw(sprites[i]);
        }
    }
}
