#pragma once

#include "Graphics/BackgroundRenderer.hpp"
#include "Physics/AABB.hpp"
#include "Utils/TileMap.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <string>

class SpriteSheet;

// Draws a TileMap from the world/scenery atlas, themed.
//
// Lifted out of PlayingState::render(), which was the only place that knew how a
// TileType becomes a sprite — roughly two hundred lines of switch plus the pipe
// run arithmetic. The level editor needs exactly the same answer, because an
// editor that paints the level in debug colours is not showing you the level you
// are building, and a second copy of that switch is precisely the hand-synced
// duplication g-rule-22 exists to prevent.
//
// It draws tiles and nothing else: no entities, no HUD, no grid.
class TileMapRenderer {
public:
    // Which atlas art one column of a tilemap pipe run is drawn from.
    //
    // `frame` is an atlas frame name. `sliceHeight` of 0 means "draw the whole
    // frame"; otherwise only the `sliceHeight` source rows starting at
    // `sliceTop` are drawn, which is how a 32x64 whole narrow pipe is split into
    // a rim tile and however many body tiles a run needs.
    struct PipeTileArt {
        std::string frame;
        int sliceTop = 0;
        int sliceHeight = 0;
    };

    // `sheet` may be null (no atlas loaded); every tile then falls back to its
    // TileInfo debug colour rather than drawing nothing.
    void setSpriteSheet(const SpriteSheet* sheet) { m_sheet = sheet; }
    void setTheme(BackgroundTheme theme) { m_theme = theme; }
    BackgroundTheme getTheme() const { return m_theme; }

    // Draws only the tiles `visibleBounds` covers, plus a tile of margin for the
    // liquid bob. Sweeping the whole grid was roughly 4,400 sprite draws per
    // frame on a 200-wide level (audit A-14).
    //
    // `animTimer` is the host's own accumulated seconds; passing it in rather
    // than keeping a clock here means the editor and the game animate in step
    // and a paused host freezes the coins.
    void render(sf::RenderTarget& target, const TileMap& tileMap,
                const AABB& visibleBounds, float animTimer) const;

    // The art for the pipe tile at (tileX, tileY).
    //
    // Guarantees that no tile is ever drawn from half a rim: the run's own
    // extent, not the left neighbour, decides this — see the comment at the
    // definition.
    static PipeTileArt pipeTileArtAt(const TileMap& tileMap, int tileX, int tileY);

private:
    const SpriteSheet* m_sheet = nullptr;
    BackgroundTheme m_theme = BackgroundTheme::Overworld;
};
