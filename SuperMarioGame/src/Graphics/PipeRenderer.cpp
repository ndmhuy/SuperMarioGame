#include "Graphics/PipeRenderer.hpp"
#include "Utils/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <string>

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

// --- What the L-bend frames actually contain -------------------------------
//
// `pipe_dark_green_long_l` and `_short_l` are L-BENDS. The `_l` is the bend,
// not "left" — measured by cropping world_scenery_item.png, not inferred from
// the name, because the name is exactly what made the R21 batch pick long_l for
// a top-entry pipe and ship a warp pipe with no rim to stand on.
//
// In the 62x128 `long_l` frame:
//   * the vertical shaft occupies source x [34,62) over the frame's full height
//     and is UNIFORM down its length, so stretching it vertically is
//     pixel-identical to tiling it — which is why the rising shaft below is one
//     stretched quad rather than a loop;
//   * the horizontal arm occupies source y [96,128) across the full width, with
//     its open mouth capped at the left edge.
//
// Held as fractions of the frame so both halves are placed from the SAME
// mapping: a hardcoded 34px would put the shaft 3px off the arm's own shaft at
// the bend, and any atlas re-pack would silently break it.
constexpr float kShaftLeftFrac  = 34.0f / 62.0f;
constexpr float kShaftWidthFrac = 28.0f / 62.0f;
constexpr float kShaftSrcTopFrac    = 32.0f / 128.0f;
constexpr float kArmSrcTopFrac      = 96.0f / 128.0f;
constexpr float kBendSrcHeightFrac  = 32.0f / 128.0f;

// A sub-rectangle of an atlas frame, by fraction of the frame.
sf::IntRect subFrame(const sf::IntRect& frame, float xFrac, float wFrac,
                     float yFrac, float hFrac) {
    sf::IntRect r = frame;
    r.position.x += static_cast<int>(std::lround(frame.size.x * xFrac));
    r.position.y += static_cast<int>(std::lround(frame.size.y * yFrac));
    r.size.x = std::max(1, static_cast<int>(std::lround(frame.size.x * wFrac)));
    r.size.y = std::max(1, static_cast<int>(std::lround(frame.size.y * hFrac)));
    return r;
}

// Places `sprite` so its texture rect fills the given rectangle of pipe-local
// space, then draws it under the shared transform.
void drawStretched(sf::RenderTarget& target, sf::Sprite sprite,
                   sf::Vector2f at, sf::Vector2f box, const sf::RenderStates& states) {
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;
    if (box.x <= 0.0f || box.y <= 0.0f) return;
    sprite.setScale({box.x / bounds.size.x, box.y / bounds.size.y});
    sprite.setPosition(at);
    target.draw(sprite, states);
}

} // namespace

PipeRenderer::TileArt PipeRenderer::cellArt(int columnOffset, int runWidth,
                                            bool isRimRow, const std::string& color) {
    // pipe_green_head_left/right and pipe_green_body_left/right are 16x16
    // HALVES of a pipe, each stretched to a full 32px cell at the draw site.
    // Columns therefore pair off from the run's left edge: each pair composes
    // one whole 2-cell-wide pipe. A column left unpaired — an odd run's last
    // column, or a 1-wide run — cannot be drawn from a half without showing
    // half a rim (the reported "half pipe"), so it is drawn from
    // pipe_*_long_up, which is a COMPLETE 32px-wide pipe. A half rim is
    // therefore not representable here, whatever the level data says
    // (R21-D1, after D22/D23).
    if (columnOffset == runWidth - 1 && (runWidth % 2) == 1) {
        // pipe_dark_green_long_up is 32x64: one cell of rim over one of body,
        // so a narrow pipe of any height is built from whole art.
        TileArt art;
        art.frame = "pipe_" + wholeFamilyFor(color) + "_long_up";
        art.sliceTop    = isRimRow ? 0 : 32;
        art.sliceHeight = 32;
        return art;
    }

    // Only green, red and purple ship halves; a colour without them is caught
    // by draw(), which retries in green rather than drawing nothing.
    const bool rightHalf = (columnOffset % 2) == 1;
    TileArt art;
    art.frame = "pipe_" + color +
                (isRimRow ? (rightHalf ? "_head_right" : "_head_left")
                          : (rightHalf ? "_body_right" : "_body_left"));
    return art;
}

