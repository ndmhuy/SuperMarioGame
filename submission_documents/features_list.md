# List of Features (0.25pts each — Total 10pts)

## 1. Core Engine & Architecture
1. **Game State Management (State Pattern)** - Smooth transitions between menu, playing, pause, and game over states.
2. **Resource Management (Singleton)** - Centralized texture, font, and sound loading/caching.
3. **Event System (Observer Pattern)** - Decoupled messaging between entities, UI, and audio.
4. **Input Handling (Command Pattern)** - Configurable key bindings mapped to actionable commands.
5. **Object Pooling** - Reusing projectile and particle instances to prevent memory fragmentation.
6. **Data Serialization** - Saving and loading game states (unlocks, levels) via JSON.

## 2. Physics & Collision
7. **AABB Collision Detection** - Axis-Aligned Bounding Box checks for all game objects.
8. **Spatial Hashing** - Optimized broad-phase collision detection for large levels.
9. **Kinematic Physics Engine** - Sub-pixel movement, gravity, friction, and acceleration.
10. **Collision Resolution** - Correcting overlaps and handling directional impacts (top, bottom, sides).

## 3. World & Level Architecture
11. **TileMap Rendering** - Efficient grid-based rendering of static level geometry.
12. **Camera System** - Dynamic scrolling camera that follows the player(s) with boundary clamping.
13. **Level Loader** - Parsing level layouts, entity placements, and properties from JSON files.
14. **Parallax Backgrounds** - Multi-layered scrolling backgrounds providing depth.
15. **Level Editor** - In-game ImGui editor for placing tiles, entities, and playtesting dynamically.

## 4. Player Mechanics
16. **Player States** - Small, Super, Fire, Cape, and Mini Mario states with distinct hitboxes and abilities.
17. **Advanced Movement** - Variable jump height, sprint mechanics, jump buffering, and coyote time.
18. **Power-up Interactions** - Dynamic state transitions upon collecting Mushrooms, Fire Flowers, etc.
19. **Invincibility (Starman)** - Temporary immunity to damage with visual effects and unique music.
20. **Death Sequence** - Classic freeze-and-fall animation upon dying.

## 5. Enemies & AI
21. **Entity Factory (Factory Pattern)** - Dynamic instantiation of 13+ enemy types.
22. **Enemy AI (Strategy Pattern)** - Configurable behaviors like patrol, chase, and fly.
23. **Goomba & Koopa Troopa** - Classic walking enemies and interactable shells.
24. **Complex Enemies** - Piranha Plants (timer emergence) and Hammer Bros (projectile throwing).
25. **Boss Fights** - Boom Boom and Bowser with multi-phase health and unique patterns.
26. **Shadow Mario AI** - Replay-based AI that chases the player's previous path.

## 6. Items & Interactables
27. **Question Blocks & Brick Blocks** - Bumping mechanics that spawn items or break based on player state.
28. **P-Switch & POW Block** - Global level interactions (coin transformation, screen shake).
29. **Warp Pipes** - Transitions between overworld and underground sub-areas.
30. **Moving & Falling Platforms** - Kinetic platforms for advanced platforming challenges.
31. **Flagpole Completion** - Level end sequence calculating score based on grab height.

## 7. Graphics, UI & Polish
32. **Sprite Animation System** - Frame-based animations synchronized with entity states.
33. **HUD (Heads-Up Display)** - Real-time tracking of score, coins, time, and lives.
34. **Minimap** - Overlay showing the entire level structure and entity positions.
35. **Particle System** - Visual effects for block breaking, fireballs, and scoring.
36. **Screen Transitions** - Fade in/out and screen shake effects.

## 8. Meta-Game & Extra Modes
37. **Multiplayer Modes** - Two-Player Versus, Co-op, and AI opponent integration.
38. **Achievement System** - Tracking milestones and unlocking rewards.
39. **Replay System & Rewind (Memento Pattern)** - Recording game snapshots and allowing playback.
40. **Meta-Game Features** - World map navigation, High Scores, and Accessibility options.
