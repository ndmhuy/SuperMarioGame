# Member Contributions — Group 52 (Super Mario Game)

**Course:** CS202 - C++ Programming / Object-Oriented Design  
**Class:** 25A01  
**Group:** 52  

This document is the same data as `Member_Contributions.xlsx` (the TA-facing spreadsheet, in the format used by the group's other CS202 project) rendered as Markdown/PDF for readability — both are generated from one source so they cannot drift apart.

## Summary

| | |
|---|---|
| Number of students | 2 |
| Number of tasks | 118 |
| Number of task hours | 214.5 |
| Number of Git commits | 316 |
| Max student percentage | 0.5 |
| Project score | 10 |

> Do not edit the grey cells in the spreadsheet edition. TAs enter the score of all PAs in the project score and get the individual scores; students may enter an estimated project score to see how percentages affect it. `Percent` is set to 0.5/0.5 for both members — the raw Tasks/Hours/Git percentages below inform that judgment rather than dictate it, the same convention the reference spreadsheet uses.

| No | Student ID | Full name | Tasks | Tasks % | Task Hours | Hours % | Git Commits | Git % | Percent | Score (Student) |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 25125083 | Nguyễn Đình Minh Huy | 73 | 61.9% | 134.5 | 62.7% | 258 | 81.6% | 0.5 | 10 |
| 2 | 25125084 | Trần Gia Huy | 45 | 38.1% | 80.0 | 37.3% | 58 | 18.4% | 0.5 | 10 |

Note on the gap between the Git % column and the Tasks/Hours % columns: Member A's git identity carries a large share of small fix/doc/log commits (258 of 316) accumulated across the whole session history, while task count and estimated hours — a better proxy for actual design/implementation effort — split closer to 62/38. Both are shown rather than only the more flattering one.

---

## Task-by-task breakdown

*A task should be done by only 1-2 students. Each member worked far more than the minimum 5 tasks the reference template asks for.*

| No | Student ID | Full name | Task Description | Hours | Evidence |
|---|---|---|---|---|---|
| 1 | 25125083 | Nguyễn Đình Minh Huy | Configured CMake (FetchContent) for SFML 3.0.2 + Dear ImGui/ImGui-SFML and set up the project directory structure | 2 | see docs/Group52_04/52.md |
| 2 | 25125083 | Nguyễn Đình Minh Huy | Drafted the v2.0 project spec (110 features), UML class diagrams and the implementation plan (TASKS.md, TASK_DIVISION.md) | 3 | see docs/Group52_04/52.md |
| 3 | 25125083 | Nguyễn Đình Minh Huy | Wrote clean C++ base skeletons for managers, physics and entities | 1.5 | see docs/Group52_04/52.md |
| 4 | 25125083 | Nguyễn Đình Minh Huy | Implemented MathUtils (vector ops, clamp, interpolation helpers) | 1 | see docs/Group52_04/52.md |
| 5 | 25125083 | Nguyễn Đình Minh Huy | Built GameStateManager/IGameState state-stack pattern and the Game singleton main loop at a fixed 60Hz timestep | 3 | see docs/Group52_04/52.md |
| 6 | 25125083 | Nguyễn Đình Minh Huy | Implemented AABB collision primitives and a dynamic SpatialHash broadphase grid | 2.5 | see docs/Group52_04/52.md |
| 7 | 25125083 | Nguyễn Đình Minh Huy | Built the PhysicsEngine update pipeline (gravity, gravity zones/water viscosity, axis-separated tile resolution) | 2.5 | see docs/Group52_04/52.md |
| 8 | 25125083 | Nguyễn Đình Minh Huy | Implemented CollisionDetector/CollisionResolver (solid tiles, conveyors, stomps/damage, player-vs-player bounces) | 3 | see docs/Group52_04/52.md |
| 9 | 25125083 | Nguyễn Đình Minh Huy | Restructured Entity attributes to protected with friend-class access and read-only getters | 1 | see docs/Group52_04/52.md |
| 10 | 25125083 | Nguyễn Đình Minh Huy | Implemented the IPlayerState decorator pattern (StarDecorator, MegaDecorator) with timers | 2 | see docs/Group52_04/52.md |
| 11 | 25125083 | Nguyễn Đình Minh Huy | Coded concrete playable characters (Mario, Luigi, Toad, Peach) with custom physics/abilities | 2.5 | see docs/Group52_04/52.md |
| 12 | 25125083 | Nguyễn Đình Minh Huy | Built the Item base class and 12 concrete item subclasses (Mushroom, FireFlower, Coin, Star, etc.) | 3 | see docs/Group52_04/52.md |
| 13 | 25125084 | Trần Gia Huy | Built InputManager (Command Pattern) with dual-player key mapping (P1 WASD/Space/F, P2 Arrows/M) | 2 | see docs/Group52_04/52.md |
| 14 | 25125084 | Trần Gia Huy | Implemented SoundManager (Singleton) with music/SFX channels, WAV/OGG loading | 1.5 | see docs/Group52_04/52.md |
| 15 | 25125084 | Trần Gia Huy | Researched/designed the IMovementStrategy interface and 8 enemy AI strategies | 2 | see docs/Group52_04/52.md |
| 16 | 25125084 | Trần Gia Huy | Prepared abstract skeletons for the Enemy and Block type hierarchies | 1 | see docs/Group52_04/52.md |
| 17 | 25125084 | Trần Gia Huy | Designed the EntityFactory registry blueprint for JSON-driven instantiation | 1 | see docs/Group52_04/52.md |
| 18 | 25125083 | Nguyễn Đình Minh Huy | Implemented the full Camera class (LERP tracking, screen shake, viewport clamping) | 2 | see docs/Group52_05/52.md |
| 19 | 25125083 | Nguyễn Đình Minh Huy | Refactored move/run commands to intent flags on Character/Player instead of direct velocity changes | 1.5 | see docs/Group52_05/52.md |
| 20 | 25125083 | Nguyễn Đình Minh Huy | Centralized friction/acceleration/run-speed logic into PhysicsEngine::update() with surface-specific properties | 2 | see docs/Group52_05/52.md |
| 21 | 25125083 | Nguyễn Đình Minh Huy | Integrated Camera into PlayingState with dual-view (world vs HUD) rendering | 1.5 | see docs/Group52_05/52.md |
| 22 | 25125083 | Nguyễn Đình Minh Huy | Added mid-air wall-slide friction with a capped slide speed (WALL_SLIDE_SPEED) | 1.5 | see docs/Group52_05/52.md |
| 23 | 25125083 | Nguyễn Đình Minh Huy | Integrated nlohmann/json and built LevelLoader to parse tile/entity/spawn data from level JSON | 2.5 | see docs/Group52_05/52.md |
| 24 | 25125083 | Nguyễn Đình Minh Huy | Authored 4 level files (level_1..3, bonus_1) and an ImGui level selector for playtesting | 2 | see docs/Group52_05/52.md |
| 25 | 25125084 | Trần Gia Huy | Implemented IMovementStrategy and 8 concrete AI strategies (Patrol, Chase, Fly, etc.) | 2.5 | see docs/Group52_05/52.md |
| 26 | 25125084 | Trần Gia Huy | Coded the Enemy base class plus Goomba, KoopaTroopa, KoopaParatroopa, Boo | 3 | see docs/Group52_05/52.md |
| 27 | 25125084 | Trần Gia Huy | Coded the Block base class plus BrickBlock, QuestionBlock, Pipe, Flagpole | 2.5 | see docs/Group52_05/52.md |
| 28 | 25125084 | Trần Gia Huy | Coded 7 advanced v2.0 enemies (PiranhaPlant, BulletBill, HammerBro, Thwomp, ChainChomp, Lakitu, Spiny) | 3.5 | see docs/Group52_05/52.md |
| 29 | 25125084 | Trần Gia Huy | Coded 5 advanced v2.0 blocks (HiddenBlock, MovingPlatform, FallingPlatform, IceBlock, ConveyorBelt) | 2.5 | see docs/Group52_05/52.md |
| 30 | 25125084 | Trần Gia Huy | Implemented EntityFactory to instantiate 25+ entity types from JSON | 2.5 | see docs/Group52_05/52.md |
| 31 | 25125084 | Trần Gia Huy | Built SpriteSheet, Animation (flyweight) and AnimationManager/Animator | 3 | see docs/Group52_05/52.md |
| 32 | 25125084 | Trần Gia Huy | Wrote verify_enemies_new.cpp / verify_blocks_new.cpp test harnesses | 1.5 | see docs/Group52_05/52.md |
| 33 | 25125083 | Nguyễn Đình Minh Huy | Built Serializer for save-slot JSON schemas (character, player state, scores, checkpoints, playtime) | 2.5 | see docs/Group52_06/52.md |
| 34 | 25125083 | Nguyễn Đình Minh Huy | Implemented StatisticsTracker (EventBus subscriber for coins/enemies/deaths/combos/playtime) | 1.5 | see docs/Group52_06/52.md |
| 35 | 25125083 | Nguyễn Đình Minh Huy | Implemented AchievementManager (10 achievements) plus a slide-in/out toast HUD | 2 | see docs/Group52_06/52.md |
| 36 | 25125083 | Nguyễn Đình Minh Huy | Built the ImGui Pause/Settings panel (save slots, volume, keybindings, difficulty, auto-save) | 2 | see docs/Group52_06/52.md |
| 37 | 25125083 | Nguyễn Đình Minh Huy | Wrote the verify_save_load.cpp integration test suite | 1 | see docs/Group52_06/52.md |
| 38 | 25125083 | Nguyễn Đình Minh Huy | Implemented the Undo/Redo command framework (EditorCommands) | 2 | see docs/Group52_06/52.md |
| 39 | 25125083 | Nguyễn Đình Minh Huy | Built the MapEditor viewport dashboard (tile/entity brushes, undo/redo, save) | 2 | see docs/Group52_06/52.md |
| 40 | 25125083 | Nguyễn Đình Minh Huy | Implemented pixel-to-grid coordinate mapping and grid-line rendering | 1.5 | see docs/Group52_06/52.md |
| 41 | 25125083 | Nguyễn Đình Minh Huy | Centralized duplicate tile/entity name mappers into SerializationUtils | 1 | see docs/Group52_06/52.md |
| 42 | 25125083 | Nguyễn Đình Minh Huy | Wrote the verify_map_editor.cpp test harness | 1 | see docs/Group52_06/52.md |
| 43 | 25125084 | Trần Gia Huy | Participated in architectural reviews, resolved cross-branch merge conflicts, refined achievement SFX triggers | 1 | see docs/Group52_06/52.md |
| 44 | 25125083 | Nguyễn Đình Minh Huy | Merged A/feature/save-load and A/feature/level-editor; addressed PR review fixes (EventBus sentinel, debug key remap, Game keybinding encapsulation) | 2 | see docs/Group52_07/52.md |
| 45 | 25125083 | Nguyễn Đình Minh Huy | Fixed a stale achievement-toast bug on slot reload; aligned achievement keys with SPEC.md | 1 | see docs/Group52_07/52.md |
| 46 | 25125083 | Nguyễn Đình Minh Huy | Built the consolidated verify_all.cpp test runner combining 6 test suites; updated CMakeLists.txt | 2 | see docs/Group52_07/52.md |
| 47 | 25125083 | Nguyễn Đình Minh Huy | Authored two_player_ai_plan.md (2-player + Shadow Mario architecture design doc) | 2.5 | see docs/Group52_07/52.md |
| 48 | 25125083 | Nguyễn Đình Minh Huy | Ran an interactive binary demo to validate save/load and level-editor integration | 1 | see docs/Group52_07/52.md |
| 49 | 25125084 | Trần Gia Huy | Entity Factory & concrete entities merged into dev (integration of prior work) | 0.5 | see docs/Group52_07/52.md |
| 50 | 25125084 | Trần Gia Huy | Enemy hierarchy + AI strategies merged into dev (integration of prior work) | 0.5 | see docs/Group52_07/52.md |
| 51 | 25125084 | Trần Gia Huy | Blocks/platforms merged into dev (integration of prior work) | 0.5 | see docs/Group52_07/52.md |
| 52 | 25125084 | Trần Gia Huy | Built Hud (lives/score/coins/time/stage) plus a boss health bar and retro typography (PressStart2P/SuperMario256 fonts) | 2 | see docs/Group52_07/52.md |
| 53 | 25125084 | Trần Gia Huy | Finalized SpriteSheet/AnimationManager/Animator; added verify_blocks/verify_enemies/verify_graphics/verify_graphics_visual/verify_hud_boss test targets | 1.5 | see docs/Group52_07/52.md |
| 54 | 25125084 | Trần Gia Huy | Consolidated duplicate asset/ and assets/ folders into one assets/ root | 0.5 | see docs/Group52_07/52.md |
| 55 | 25125084 | Trần Gia Huy | Built the Minimap component (scaled tilemap preview, camera frustum, player tracking dot) | 2 | Git commit cb6eab2 |
| 56 | 25125084 | Trần Gia Huy | Wrote the verify_minimap_visual.cpp test target | 1 | see docs/Group52_07/52.md |
| 57 | 25125083 | Nguyễn Đình Minh Huy | Reviewed/validated the ParticleSystem/ObjectPool<Particle> design (O(1) acquire/release, zero-allocation) | 1 | see docs/Group52_08/52.md |
| 58 | 25125083 | Nguyễn Đình Minh Huy | Updated TASKS.md through Phase 8 and Bonus A, cross-referenced against TASK_DIVISION.md | 0.5 | see docs/Group52_08/52.md |
| 59 | 25125083 | Nguyễn Đình Minh Huy | Ran git fetch --all, verified dev in sync with origin/dev, tracked branch status | 0.5 | see docs/Group52_08/52.md |
| 60 | 25125084 | Trần Gia Huy | Implemented ParticleSystem with ObjectPool<Particle> (burst/continuous emission modes) | 2.5 | Git commit 7a6f62b |
| 61 | 25125084 | Trần Gia Huy | Tuned particle parameters (alpha fade, emission rate, spread angle) | 1.5 | Git commits c7d4fd4, 62c78ef |
| 62 | 25125084 | Trần Gia Huy | Built the verify_particles_visual interactive test target | 1 | see docs/Group52_08/52.md |
| 63 | 25125084 | Trần Gia Huy | Extracted the player sprite sheet player.json (92 animation frames: idle/walk/run/jump/etc.) | 3 | Git commits d6bd07d, 0be4508, 9403be5 |
| 64 | 25125084 | Trần Gia Huy | Built the verify_player_animation_visual test target | 1 | see docs/Group52_08/52.md |
| 65 | 25125084 | Trần Gia Huy | Began sprite naming standardization / asset pipeline scripts | 1 | Git commit 15327b4 |
| 66 | 25125084 | Trần Gia Huy | Enhanced random-number generation with std::uniform_real_distribution for particle spread | 0.5 | Git commit 9bb9d16 |
| 67 | 25125083 | Nguyễn Đình Minh Huy | Implemented the Fireball entity + ObjectPool<Fireball>, EntityFactory/EventBus wiring, verify_fireball.cpp (4/4 pass) | 3 | see docs/Group52_09/52.md |
| 68 | 25125083 | Nguyễn Đình Minh Huy | Enhanced Flagpole with height-scaled scoring, animation and a LevelComplete event; verify_flagpole_trampoline.cpp | 2 | see docs/Group52_09/52.md |
| 69 | 25125083 | Nguyễn Đình Minh Huy | Enhanced Trampoline bounce mechanics (spring compression, configurable impulse) | 1.5 | see docs/Group52_09/52.md |
| 70 | 25125083 | Nguyễn Đình Minh Huy | Built the CheckpointFlag entity (visual states, auto-save, CheckpointActivated event, respawn coordinates) | 2 | see docs/Group52_09/52.md |
| 71 | 25125083 | Nguyễn Đình Minh Huy | Implemented the Memento pattern: GameSnapshot + TimeRewindManager (300-frame circular buffer); verify_memento_rewind.cpp (4/4 pass) | 3.5 | see docs/Group52_09/52.md |
| 72 | 25125083 | Nguyễn Đình Minh Huy | Built MapGenerator for procedural level generation; verify_map_generator.cpp | 3 | see docs/Group52_09/52.md |
| 73 | 25125083 | Nguyễn Đình Minh Huy | Implemented the dual bounding-box architecture (physicsBox/combatBox) across Entity/CollisionDetector/CollisionResolver | 2.5 | see docs/Group52_09/52.md |
| 74 | 25125083 | Nguyễn Đình Minh Huy | Added void-fall death detection in PlayingState | 1 | see docs/Group52_09/52.md |
| 75 | 25125083 | Nguyễn Đình Minh Huy | Added brick/question-block head-butt reactions in PhysicsEngine | 1.5 | see docs/Group52_09/52.md |
| 76 | 25125083 | Nguyễn Đình Minh Huy | Removed public setLives/setCoins/setScore from Player; added friend class PlayingState | 0.5 | see docs/Group52_09/52.md |
| 77 | 25125083 | Nguyễn Đình Minh Huy | Added Camera::move() for editor WASD/arrow panning | 1 | see docs/Group52_09/52.md |
| 78 | 25125083 | Nguyễn Đình Minh Huy | Added a Level Editor Mode (F1) button to MenuState | 0.5 | see docs/Group52_09/52.md |
| 79 | 25125083 | Nguyễn Đình Minh Huy | Hardened Game::shutdown() to pop all game states before manager teardown | 1 | see docs/Group52_09/52.md |
| 80 | 25125083 | Nguyễn Đình Minh Huy | Added multi-path font-loading fallback candidates in PlayingState | 0.5 | see docs/Group52_09/52.md |
| 81 | 25125083 | Nguyễn Đình Minh Huy | Fetched/merged origin/B/feature/graphic-visual into dev; resolved font-loading + agent_history.log conflicts; pushed dev | 1 | Git commit 17cef23 |
| 82 | 25125083 | Nguyễn Đình Minh Huy | Fixed non-solid TileType::Coin tiles not being collected on walk-through | 1 | Git commit d12e763 |
| 83 | 25125083 | Nguyễn Đình Minh Huy | Deduplicated a ResourceManager missing-SoundBuffer warning spam | 0.5 | see docs/Group52_09/52.md |
| 84 | 25125084 | Trần Gia Huy | Built a spritesheet-merging tool; updated enemy_projectile/item/world_scenery_item atlases | 3 | Git commits 044a5d8, 80c048e, b90a899 |
| 85 | 25125084 | Trần Gia Huy | Cleaned sprite/atlas coordinates affected by the merge | 1 | Git commit 10828a7 |
| 86 | 25125084 | Trần Gia Huy | Updated SPRITE_NAMING_CONVENTIONS.md (frame inventories, naming standards) | 1 | see docs/Group52_09/52.md |
| 87 | 25125084 | Trần Gia Huy | Built SpriteColorFilter (HSL->RGB, 12Hz Star Power cycling, 15Hz hurt flicker, hit-flash tint) | 2 | Git commit 2c3bb83 |
| 88 | 25125084 | Trần Gia Huy | Built SpriteTransformAnim (scale-lerp easing, 720 deg/s spin, 12Hz Mega/Mini flicker) | 2 | Git commit 2c3bb83 |
| 89 | 25125084 | Trần Gia Huy | Built the EntityDeathEffect singleton (EnemyFlip, StarKillSpin, PlayerDeathHop) | 2 | Git commit 2c3bb83 |
| 90 | 25125084 | Trần Gia Huy | Added a SoundManager synthesized fallback beep buffer | 1 | see docs/Group52_09/52.md |
| 91 | 25125084 | Trần Gia Huy | Upgraded verify_graphics_visual with a Transform/Color-Filter FX ImGui tab | 1.5 | see docs/Group52_09/52.md |
| 92 | 25125083 | Nguyễn Đình Minh Huy | Refined MapGenerator (elevation profiles, biome ceilings, lava pits, jump-reachability-guarded platforms, 3 Star Coins, threat-pacing curve) + ImGui tuning panel | 3.5 | see docs/Group52_10/52.md |
| 93 | 25125083 | Nguyễn Đình Minh Huy | Built the 3-world campaign + warp-pipe system; generate_game_levels.cpp synthesizing 7 level JSONs; extended SerializationUtils/LevelLoader for warp metadata; seamless sub-level transitions | 4 | see docs/Group52_10/52.md |
| 94 | 25125083 | Nguyễn Đình Minh Huy | Added ImGui campaign navigation dropdowns and an Active Level Tube warp selector | 1.5 | see docs/Group52_10/52.md |
| 95 | 25125083 | Nguyễn Đình Minh Huy | Fixed a static-block gravity bug (getGravityMultiplier() override); added item resting-state physics (resolveItemVsBlock) | 1.5 | see docs/Group52_10/52.md |
| 96 | 25125083 | Nguyễn Đình Minh Huy | Optimized player hitbox dimensions (32px to 24px width tiers); refined stomp-vs-side-hit combat resolution | 2 | see docs/Group52_10/52.md |
| 97 | 25125083 | Nguyễn Đình Minh Huy | Fixed the LevelLoader ifstream failbit cascade (file.clear()); expanded fallback search paths | 1.5 | see docs/Group52_10/52.md |
| 98 | 25125083 | Nguyễn Đình Minh Huy | Fixed a SIGABRT shutdown crash (explicit m_window.close()) and hardened GameStateManager transitions | 1.5 | see docs/Group52_10/52.md |
| 99 | 25125084 | Trần Gia Huy | Wired sprite animations across all entities; aspect-ratio scaling for HUD/items; POW 8-frame shockwave; trampoline/PSwitch visual states | 3.5 | Git commits 069817b, 0bc21c3, 2d5058e, 83660b5, 065557d, 61e30bc |
| 100 | 25125084 | Trần Gia Huy | Built the Web Metadata Editor Studio (metadata_editor.html, edit_sprite_metadata.py) with cursor-anchored zoom, position-finder HUD, drag-to-draw frames, TexturePacker-compatible export | 2.5 | see docs/Group52_10/52.md |
| 101 | 25125084 | Trần Gia Huy | Integrated sprite HUD assets (animated coin counter, Star Coin indicators, character-specific lives icon) | 1.5 | see docs/Group52_10/52.md |
| 102 | 25125084 | Trần Gia Huy | Built PipeRenderer (2-block-wide AABB, directional multi-part pipe head/body rendering) | 1.5 | Git commit a0f1a3b |
| 103 | 25125084 | Trần Gia Huy | Wired game-wide SFX/BGM into SoundManager (jump, stomp, coin, break, power-up/down, boing, stage-clear, game-over) + fallback buffer | 2.5 | Git commits 1002595, c383c19, 9976209 |
| 104 | 25125084 | Trần Gia Huy | Tuned all 11 enemy AI state machines (Thwomp slam cycle, Boo gaze-detection, HammerBro parabolic throw, Koopa shell physics) | 3 | Git commit d954a66 |
| 105 | 25125084 | Trần Gia Huy | Built verify_enemies_behavior.cpp + enhanced verify_enemies_visual.cpp (11 isolated test rooms) | 1.5 | Git commit d954a66 |
| 106 | 25125083 | Nguyễn Đình Minh Huy | Resolved the Level 1-3 crash on Bowser's first fireball (mid-frame entity-spawn vector reallocation); rebuilt and playtested on Windows | 3 | see logs/agent_history.log 2026-08-16/22 |
| 107 | 25125083 | Nguyễn Đình Minh Huy | Fixed 12 defects from a playtest pass plus two menus that overflowed | 2 | Git commit f6fc8bc |
| 108 | 25125083 | Nguyễn Đình Minh Huy | Fixed CMakeLists naming src/Graphics/HUD.cpp when the file is Hud.cpp | 0.5 | Git commit 03bf828 |
| 109 | 25125083 | Nguyễn Đình Minh Huy | Fixed the player being unable to jump while walking into a wall | 1 | Git commit 60cc735 |
| 110 | 25125083 | Nguyễn Đình Minh Huy | Fixed 28 files using std:: symbols without including their header | 1 | Git commit 4ac8a1c |
| 111 | 25125083 | Nguyễn Đình Minh Huy | Fixed CI building only 2 of the 13 binaries it then tested | 1 | Git commit 8141120 |
| 112 | 25125083 | Nguyễn Đình Minh Huy | Fixed 4 test harnesses opening an sf::RenderWindow on a CI runner with no display | 1 | Git commit 33d5aae |
| 113 | 25125083 | Nguyễn Đình Minh Huy | Fixed three defects the screenshots showed that the tests did not (backdrop ground line, hill-bottom offset, a Player 2 HUD badge overwriting the world field) | 2 | Git commit da8ca99 |
| 114 | 25125083 | Nguyễn Đình Minh Huy | Sealed test hermeticity: Serializer::setSaveDirectory() + TestSaveSandbox.hpp so all 23 verify_* harnesses point at a fresh temp directory | 2.5 | Git commit 045f9b7 |
| 115 | 25125083 | Nguyễn Đình Minh Huy | Generated real UML class diagrams from the headers (tools/gen_class_diagram.py) and wrote the CS202 final report's first edition (15 sections, generated by build_report.py) | 4 | Git commits a0395d6, 3c0a768, 64088b4 |
| 116 | 25125083 | Nguyễn Đình Minh Huy | Produced a LaTeX/PDF edition of the report in the HCMUS template and deepened it (implementation section 3->11 subsections, technical-problems section rebuilt into 8 themed groups) | 4 | Git commits fdec9cc, 730336c |
| 117 | 25125083 | Nguyễn Đình Minh Huy | Diagnosed and fixed three defects reported from a real playthrough: the end-of-level castle's buffer margin, Boom Boom's arena overlapping the 1-2 flagpole, and the camera/z-order bug carried into 1-3; added regression tests for all three | 4 | this session, branch A/fix/level-completion-and-camera-defects |
| 118 | 25125084 | Trần Gia Huy | Resolved merge conflicts while pulling the latest changes and merged into main | 2 | Git commits 72b13c6, a522007, 75e9cbb |
