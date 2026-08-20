#pragma once

#include <string>

#include "Entities/Enemy.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

class PiranhaPlant : public Enemy {
public:
    explicit PiranhaPlant(sf::Vector2f position);
    ~PiranhaPlant() override = default;

    std::string getTypeName() const override { return "piranha_plant"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    // Toothed: onStomped() damages the player.
    bool isStompSafe() const override { return false; }
    void onHitByFireball() override;

    // Drawn clipped to the part that has actually left the pipe. The plant used
    // to be drawn whole at its anchor while "retracted", so it sat in full view
    // on the pipe mouth the entire time it was supposed to be hidden.
    void render(sf::RenderTarget& target) override;

    // Nothing inside the pipe can bite. Without this the invisible-but-present
    // plant still damaged anyone standing on the pipe.
    bool isCollidable() const override;

    // How many pixels of the plant are currently outside the pipe. Public so a
    // test can watch the whole emergence cycle rather than eyeball it.
    float artworkVisibleHeight() const { return emergedHeight(); }

    // Anchored in its pipe: neither gravity nor the tilemap moves it, the
    // emergence strategy does. Public because they are part of what this entity
    // reports about itself, and a test cannot ask through a private override.
    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }

private:
    float emergedHeight() const;
};