void PipeRenderer::draw(sf::RenderTarget& target,
                        const SpriteSheet* scenerySheet,
                        sf::Vector2f position,
                        sf::Vector2f size,
                        float rotationDegrees,
                        Shape shape,
                        const std::string& color,
                        float shaftRisePx) {
    if (!scenerySheet) return;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    // --- One transform for the whole assembly -------------------------------
    //
    // A pipe is drawn from several sprites, and giving each its own rotation
    // about its own centre is what used to leave hairline seams between the
    // quadrants and visibly mismatched corners on a rotated pipe: independently
    // transformed sprites cannot stay flush. Composing ONE rotation into the
    // RenderStates instead means the pieces are laid out in the pipe's own
    // local space (origin at its top-left, extent `size`) and rotated together.
    //
    // An east-facing L-bend is the west-facing one mirrored about the box
    // centre, so the mirror lives here too and the layout code below only ever
    // has one handedness to describe.
    sf::Transform xf;
    xf.translate(position + size * 0.5f);
    if (rotationDegrees != 0.0f) xf.rotate(sf::degrees(rotationDegrees));
    if (shape == Shape::LBendMouthEast) xf.scale({-1.0f, 1.0f});
    xf.translate(-size * 0.5f);
    const sf::RenderStates states(xf);

    if (shape == Shape::VerticalTop) {
        const float tile = Constants::TILE_SIZE;
        const int cols = std::max(1, static_cast<int>(std::lround(size.x / tile)));
        const int rows = std::max(1, static_cast<int>(std::lround(size.y / tile)));
        const float cellW = size.x / static_cast<float>(cols);
        const float cellH = size.y / static_cast<float>(rows);

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                TileArt art = cellArt(col, cols, row == 0, color);
                if (!scenerySheet->hasFrame(art.frame)) {
                    art = cellArt(col, cols, row == 0, "green");
                    if (!scenerySheet->hasFrame(art.frame)) continue;
                }
                sf::Sprite cell = scenerySheet->getSprite(art.frame);
                if (art.sliceHeight > 0) {
                    // Applied before getLocalBounds(), which reports the rect.
                    sf::IntRect rect = cell.getTextureRect();
                    rect.position.y += art.sliceTop;
                    rect.size.y = art.sliceHeight;
                    cell.setTextureRect(rect);
                }
                drawStretched(target, cell,
                              {col * cellW, row * cellH}, {cellW, cellH}, states);
            }
        }
        return;
    }

    // --- L-bend: a horizontal mouth at floor level, and a shaft that leaves --
    const std::string bendFrame = "pipe_" + wholeFamilyFor(color) + "_long_l";
    if (!scenerySheet->hasFrame(bendFrame)) {
        // No L art in this atlas. A rimmed vertical pipe is wrong about the
        // direction but right about being a pipe, which beats drawing nothing.
        draw(target, scenerySheet, position, size, rotationDegrees,
             Shape::VerticalTop, color, 0.0f);
        return;
    }

    const sf::IntRect frame = scenerySheet->getSprite(bendFrame).getTextureRect();

    // The shaft continuation first, so the bend below paints over where they
    // meet rather than leaving a seam at the join.
    if (shaftRisePx > 0.0f) {
        sf::Sprite shaft = scenerySheet->getSprite(bendFrame);
        shaft.setTextureRect(subFrame(frame, kShaftLeftFrac, kShaftWidthFrac,
                                      kShaftSrcTopFrac, kBendSrcHeightFrac));
        drawStretched(target, shaft,
                      {size.x * kShaftLeftFrac, -shaftRisePx},
                      {size.x * kShaftWidthFrac, shaftRisePx},
                      states);
    }

    // The bend itself fills the collider, at the frame's own aspect — one
    // sprite for one hitbox, the same rule the vertical case follows.
    //
    // NOT stretched to make the arm taller. The first attempt drew the arm two
    // tiles tall so a full-size player would visibly fit through the mouth,
    // which doubled every line in the bore and turned a legible elbow into two
    // chunky blocks; the frame only reads as an L-bend at the proportions it
    // was drawn in. The trigger follows the art instead of the reverse — see
    // L_BEND_MOUTH_HEIGHT_FRAC — and it tests the player's FEET, so Small and
    // Super both enter a one-tile mouth even though Super is taller than it.
    sf::Sprite bend = scenerySheet->getSprite(bendFrame);
    drawStretched(target, bend, {0.0f, 0.0f}, size, states);
}
