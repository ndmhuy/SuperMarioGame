#include "Graphics/TileMapRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>

TileMapRenderer::PipeTileArt TileMapRenderer::pipeTileArtAt(const TileMap& tileMap,
                                                            int tileX, int tileY) {
    // pipe_green_head_left/right and pipe_green_body_left/right are 16x16 HALVES
    // of a pipe, each stretched to a full tile at the draw site. Which half a
    // column shows used to be decided by "is my left neighbour a pipe?", which
    // is not the same question: it makes the first column of every run a LEFT
    // half and every later column a RIGHT half. A one-column run therefore drew
    // the left half of a rim at 2x — the reported half pipe — and a three-column
    // run drew L,R,R.
    //
    // The run's own extent answers it properly: columns pair off from the run
    // start, and any column left unpaired (an odd-width run's last column, or a
    // one-column run) is drawn from pipe_dark_green_up / _long_up, which are
    // COMPLETE 32px-wide pipes rather than halves. A half rim is therefore not
    // representable here, whatever the level data says (R21-D1, after D22/D23).
    int runStart = tileX;
    while (runStart > 0 && tileMap.getTileType(runStart - 1, tileY) == TileType::Pipe) --runStart;
    int runEnd = tileX;
    while (runEnd + 1 < tileMap.getWidth() &&
           tileMap.getTileType(runEnd + 1, tileY) == TileType::Pipe) ++runEnd;

    const bool isTopExposed = (tileY == 0) ||
                              (tileMap.getTileType(tileX, tileY - 1) != TileType::Pipe);
    const int offset   = tileX - runStart;
    const int runWidth = runEnd - runStart + 1;

    if (offset == runWidth - 1 && (runWidth % 2) == 1) {
        // pipe_dark_green_long_up is 32x64: rim on top, body underneath. One
        // tile of each, so a narrow pipe of any height is built from whole art.
        PipeTileArt art;
        art.frame = "pipe_dark_green_long_up";
        art.sliceTop = isTopExposed ? 0 : 32;
        art.sliceHeight = 32;
        return art;
    }

    const bool isRightHalf = (offset % 2) == 1;
    PipeTileArt art;
    if (isTopExposed) {
        art.frame = isRightHalf ? "pipe_green_head_right" : "pipe_green_head_left";
    } else {
        art.frame = isRightHalf ? "pipe_green_body_right" : "pipe_green_body_left";
    }
    return art;
}

