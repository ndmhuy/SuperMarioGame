#pragma once

#include <string>

#include "Entities/Block.hpp"

class QuestionBlock : public Block {
public:
    // What a question block holds. These ids appear verbatim as "itemType" in the
    // level JSON, so they are part of the save format — append, never renumber.
    //
    // Deliberately its own id space: Player::powerUp() uses a different set of
    // ids and has no notion of a coin. PlayingState's spawn listener maps between
    // them in exactly one place.
    enum Content : int {
        Coin        = 0,   // dispensed inline, no entity spawned
        Mushroom    = 1,
        FireFlower  = 2,
        CapeFeather = 3,
        Star        = 4,
        MiniMushroom= 5,
        MegaMushroom= 6,
        OneUp       = 7
    };

    explicit QuestionBlock(sf::Vector2f position, int itemType = Content::Coin);
    ~QuestionBlock() override = default;

    std::string getTypeName() const override { return "question_block"; }

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    int getItemType() const { return m_containedItemType; }
    int getContainedItemType() const { return m_containedItemType; }
    bool isEmpty() const { return m_isEmpty; }

    // Re-author what this block holds, for the level editor's Inspector.
    //
    // "itemType" has been in the level schema since the schema existed and was
    // settable only by hand-editing JSON, so every block anyone placed in the
    // editor dispensed a coin. Refuses an id outside Content rather than
    // storing one the spawn listener would silently drop.
    void setContents(int itemType);

private:
    int m_containedItemType = 0;
    bool m_isEmpty = false;

    // Needed to draw the spent look directly (D24): once m_isEmpty, this stops
    // playing the blinking "?" animation and instead shows a plain solid block
    // — same idea as the sub-level's TileType::Question -> TileType::Used tile
    // swap in PhysicsEngine.cpp, and what SPEC.md's Blocks table means by
    // "Becomes empty block".
    const SpriteSheet* m_spriteSheet = nullptr;
};
