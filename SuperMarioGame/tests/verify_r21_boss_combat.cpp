// verify_r21_boss_combat.cpp — the three rules of the Bowser fight.
//
// Every case here corresponds to a live defect reported against the 1-3 fight:
//
//   1. "the Bowser stun by fireball is not working ... it does not receive
//      fireball" — the chain itself was intact, but a bursting Fireball stayed
//      collidable for the three frames of its burst animation, so one shot
//      counted as three of the four hits an opening costs and the mechanic never
//      behaved as its own constants describe.
//   2. "... but can be stomped currently" — Boss::onPlayerTouch paid out for any
//      genuine descending impact, which is Boom Boom's fight and not Bowser's.
//   3. "when the bridge is cut off, the bowser should drop into lava and lose
//      health gradually, not just disappearing" — chopBridge() called
//      defeatNow(), which is instant.
//
// The fireball case is driven through the pipeline the game actually uses
// (PhysicsEngine broadphase -> CollisionResolver::resolveEntityVsEntity ->
// category dispatch -> Fireball::onHitEnemy) against level_3's own tilemap and
// the Bowser the level loader placed, because every previous version of this
// mechanic's coverage called a resolver method directly and so could not have
// caught either half of defect 1.
//
// Run via:  ctest -R boss_combat --output-on-failure

#include "Core/Game.hpp"
#include "Entities/BoomBoom.hpp"
#include "Entities/Boss.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Mario.hpp"
#include "Physics/CollisionResolver.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    if (condition) {
        std::cout << "  [PASS] " << what << "\n";
    } else {
        std::cout << "  [FAIL] " << what << "\n";
        ++g_failures;
    }
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

// Levels live relative to the source tree; the test binary may run from build/.
std::string levelPath(const std::string& name) {
    const std::vector<std::string> roots = {
        "assets/levels/", "../assets/levels/", "../../assets/levels/",
        "SuperMarioGame/assets/levels/"
    };
    for (const auto& r : roots) {
        if (std::filesystem::exists(r + name)) return r + name;
    }
    return "assets/levels/" + name;
}

constexpr float DT = 1.0f / 60.0f;

// The 1-3 arena, assembled the way the game assembles it: level_3's tilemap
// installed on Game (so TerrainProbe and the lava query see it), the Bowser the
// level loader placed (so his arena, spawn and pacing bounds are the level's),
// and Fire Mario standing on the bridge beside him.
//
// Not a hand-built map: both halves of defect 1 depend on real geometry — where
// the bridge is, where the enclosure posts added in 384250f sit, and how far a
// shot has to travel — and a synthetic scene would have proved nothing about any
// of it.
struct ArenaScene {
    TileMap map;
    LevelData data;
    std::vector<std::unique_ptr<Entity>> entities;
    PhysicsEngine physics;
    Bowser* bowser = nullptr;
    Mario* mario = nullptr;
    bool ok = false;

    explicit ArenaScene(float marioTileX = 182.0f) {
        LevelLoader loader;
        if (!loader.loadLevel(levelPath("level_3.json"), map, data)) return;
        Game::getInstance().setTileMap(&map);

        for (auto& e : data.entities) {
            if (dynamic_cast<Bowser*>(e.get())) {
                bowser = static_cast<Bowser*>(e.get());
                entities.push_back(std::move(e));
                break;
            }
        }
        if (!bowser) return;

        auto owned = std::make_unique<Mario>(
            sf::Vector2f{marioTileX * Constants::TILE_SIZE, 18.0f * Constants::TILE_SIZE});
        mario = owned.get();
        mario->setStartingForm(Player::Form::Fire);
        entities.push_back(std::move(owned));

        // Let both settle onto the bridge before anything is measured.
        step(40);
        ok = true;
    }

    ~ArenaScene() { Game::getInstance().setTileMap(nullptr); }

    void step(int frames) {
        for (int f = 0; f < frames; ++f) {
            for (auto& e : entities) if (e) e->update(DT);
            physics.update(entities, map, DT);
        }
    }

    // A shot fired with exactly the parameters PlayingState's PlayerShotFireball
    // listener uses, aimed at whichever side Bowser is on.
    Fireball* throwFireball() {
        const float dir = (bowser->getPosition().x >= mario->getPosition().x) ? 1.0f : -1.0f;
        auto owned = std::make_unique<Fireball>(
            mario->getPosition() + sf::Vector2f(dir * 16.0f, 8.0f),
            sf::Vector2f(dir * 350.0f, 50.0f));
        Fireball* fb = owned.get();
        entities.push_back(std::move(owned));
        return fb;
    }

    int fireHits() const {
        return Bowser::FIRE_HITS_PER_STAGGER - bowser->getFireHitsToStagger();
    }
};

