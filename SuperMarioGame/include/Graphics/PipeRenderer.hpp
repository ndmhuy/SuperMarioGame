#pragma once

#include <SFML/Graphics.hpp>
#include "Graphics/SpriteSheet.hpp"
#include <string>

class PipeRenderer {
public:
    // What shape a pipe's art takes, which follows from how it is entered.
    //
    // A renderer-side enum rather than a reuse of Pipe::EntryMode on purpose:
    // Graphics must not depend on Entities, and the tilemap's decorative pipe
    // runs have a shape but no entry behaviour at all.
    enum class Shape {
        VerticalTop,     // rimmed vertical shaft — stood on, entered downward
        LBendMouthWest,  // horizontal mouth at the bottom-left, shaft rising
        LBendMouthEast   // the same, mirrored
    };

    // How much of an L-bend's height is its horizontal arm — the part a player
    // walks into — as a fraction of the drawn box.
    //
    // 32 of the 128 rows of pipe_dark_green_long_l, measured off the atlas. The
    // bend is drawn at the frame's own aspect, so this IS the drawn arm rather
    // than a figure chosen to match it: declaring it here, in the class that
    // draws it, and having Pipe compute its trigger band from it is what keeps
    // the mouth you can enter and the mouth you can see the same rectangle.
    // Two independently-written 64s is exactly the drift g-rule-22 forbids.
    static constexpr float L_BEND_MOUTH_HEIGHT_FRAC = 32.0f / 128.0f;

    // Which atlas art one 32px cell of a vertical pipe is drawn from.
    //
    // `sliceHeight` of 0 means "the whole frame"; otherwise only the
    // `sliceHeight` source rows starting at `sliceTop` are used, which is how a
    // 32x64 whole narrow pipe is split into a rim cell and body cells.
    struct TileArt {
        std::string frame;
        int sliceTop = 0;
        int sliceHeight = 0;
    };

    // The art for one cell of a vertical pipe `runWidth` cells wide, at
    // `columnOffset` cells from its left edge, on the rim row or below it.
    //
    // The ONE place this decision is taken. TileMapRenderer::pipeTileArtAt asks
    // it for a tilemap run and draw() asks it for a Pipe entity's collider, so a
    // 2-cell-wide entity pipe and a 2-tile-wide tilemap run cannot drift apart.
    // They used to be two row-tilers: the tilemap's composed halves correctly
    // while the entity's stamped four quadrants at +-size*0.25, which is only
    // geometrically right for a square box — so growing the collider to 2x4
    // turned the entity path into garbage while the tilemap path stayed fine.
    //
    // Guarantees no cell is ever half a rim: see the comment at the definition.
    static TileArt cellArt(int columnOffset, int runWidth, bool isRimRow,
                           const std::string& color = "green");

    // `shaftRisePx` extends an L-bend's vertical shaft that far ABOVE
    // `position` — how a sub-level's up-pipe visibly leaves the room through
    // its ceiling instead of stopping in mid-air. Ignored by VerticalTop.
    //
    // `size` is the pipe's collider, and the art fills it: the sprite and the
    // thing the player collides with are deliberately the same rectangle. The
    // rising shaft is the one exception, and it is scenery — the collider ends
    // at the room, so nothing above `position` is solid.
    static void draw(sf::RenderTarget& target,
                     const SpriteSheet* scenerySheet,
                     sf::Vector2f position,
                     sf::Vector2f size,
                     float rotationDegrees = 0.0f,
                     Shape shape = Shape::VerticalTop,
                     const std::string& color = "green",
                     float shaftRisePx = 0.0f);
};
