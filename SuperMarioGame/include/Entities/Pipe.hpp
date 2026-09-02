#pragma once

#include "Entities/Block.hpp"
#include <string>

class Pipe : public Block {
public:
    // A warp pipe is 2 tiles wide and 4 tall.
    //
    // It used to be 2x2, which PipeRenderer draws from the 64x64 whole-pipe
    // frame: a stub no taller than the player and indistinguishable from the
    // decorative pipe runs stamped into the tilemap, so nothing on screen said
    // "this one goes somewhere". At 2x4 PipeRenderer::wholeFrameFor crosses its
    // `size.y >= size.x * 1.75` test and picks the 62x128 double-height art
    // instead. checkWarp() reads the bounding box, so the trigger follows.
    //
    // Level data places a pipe by its TOP-left corner, so every authored and
    // generated warp pipe moved up two tiles when this changed — the seven level
    // JSONs and MapGenerator's two call sites.
    static constexpr float WIDTH_PX  = 64.0f;
    static constexpr float HEIGHT_PX = 128.0f;

    // `color` selects an atlas family: "green" (pipe_dark_green_*) or "white"
    // (pipe_white_black_*). PipeRenderer maps it; an unknown value falls back to
    // the quarter-sprite assembly rather than drawing nothing.
    explicit Pipe(sf::Vector2f position, int pipeId = 0, sf::Vector2f exitPosition = {0.0f, 0.0f}, std::string targetLevel = "", bool isEntrance = false, float rotationDegrees = 0.0f, std::string color = "green");
    ~Pipe() override = default;

    std::string getTypeName() const override { return "pipe"; }

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Checks if warp conditions (player standing on top and pressing down) are met
    bool checkWarp(const Player& player) const;

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
    const SpriteSheet* m_spriteSheet = nullptr;
};