// ---------------------------------------------------------------------------
// Defect 1 — fire reaches Bowser, and one shot is worth exactly one hit.
// ---------------------------------------------------------------------------
void testFireballRegistersOnBowserAndStaggersHim() {
    section("D1  fire lands on Bowser through the real pipeline, four shots open him");

    ArenaScene scene;
    if (!scene.ok) { check(false, "level_3's arena and its Bowser load"); return; }

    check(scene.bowser->hasArena(), "the level gives Bowser an arena to be fought in");
    const int healthBefore = scene.bowser->getHealth();

    // One shot first, on its own, because "one shot, one hit" is the half of
    // this defect that no assertion covered: a bursting Fireball stayed
    // collidable and re-hit him on every frame of its own burst.
    Fireball* first = scene.throwFireball();
    int framesToHit = -1;
    for (int f = 0; f < 90; ++f) {
        scene.step(1);
        if (scene.fireHits() > 0 && framesToHit < 0) framesToHit = f;
        if (!first->isActive()) break;
    }
    check(framesToHit >= 0, "a fireball thrown at Bowser inside the arena reaches him");
    check(scene.fireHits() == 1,
          "and counts as exactly one hit, not one per frame of its burst (got " +
              std::to_string(scene.fireHits()) + ")");
    check(scene.bowser->getHealth() == healthBefore,
          "fire costs him no health — he breathes the stuff");
    check(!scene.bowser->isStaggered(), "one shot is not an opening");

    // Then the remaining three, which is what the constant says an opening costs.
    for (int shot = 2; shot <= Bowser::FIRE_HITS_PER_STAGGER; ++shot) {
        Fireball* fb = scene.throwFireball();
        for (int f = 0; f < 90 && fb->isActive(); ++f) scene.step(1);
    }
    check(scene.bowser->isStaggered(),
          "the fourth fireball staggers him, exactly as FIRE_HITS_PER_STAGGER says");
    check(scene.bowser->getHealth() == healthBefore,
          "and still costs him no health directly");
}

// ---------------------------------------------------------------------------
// Defect 2 — the stagger gate. THIS is the mutation-tested assertion: reverting
// Bowser::canBeStompedWhileGuarded() to Boss's default `true` must fail the
// first check below and the health check beside it.
// ---------------------------------------------------------------------------
void testBowserCannotBeStompedWhileGuarded() {
    section("D2  only a reeling Bowser can be stomped");

    TileMap map;
    map.initialize(60, 24);
    for (int x = 0; x < 60; ++x) map.setTile(x, 22, TileType::Ground);
    Game::getInstance().setTileMap(&map);

    CollisionResolver resolver;
    CollisionInfo fromAbove;
    fromAbove.collided = true;
    fromAbove.normal = sf::Vector2f(0.0f, -1.0f);

    // A genuine descending impact: feet on his crown, falling fast enough to
    // clear Boss::STOMP_MIN_DESCENT_SPEED.
    auto stompFrom = [&resolver, &fromAbove](Mario& player, Boss& boss) {
        player.setPosition({boss.getPosition().x,
                            boss.getBoundingBox().y - player.getBoundingBox().height + 4.0f});
        player.setVelocity({0.0f, Boss::STOMP_MIN_DESCENT_SPEED * 3.0f});
        resolver.resolvePlayerVsEnemy(player, boss, fromAbove);
    };

    {   // Guard up: the stomp is refused and costs the player instead.
        Mario player({0.0f, 0.0f});
        player.setStartingForm(Player::Form::Fire);
        Bowser bowser({400.0f, 600.0f});
        const int healthBefore = bowser.getHealth();

        stompFrom(player, bowser);

        check(bowser.getHealth() == healthBefore,
              "jumping on an unstaggered Bowser costs him no health");
        check(player.getForm() != Player::Form::Fire || player.getInvincibilityTimer() > 0.0f,
              "and costs the player a hit instead");
        check(player.getVelocity().y < 0.0f && std::abs(player.getVelocity().x) > 0.0f,
              "knocked away from him rather than bounced off his head");
    }

    {   // Four fireballs, then the same stomp: now it lands.
        Mario player({0.0f, 0.0f});
        player.setStartingForm(Player::Form::Fire);
        Bowser bowser({400.0f, 600.0f});
        for (int i = 0; i < Bowser::FIRE_HITS_PER_STAGGER; ++i) bowser.onHitByFireball();
        check(bowser.isStaggered(), "four fireballs open him");

        const int healthBefore = bowser.getHealth();
        stompFrom(player, bowser);

        check(bowser.getHealth() == healthBefore - 1,
              "and a contact taken through that opening costs him a point");
        check(player.getVelocity().y < 0.0f, "the player bounces off him");
        check(player.getForm() == Player::Form::Fire,
              "and is not hurt doing it — a stagger is a dropped guard");
    }

    {   // The regression guard for the other side of the same change: a
        // descending stomp is Boom Boom's intended and only mechanic, and
        // verify_regressions has arena and stomp cases that must stay green.
        Mario player({0.0f, 0.0f});
        BoomBoom boss({400.0f, 600.0f});
        const int healthBefore = boss.getHealth();

        stompFrom(player, boss);

        check(boss.getHealth() == healthBefore - 1,
              "Boom Boom is still stomped without needing a stagger first");
        check(player.getVelocity().y < 0.0f, "and still bounces the player");
    }

    Game::getInstance().setTileMap(nullptr);
}

