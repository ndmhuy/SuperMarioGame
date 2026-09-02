#pragma once

#include "Entities/Block.hpp"
#include "Graphics/PipeRenderer.hpp"
#include <string>

class Pipe : public Block {
public:
    // How this pipe is meant to be entered — and, because the atlas has
    // different art for each, what it looks like.
    //
    // Named for WHERE THE MOUTH IS, not which key opens it: `SideLeft` has its
    // opening on the pipe's west face, so a player walks RIGHT into it. An enum
    // rather than a pair of bools because the cases are mutually exclusive and
    // "isSide && !facesWest" is a state a reader has to decode rather than read.
    //
    // The two side modes exist because the atlas's L-bend frames are the only
    // art in the game that says "this pipe goes somewhere else": a shaft that
    // runs off the top of the frame with a mouth at floor level reads as
    // "enter here, it goes UP", which is exactly what a sub-level's way out is.
    // A top-entry pipe is the descending half of the same pair.
    enum class EntryMode {
        Top,        // classic: stand on the rim, press Down
        SideLeft,   // mouth on the west face, walked into by holding Right
        SideRight   // mouth on the east face, walked into by holding Left
    };

    // A warp pipe is 2 tiles wide and 4 tall.
    //
    // It used to be 2x2: a stub no taller than the player and indistinguishable
    // from the decorative pipe runs stamped into the tilemap, so nothing on
    // screen said "this one goes somewhere". checkWarp() reads the bounding box,
    // so the trigger follows the collider.
    //
    // Level data places a pipe by its TOP-left corner, so every authored and
    // generated warp pipe moved up two tiles when this changed — the seven level
    // JSONs and MapGenerator's two call sites.
    static constexpr float WIDTH_PX  = 64.0f;
    static constexpr float HEIGHT_PX = 128.0f;

    // How tall the horizontal arm of a side-entry pipe is: the band a player
    // has to be standing in to walk into it.
    //
    // Computed from the renderer's own fraction rather than restated as a
    // number, so the mouth you can ENTER is by construction the mouth that is
    // DRAWN. One tile at the shipped 4-tile collider — shorter than a full-size
    // player, which is a limit of the atlas art and not a bug: the trigger
    // below measures FEET, so every player form enters the same mouth.
    float mouthHeight() const {
        return boundingBox.height * PipeRenderer::L_BEND_MOUTH_HEIGHT_FRAC;
    }

    // `color` selects an atlas family: "green" (pipe_dark_green_*) or "white"
    // (pipe_white_black_*). PipeRenderer maps it; an unknown value falls back to
    // the quarter-sprite assembly rather than drawing nothing.
    explicit Pipe(sf::Vector2f position, int pipeId = 0, sf::Vector2f exitPosition = {0.0f, 0.0f}, std::string targetLevel = "", bool isEntrance = false, float rotationDegrees = 0.0f, std::string color = "green");
    ~Pipe() override = default;

    std::string getTypeName() const override { return "pipe"; }

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Whether this player is, right now, asking to go through this pipe.
    //
    // Deliberately side-effect free — see the comment at the definition — and
    // deliberately answers for the pipe's own EntryMode: a top-entry pipe wants
    // the player on its rim pressing Down, a side-entry pipe wants them on the
    // floor beside its mouth heading into it. Down does nothing to a side-entry
    // pipe: there is no opening on top of one.
    bool checkWarp(const Player& player) const;

    // The geometric half of checkWarp(): is this player in position to enter,
    // regardless of what they are pressing?
    //
    // Split out so the placement of every mouth in every shipped level is
    // testable without a keyboard. checkWarp() is this AND the right key held,
    // which is the part only a running game can answer.
    bool isAtEntryPoint(const Player& player) const;

    EntryMode getEntryMode() const { return m_entryMode; }
    void setEntryMode(EntryMode mode) { m_entryMode = mode; }
    bool isTopEntry() const { return m_entryMode == EntryMode::Top; }
    bool isSideEntry() const { return m_entryMode != EntryMode::Top; }

    // Centre of the opening a player disappears into, in world pixels. The
    // entry animation slides them towards it; isAtEntryPoint() measures from it.
    sf::Vector2f getMouthCenter() const;

    // Which way a player has to be heading to enter: +1 eastward (hold Right),
    // -1 westward (hold Left), 0 for a top-entry pipe, which is not entered
    // horizontally at all.
    float getSideApproachDirection() const;

    // How far this pipe's shaft is DRAWN above its collider, in pixels — how an
    // up-pipe visibly leaves the room through the ceiling rather than stopping
    // in mid-air, which was the whole complaint about the old art.
    //
    // Derived from the room's own geometry at load time (LevelLoader walks up to
    // the first solid tile) rather than authored, so it is deliberately NOT
    // serialised: a level file carrying its own copy would restate something the
    // tilemap already says, and would be wrong the first time a ceiling moved.
    float getShaftRise() const { return m_shaftRise; }
    void setShaftRise(float pixels);

    // Pipes are drawn by PipeRenderer from several atlas frames rather than by
    // the animator, so the inherited animator-based answers do not describe
    // them. Reported honestly here so the artwork sweep is not lied to.
    bool hasArtwork() const override { return m_spriteSheet != nullptr; }
    sf::Vector2f artworkSize() const override {
        return m_spriteSheet ? sf::Vector2f{boundingBox.width, boundingBox.height}
                             : sf::Vector2f{0.0f, 0.0f};
    }

    int getPipeId() const { return m_pipeId; }
    sf::Vector2f getExitPosition() const { return m_exitPosition; }
    std::string getTargetLevel() const { return m_targetLevel; }
    bool isEntrance() const { return m_isEntrance; }
    float getRotationDegrees() const { return m_rotationDegrees; }
    void setRotationDegrees(float deg) { m_rotationDegrees = deg; }
    const std::string& getColor() const { return m_color; }

    // Re-author where this pipe goes, for the level editor's Inspector.
    //
    // One call rather than four setters because the four fields are only
    // meaningful together: an entrance with no destination warps into nothing,
    // and a destination on a pipe that is not an entrance is never read. All
    // four have been in the level schema since it was written and none of them
    // could be authored anywhere but a text editor.
    //
    // `exitPosition` is in WORLD pixels, like getExitPosition(); the level JSON
    // stores the same value in tiles.
    void configureWarp(int pipeId, bool isEntrance, std::string targetLevel,
                       sf::Vector2f exitPosition);

private:
    int m_pipeId = 0;
    sf::Vector2f m_exitPosition;
    std::string m_targetLevel;
    bool m_isEntrance = false;
    float m_rotationDegrees = 0.0f;
    std::string m_color = "green";
    EntryMode m_entryMode = EntryMode::Top;
    float m_shaftRise = 0.0f;
    const SpriteSheet* m_spriteSheet = nullptr;

    // The art shape this pipe's EntryMode calls for.
    PipeRenderer::Shape artShape() const;
};
