# Member Contributions — Group 52 (Super Mario Game)

**Course:** CS202 - C++ Programming / Object-Oriented Design  
**Class:** 25A01  
**Group:** 52  

This document is the same data as `Member_Contributions.xlsx` (the TA-facing spreadsheet, in the format used by the group's other CS202 project) rendered as Markdown/PDF for readability — both are generated from `scripts/build_contributions.py` (one source, one task table) so they cannot drift apart.

## Summary

| | |
|---|---|
| Number of students | 2 |
| Number of tasks | 157 |
| Number of task hours | 321 |
| Number of Git commits | 478 |
| Max student percentage | 0.5 |
| Project score | 10 |

> Do not edit the grey cells in the spreadsheet edition. TAs enter the score of all PAs in the project score and get the individual scores; students may enter an estimated project score to see how percentages affect it. `Percent` is set to 0.5/0.5 for both members — the raw Tasks/Hours/Git percentages below inform that judgment rather than dictate it, the same convention the reference spreadsheet uses.

| No | Student ID | Full name | Tasks | Tasks % | Task Hours | Hours % | Git Commits | Git % | Percent | Score (Student) |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 25125083 | Nguyễn Đình Minh Huy | 112 | 71.3% | 241 | 75.1% | 417 | 87.2% | 0.5 | 10 |
| 2 | 25125084 | Trần Gia Huy | 45 | 28.7% | 80 | 24.9% | 61 | 12.8% | 0.5 | 10 |

Note on the gap between the Git % column and the Tasks/Hours % columns: Nguyễn Đình Minh Huy's git identity carries a large share of small fix/doc/log commits (417 of 478) accumulated across the whole session history, while task count and estimated hours — a better proxy for actual design/implementation effort — split closer to 71/29. Both are shown rather than only the more flattering one.

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
| 118 | 25125083 | Nguyễn Đình Minh Huy | Implemented Endless Mode (chunked infinite level generation with rising difficulty, distance-based scoring) and a real solvability oracle for MapGenerator, after evaluating (and rejecting the framework of) the abandoned A/mapgen-gan-plan and A/rl-neural-policy side branches for anything worth porting | 5 | this session, branch A/feature/endless-mode-and-solvability |
| 119 | 25125083 | Nguyễn Đình Minh Huy | Rebuilt the submission package: a 97-feature list and per-member manual test checklist grounded in a fresh source-code audit, the Member Contributions spreadsheet (matching the DesignPatternsGroup52 format), and an expanded report (OOP/pattern rationale, two new UML diagrams) | 4 | this session, branch A/docs/submission-package-update |
| 120 | 25125084 | Trần Gia Huy | Resolved merge conflicts while pulling the latest changes and merged into main | 2 | Git commits 72b13c6, a522007, 75e9cbb |
| 121 | 25125083 | Nguyễn Đình Minh Huy | Merged all outstanding work to dev/main and generated the initial submission package (AI Usage Declaration, features list, task division, demo-video placeholder, member contributions, final report) while kicking off the defect-auditing test suite. | 3 | see logs/agent_history.log 2026-08-30, branch main |
| 122 | 25125083 | Nguyễn Đình Minh Huy | Merged the level-completion, endless-mode, and submission-package branches into dev then main and pushed; corrected the AI Usage Declaration's tool attribution (Antigravity vs. Claude Code) per user feedback. | 1 | see logs/agent_history.log 2026-08-31 09:45 and 10:00, branch main |
| 123 | 25125083 | Nguyễn Đình Minh Huy | Replaced the report's ASCII architecture diagram with a real figure, embedded full-depth UML diagrams in the appendix, and rewrote the class-diagram generator to render every attribute/method with UML visibility notation, fixing a brace-init parsing bug along the way. | 3.5 | branches A/docs/report-architecture-figure-and-full-uml, A/docs/detailed-uml-full-attributes-methods |
| 124 | 25125083 | Nguyễn Đình Minh Huy | Ran a three-way SPEC/code/report audit via three parallel subagent passes, producing a 15-defect, 14-task (R1-R14) remediation plan and correcting nine stale claims in the features list and report. | 4 | branch A/docs/spec-feature-audit |
| 125 | 25125083 | Nguyễn Đình Minh Huy | Small-defect and hygiene batch (R1/R2/R3/R6): fixed SoundManager's level-BGM index mapping, Enemy/Spiny's screen-height despawn bug, and unsurfaced solvability results; deleted the dead AnimationManager class; degraded audio startup gracefully with no device; untracked .member_profile.json and added SPEC's descope addendum. | 3.5 | see logs/agent_history.log 2026-08-31 11:48-12:34 |
| 126 | 25125083 | Nguyễn Đình Minh Huy | Migrated every raw EventBus SubscriptionId to ScopedSubscription across PlayingState/AchievementManager/StatisticsTracker/Camera (R4), and fixed a real static-destruction-order SIGSEGV the migration surfaced. | 2.5 | branch A/fix/scoped-subscription-adoption, commit 00c08f8 |
| 127 | 25125083 | Nguyễn Đình Minh Huy | Fixed a P-Switch soft-lock caused by mis-themed sub-level floors, a half-rendered single-column pipe in all three sub-levels, and made the overworld QuestionBlock's sprite reflect its emptied state to match SPEC (R16). | 3 | branch A/fix/sublevel-tile-batch, commit cc6a32d |
| 128 | 25125083 | Nguyễn Đình Minh Huy | Investigated reported 'Bowser never spawns' and 'flag reachable mid-fight' defects (both refuted as already fixed/non-reproducing) and widened Boom Boom's arena from 3.5 to 12.5 tiles of pacing room by removing decorative staircase tiles (R15). | 2.5 | branch A/fix/boss-encounter-batch, commit 56c8db7 |
| 129 | 25125083 | Nguyễn Đình Minh Huy | Fixed the end-game audio batch: castle_complete fanfare being clobbered by level_complete on boss levels via a deferred one-shot-music swap; investigated and struck two further audio defects as not reproducible/already fixed (R19). | 2 | branch A/fix/endgame-audio-batch, commit de52042 |
| 130 | 25125083 | Nguyễn Đình Minh Huy | Reconciled the SPEC-audit document's defect ledger against shipped code, marking or striking about twenty defects and phases (R1-R19) and documenting four previously mis-recorded playtest defects. | 1 | branch A/docs/audit-status-reconciliation, commit cc08fc3 |
| 131 | 25125083 | Nguyễn Đình Minh Huy | Fixed the Star power's missing rainbow-tint visual, the sub-level Piranha Plant's pipe-centering offset, and the fireball-kill flip-death sprite anchor via a shared Entity::drawSprite fix reaching every flip-killable enemy (R17). | 2.5 | branch A/fix/player-enemy-visual-batch, commit 4550bfe |
| 132 | 25125083 | Nguyễn Đình Minh Huy | Eliminated all 28 real dynamic_casts from CollisionResolver/PhysicsEngine via new virtual Entity/Item/Block/Enemy/Character hooks (nine commits, one dispatch family each), adding 26 regression checks and discovering that Release's -DNDEBUG compiled away nearly all existing assert()-based coverage (R5). | 4 | branch A/refactor/collision-dispatch, commit 4b87e0d |
| 133 | 25125083 | Nguyễn Đình Minh Huy | Placed all six previously-unused enemy types (KoopaParatroopa, Boo, BulletBill, Thwomp, ChainChomp, Lakitu) and hidden blocks across the campaign, fixed the secret_finder achievement's wrong trigger condition, and fixed two playtest-found defects in Bullet Bill and Lakitu placement (R9). | 4 | branch A/feature/campaign-population-pass, commit 4d6be7e |
| 134 | 25125083 | Nguyễn Đình Minh Huy | Stripped -DNDEBUG from ctest targets only (fixing vacuously-passing assert-based harnesses), refactored the build to compile app sources once via two CMake OBJECT libraries, and added a mutation-tested guard against tests writing into the real saves/ directory (R11). | 4 | branches A/fix/tests-honour-assertions, A/build/object-library, A/fix/hermetic-tests |
| 135 | 25125083 | Nguyễn Đình Minh Huy | Wired eight dormant systems into real gameplay: star-power side-touch kills, four unused particle types, surface-dependent footsteps, combo-hit pitch escalation, menu-row navigation SFX, a single-player camera clamp, and debug-console tab-completion (R7). | 3 | branch A/feature/wire-dormant-vfx-sfx, commit 7d94fb9 |
| 136 | 25125083 | Nguyễn Đình Minh Huy | Added a keyboard-navigable LOAD GAME menu with a 3-slot picker showing character/level/score/time previews, reusing PlayingState's existing loadFromSlot path (R8). | 2.5 | branch A/feature/load-game-menu, commit d6672b7 |
| 137 | 25125083 | Nguyễn Đình Minh Huy | Implemented main-menu Attract Mode (30s idle triggers a bundled demo replay, dismissed by any key), fixing two latent ReplayRecorder playback-pacing/fade-in bugs surfaced by actually watching it run (R10). | 3 | branch A/feature/attract-mode, commit 6075c27 |
| 138 | 25125083 | Nguyễn Đình Minh Huy | Ran a full scripted playtest evidence pass across campaign completion, P-Switch/POW/axe, 2P versus death, difficulty modes, key rebinding, and FPS, discovering two new defects: boss destroyed by the respawn-safety sweep, and Load Game always resuming World 1-1 (R12). | 3 | branch A/verify/full-playtest-pass |
| 139 | 25125083 | Nguyễn Đình Minh Huy | Fixed the boss-survives-respawn defect (D29), Load Game restoring the saved level (D30), and re-implemented a double-exit guard for PlayingState, each with a mutation-tested regression case; merged and independently re-verified into dev (R20). | 3 | branch A/fix/remaining-defects-d29-d30-exit, commit 6e4868d |
| 140 | 25125083 | Nguyễn Đình Minh Huy | Root-caused and fixed a user-reported bug where Bowser fell out of the world through his own bridge/lava floor, since the void-plane check only covered players; added an Entity::onLeftLevel virtual hook. | 2 | branch A/fix/boss-falls-out-of-world, commit 95521a8 |
| 141 | 25125083 | Nguyễn Đình Minh Huy | Fixed ten gameplay/visual issues in one pass: castle sprite/position, level-clear music timing, Star invincibility flicker, Piranha Plant centering, fireball flip pivot, Bowser arena verification, bonus-level plateau/castle seating, and Peach's Space-key float. | 3 | see logs/agent_history.log 2026-09-01 15:31 and 16:52, branch dev |
| 142 | 25125083 | Nguyễn Đình Minh Huy | Fixed sub-level pipe teleports and half-sprite rendering, decoupled the S key from ground-pound (crouch-only), halved Thwomp slam speed with a narrowed detection column, rewrote Chain Chomp's tethered-chase AI, enclosed the 1-3 bridge with walls, and fixed the camera snap on Bowser's defeat. | 3 | see logs/agent_history.log 2026-09-01 17:55, branch dev |
| 143 | 25125083 | Nguyễn Đình Minh Huy | Wrote the demo-video-requirements checklist; fixed the M-key minimap toggle, a duplicate player-death animation, metal-footstep SFX volume, and further tuned Thwomp slam/recovery speed and its 1-3 placement. | 1.5 | see logs/agent_history.log 2026-09-01 23:30, branch dev |
| 144 | 25125083 | Nguyễn Đình Minh Huy | R21 Phase 1 defect batch: fixed twelve release defects in one pass — a duplicate untracked asset tree causing level-data drift, half/warped pipe rendering, spawns embedded in solid tiles, an incomplete castle slab, double-integrated moving platforms, menu text overflow, duplicate achievement toasts, and gated debug ImGui off by default. | 4 | branch A/release/defect-batch-r21, commit 171a5b3 |
| 145 | 25125083 | Nguyễn Đình Minh Huy | Merged the R21 batch and completed Phases 2-4: built a real GUI level editor wired to EntityFactory (fixing 24 of 40 silently-broken palette buttons), debug/recording cheats with a void-rescuing Immortal mode, fixed far-chunk procedural entities teleporting back to chunk-local coordinates, an explicit entity-type registry, and Bonus D's dynamic day/night lighting via a real GLSL shader. | 4.5 | commit 97fc43a (merge into dev), branch A/release/defect-batch-r21 |
| 146 | 25125083 | Nguyễn Đình Minh Huy | Second user-reported defect wave: fixed warp-pipe hitbox/art and entry-mode rendering, Bowser's fireball stagger triple-counting a single fireball as three hits, Bowser falling into lava, Lakitu dropping a Fire Flower to keep the fight winnable, the 2P camera freezing on the eliminated player instead of following the survivor, and added save-slot choice/delete. | 3.5 | commit e97b7da (merge), branch A/fix/versus-camera-and-axes |
| 147 | 25125083 | Nguyễn Đình Minh Huy | Audited OOP/SOLID adherence and design-pattern fidelity across the codebase (dynamic_cast counts, singleton count, friend-class audit, PDF text-overlap root cause) and authored the phased submission-sweep plan with per-lane dependencies, models, and effort levels. | 3 | docs/issues/submission_sweep_plan_2026-09-02.md, branch dev |
| 148 | 25125083 | Nguyễn Đình Minh Huy | Ran the submission-sweep Phase 0 baseline: clean rebuild, full ctest/regression run, a live smoke-test launch, and confirmed the LevelSolvability fix, with no code changes. | 1 | see logs/agent_history.log 2026-09-02 18:58, branch dev |
| 149 | 25125083 | Nguyễn Đình Minh Huy | Wrote the W11, W12, and W13 weekly reports from git log and agent-history evidence, including tracing the World 1-3 mid-frame entity-spawn crash's root cause for W11. | 3 | branches A/docs/weekly-w11, A/docs/weekly-w12-w13 |
| 150 | 25125083 | Nguyễn Đình Minh Huy | Archived 20+ stale root/docs artifacts to docs/archive/, untracked stale saves/PDFs/zips, then reverted an AGENTS.md edit that had landed inside AgentHub's generated block and routed the rule change upstream instead. | 2 | branch A/chore/archive-2026-09-02, commit 9b83c9a |
| 151 | 25125083 | Nguyễn Đình Minh Huy | Extended the AI Usage Declaration to cover the full R1-R21 remediation plan, then corrected its date range and removed an unverifiable test-count claim per review. | 1 | branch A/docs/ai-declaration, commit 96560bd |
| 152 | 25125083 | Nguyễn Đình Minh Huy | Captured fresh gameplay screenshots and composited three YouTube thumbnail candidates, then narrowed to one final crop-and-zoom version after review found Mario illegibly small at native resolution. | 1.5 | branch A/docs/video-thumbnail |
| 153 | 25125083 | Nguyễn Đình Minh Huy | Fixed the report PDF's overlapping-table-text defect with content-aware column sizing, added a build.sh Overfull-hbox guard, and split two illegible full-detail UML pages into landscape multi-page spreads. | 2.5 | branch A/docs/report-layout, commit 0f3e5ea |
| 154 | 25125083 | Nguyễn Đình Minh Huy | Fixed the level editor's F5 playtest routing to Game Over/main menu instead of popping back to the editor on death, with a mutation-tested regression case. | 1.5 | branch A/fix/editor-playtest-gameover, commit 2bd2f80 |
| 155 | 25125083 | Nguyễn Đình Minh Huy | Fixed MapGenerator's procedurally-generated vault exit pipe defaulting to the wrong entry mode; investigated the reported 'unreachable third Hard-mode axe' and found it does not reproduce, shipping a protective regression guard instead of an unjustified data edit. | 1.5 | branch A/fix/hard-axe-and-vault-pipe |
| 156 | 25125083 | Nguyễn Đình Minh Huy | Added a camera-distance gate on Endless Mode's ever-growing entity list, added ImGui sliders for the dynamic-lighting tunables, and deleted the dead UiRenderer::wrapText function. | 2 | branch A/fix/endless-gate-lighting-sliders-wraptext, commit 0fe7138 |
| 157 | 25125083 | Nguyễn Đình Minh Huy | Fixed MovingPlatform never advancing its own animation, flipped Spiny's default to spawn as an unhatched egg matching SPEC, and added a hardened post-condition regression test after review found the original test too weak. | 2 | branch A/fix/moving-platform-and-spiny-egg, commit cf195cb |