// ---------------------------------------------------------------------------
// Defect 3 — the bridge chop drains him in the lava instead of deleting him.
// ---------------------------------------------------------------------------
void testBridgeChopDrainsBowserInTheLava() {
    section("D3  a Bowser who loses the bridge falls, burns and drains to zero");

    ArenaScene scene(189.0f);   // on the right ledge, where the axe is
    if (!scene.ok) { check(false, "level_3's arena and its Bowser load"); return; }

    const sf::Vector2f standingAt = scene.bowser->getPosition();
    const int fullHealth = scene.bowser->getHealth();

    // PlayingState::chopBridge's own rule, applied here so this case does not
    // depend on the state class it cannot reach: inside the arena, every solid
    // tile in a column that has lava under it is dropped.
    const AABB arena = scene.bowser->getArena();
    const int firstX = static_cast<int>(arena.x / Constants::TILE_SIZE);
    const int lastX = std::min(scene.map.getWidth() - 1,
                               static_cast<int>((arena.x + arena.width) / Constants::TILE_SIZE));
    int dropped = 0;
    for (int x = firstX; x <= lastX; ++x) {
        int topLavaRow = -1;
        for (int y = 0; y < scene.map.getHeight(); ++y) {
            if (scene.map.getTileType(x, y) == TileType::Lava) { topLavaRow = y; break; }
        }
        if (topLavaRow < 0) continue;
        for (int y = 0; y < topLavaRow; ++y) {
            if (!TileMap::getInfo(scene.map.getTileType(x, y)).isSolid) continue;
            scene.map.setTile(x, y, TileType::Empty);
            ++dropped;
        }
    }
    check(dropped > 0, "the chop drops the bridge tiles Bowser was standing on");

    // The one call PlayingState::chopBridge has to make in place of defeatNow().
    scene.bowser->beginLavaDeath();
    check(scene.bowser->isDyingInLava(), "the chop puts him into his lava death");
    check(!scene.bowser->isDefeated(), "he is not dead yet — that is the whole point");

    // He must not be recovered by the guard that catches a boss who wandered
    // off. That guard fires on exactly this shape of event (alive, and nothing
    // underneath him), and it puts him back at FULL health in his arena.
    check(!scene.bowser->onLeftLevel(),
          "the out-of-world sweep does not despawn him mid-death");
    check(scene.bowser->getHealth() == fullHealth &&
              scene.bowser->getPosition().y >= standingAt.y,
          "and does not teleport him back to his arena spawn at full health");

    bool fell = false;
    bool drainedGradually = false;
    int lastHealth = fullHealth;
    int framesToZero = -1;
    for (int f = 0; f < 600; ++f) {
        scene.step(1);
        if (scene.bowser->getPosition().y > standingAt.y + Constants::TILE_SIZE) fell = true;
        const int now = scene.bowser->getHealth();
        // "Gradually" means the bar was observed at an intermediate value, not
        // that it went from full to nothing in one frame the way defeatNow() did.
        if (now < fullHealth && now > 0) drainedGradually = true;
        if (now > lastHealth) check(false, "his health never goes back up mid-death");
        lastHealth = now;
        if (now == 0 && framesToZero < 0) framesToZero = f;
        if (framesToZero >= 0) break;
    }

    check(fell, "he falls off the stump of the bridge under his own gravity");
    check(drainedGradually,
          "and the bar is seen partway down — the player watches him go");
    check(framesToZero > 0, "the drain reaches zero");
    if (framesToZero > 0) {
        const float seconds = static_cast<float>(framesToZero) * DT;
        check(seconds > 0.5f && seconds < 6.0f,
              "in a watchable stretch rather than instantly or forever (" +
                  std::to_string(seconds) + "s)");
    }

    // And the ordinary defeat sequence takes it from there, so the score, the
    // BossDefeated event and the animation are the ones any other kill produces.
    scene.step(180);
    check(!scene.bowser->isActive(), "then the defeat sequence finishes and he is removed");
}

}  // namespace

int main() {
    std::cout << "=== R21 Bowser combat suite ===\n";

    testFireballRegistersOnBowserAndStaggersHim();
    testBowserCannotBeStompedWhileGuarded();
    testBridgeChopDrainsBowserInTheLava();

    std::cout << "\n=== " << (g_checks - g_failures) << "/" << g_checks
              << " checks passed ===\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASSED\n";
    return 0;
}
