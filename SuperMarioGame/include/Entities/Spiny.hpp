#pragma once

#include <string>
#include <vector>

#include "Entities/Enemy.hpp"

class Spiny : public Enemy {
public:
    // Default is true, not false.
    //
    // Every real spawn site — EntityCatalogue's make<Spiny> (used by both
    // LevelLoader and Lakitu's drop, since EntityFactory::create() takes only
    // a type and a position) and MapGenerator's procedural placement — calls
    // this single-argument overload. Nothing in production ever passed `true`
    // or called setEgg(true), so per SPEC §6.1 ("walks on ground after
    // hatching from egg") the egg state existed in the class and was
    // permanently dead code: a Lakitu-dropped Spiny appeared already hatched
    // and walking in mid-air instead of falling as an egg and hatching on
    // ground contact.
    //
    // The correct wire is one line deeper than this file reaches: Lakitu
    // publishes an EntitySpawnRequested event (EntitySpawnRequest has no egg
    // flag) that PlayingState's handler turns into a Spiny via
    // EntityFactory::create(); neither EntitySpawnRequest (Core/GameSnapshot.hpp)
    // nor PlayingState.cpp nor EntityCatalogue.cpp is a file this lane owns.
    // Flipping the default here reaches every real construction path without
    // touching any of them, and matches SPEC's roster entry, which describes
    // hatching-from-egg as how a Spiny comes into being at all, not as a
    // Lakitu-only special case. Explicit `false` remains available for a
    // caller that deliberately wants an already-hatched Spiny.
    explicit Spiny(sf::Vector2f position, bool isEgg = true);
    ~Spiny() override;

    // How many Spinies are alive in the world right now.
    //
    // Lakitu's drop limit has to be a *concurrent* cap, not a lifetime one: as a
    // lifetime cap it quietly retired any Lakitu that had thrown its three eggs
    // before the player ever arrived, which is the reported "Lakitu sometimes
    // doesn't drop the Spiny" (R21 D8). An entity cannot reach the world's
    // entity list, and the drop leaves as an EventBus spawn request that never
    // comes back, so the population is counted here instead — at construction
    // and destruction, which is the one place that sees every Spiny however it
    // was created.
    //
    // Process-wide by necessity rather than by preference. The levels place one
    // Lakitu each, so it is exact; two Lakitus in one level would share an
    // allowance, which is the conservative direction to be wrong in.
    static int liveCount();

    std::string getTypeName() const override { return "spiny"; }

    void onStomped() override;
    void onHitByFireball() override;
    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    bool isCollidable() const override;

    bool isEgg() const { return m_isEgg; }
    void setEgg(bool isEgg) { m_isEgg = isEgg; }

private:
    // isFlipped()/m_isFlipped deliberately NOT redeclared here.
    //
    // This class used to shadow both with its own copy. onHitByFireball() then
    // set the *derived* flag while Enemy::isDeadOrDying() and
    // Enemy::collidesWithTiles() kept reading the base one — so a fireball
    // launched this enemy into the air and it never actually died, and it went
    // on colliding with tiles the whole time. That is why "the fireball must
    // kill the enemy": it hit, it knocked them back, and nothing else happened.
    bool m_isEgg = false;
    Animation m_eggAnim;

    // Every constructed Spiny, registered by its own constructor. Raw
    // back-pointers, owned by nothing: an entry is removed by the destructor of
    // the object it points at, so it cannot outlive it.
    static std::vector<Spiny*> s_live;
};