void TileMapRenderer::render(sf::RenderTarget& target, const TileMap& tileMap,
                             const AABB& visibleBounds, float animTimer) const {
    const int coinFrame     = static_cast<int>(animTimer / 0.15f) % 4;
    const int questionFrame = static_cast<int>(animTimer / 0.20f) % 4;
    // Two-frame wave cycle shared by water and lava surfaces.
    const int waveFrame     = static_cast<int>(animTimer / 0.35f) % 2;

    const int firstX = std::max(0,
        static_cast<int>(std::floor(visibleBounds.x / Constants::TILE_SIZE)) - 1);
    const int lastX  = std::min(tileMap.getWidth() - 1,
        static_cast<int>(std::floor((visibleBounds.x + visibleBounds.width) / Constants::TILE_SIZE)) + 1);
    const int firstY = std::max(0,
        static_cast<int>(std::floor(visibleBounds.y / Constants::TILE_SIZE)) - 1);
    const int lastY  = std::min(tileMap.getHeight() - 1,
        static_cast<int>(std::floor((visibleBounds.y + visibleBounds.height) / Constants::TILE_SIZE)) + 1);

    // Bottom-to-top, so an overlapping/bobbing top layer draws over the row
    // beneath it rather than under it.
    for (int y = lastY; y >= firstY; --y) {
        for (int x = firstX; x <= lastX; ++x) {
            const TileType tileType = tileMap.getTileType(x, y);
            if (tileType == TileType::Empty) continue;

            const sf::Vector2f tilePos(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE);
            bool spriteDrawn = false;

            if (m_sheet) {
                std::string frameKey;
                PipeTileArt pipeArt;

                switch (tileType) {
                    case TileType::Ground: {
                        // Themed terrain. This used to be brown-on-grey for every
                        // level in the game, so an ice cavern and a castle floor
                        // and a grass field all looked identical — and a
                        // generated level looked like nothing in particular.
                        const bool isTopExposed =
                            (y == 0) || (tileMap.getTileType(x, y - 1) == TileType::Empty);
                        switch (m_theme) {
                            case BackgroundTheme::Underground:
                                frameKey = isTopExposed ? "solid_block_grey" : "brick_grey_inside";
                                break;
                            case BackgroundTheme::Castle:
                                frameKey = isTopExposed ? "castle_brick_white" : "brick_grey_inside";
                                break;
                            case BackgroundTheme::Ice:
                                frameKey = isTopExposed ? "solid_block_blue" : "brick_blue_inside";
                                break;
                            case BackgroundTheme::Overworld:
                            default:
                                frameKey = isTopExposed ? "solid_block_brown" : "brick_brown_inside";
                                break;
                        }
                        break;
                    }
                    case TileType::Brick:
                        frameKey = (m_theme == BackgroundTheme::Ice)
                                       ? "brick_blue_one_side"
                                       : (m_theme == BackgroundTheme::Underground ||
                                          m_theme == BackgroundTheme::Castle)
                                             ? "brick_grey_one_side"
                                             : "brick_brown_side";
                        break;
                    case TileType::Question:
                        frameKey = "question_block_" + std::to_string(questionFrame % 3);
                        break;
                    case TileType::Pipe: {
                        pipeArt = pipeTileArtAt(tileMap, x, y);
                        frameKey = pipeArt.frame;
                        break;
                    }
                    case TileType::Ice:
                        // No dedicated ice sprite in world_scenery — use solid_block_blue as fallback
                        frameKey = "solid_block_blue";
                        break;
                    case TileType::Conveyor:
                        frameKey = "conveyor_belt_green";
                        break;
                    case TileType::Water: {
                        const bool isSurface = (y == 0) || (tileMap.getTileType(x, y - 1) != TileType::Water);
                        // The surface alternates between the two wave frames the
                        // atlas ships, so it actually moves rather than only
                        // bobbing up and down (task 5.10).
                        frameKey = isSurface
                            ? (waveFrame == 0 ? "water_dark_blue_wave_long" : "water_light_blue_wave_long")
                            : "water_dark_blue_bg";
                        break;
                    }
                    case TileType::Lava: {
                        const bool isSurface = (y == 0) || (tileMap.getTileType(x, y - 1) != TileType::Lava);
                        frameKey = isSurface
                            ? (waveFrame == 0 ? "lava_wave_long" : "lava_wave_short")
                            : "lava_bg";
                        break;
                    }
                    case TileType::Coin:
                        frameKey = "coin_" + std::to_string(coinFrame % 2);
                        break;
                    default:
                        break;
                }

                if (!frameKey.empty()) {
                    sf::Sprite tileSprite = m_sheet->getSprite(frameKey);
                    if (pipeArt.sliceHeight > 0) {
                        // Narrow-pipe art is taller than one tile, so a column
                        // takes a horizontal slice of it. Applied before
                        // getLocalBounds(), which reports the texture rect.
                        sf::IntRect rect = tileSprite.getTextureRect();
                        rect.position.y += pipeArt.sliceTop;
                        rect.size.y = pipeArt.sliceHeight;
                        tileSprite.setTextureRect(rect);
                    }
                    const auto bounds = tileSprite.getLocalBounds();
                    if (bounds.size.x > 0 && bounds.size.y > 0) {
                        tileSprite.setScale(sf::Vector2f(
                            Constants::TILE_SIZE / bounds.size.x,
                            Constants::TILE_SIZE / bounds.size.y
                        ));
                        sf::Vector2f drawPos = tilePos;
                        const bool liquidSurface =
                            (tileType == TileType::Water &&
                             (y == 0 || tileMap.getTileType(x, y - 1) != TileType::Water)) ||
                            (tileType == TileType::Lava &&
                             (y == 0 || tileMap.getTileType(x, y - 1) != TileType::Lava));
                        if (liquidSurface) {
                            // Pure vertical bobbing (started 5px lower to prevent exposing a top gap)
                            const float bobY = std::sin(animTimer * 3.0f) * 2.5f;
                            drawPos.y += 5.0f + bobY;
                        }
                        tileSprite.setPosition(drawPos);
                        target.draw(tileSprite);
                        spriteDrawn = true;
                    }
                }
            }

            // Fallback to debug color rectangles if no atlas loaded or unknown tile
            if (!spriteDrawn) {
                const TileInfo& info = TileMap::getInfo(tileType);
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(tilePos);
                tileShape.setFillColor(info.debugColor);
                tileShape.setOutlineColor(sf::Color(60, 40, 20));
                tileShape.setOutlineThickness(0.5f);
                target.draw(tileShape);

                if (tileType == TileType::Ground) {
                    sf::RectangleShape grassShape(sf::Vector2f(Constants::TILE_SIZE, 6.0f));
                    grassShape.setPosition(tilePos);
                    grassShape.setFillColor(sf::Color(46, 139, 87));
                    target.draw(grassShape);
                }
            }
        }
    }
}
