# Member Contributions & Manual Testing Tasks

## Overall Contribution Split

| Member | Role | Contribution Focus | Assigned % |
| :--- | :--- | :--- | :--- |
| **Member A (Huy Nguyen)** | Engine & Infrastructure | Game Loop, Physics Engine, Collision Systems, Serialization, UI/HUD, Rendering Pipeline, Level Editor, Advanced AI (Shadow Mario/Multiplayer), Overall Architecture. | 50% |
| **Member B (Partner)** | Entities & Gameplay | Entity Hierarchy, Player States (Power-ups), Enemy AI Strategies, Level Design, Sound/Audio, Polish & Game Feel, Edge Cases, Playtesting Fixes. | 50% |

---

## Manual Testing Task Division

To ensure a comprehensive defect audit, manual testing tasks are divided between the two members based on their domain knowledge.

### Member A: Engine, Infrastructure & UI Testing
1. **Core Loop & Performance:** Play through Levels 1-1, 1-2, and 1-3 to verify steady 60 FPS and memory stability (no leaks during long sessions).
2. **Physics & Collision:** Test edge cases in wall sliding, corner clipping, jumping through platforms, and high-speed movement.
3. **Save/Load & Meta-Game:** Complete a level, purchase an unlockable on the World Map, save, and reload to verify state persistence.
4. **Multiplayer & AI Modes:** Play a "Versus CPU" match and a "Shadow Chase" match. Verify camera tethering, split input bindings, and AI pathing.
5. **Level Editor:** Open the editor (F1), place new entities (e.g., Bowser, Spring), modify terrain, save to JSON, and load it successfully.
6. **Replay/Rewind:** Trigger time-rewind in the middle of a jump or after dying; verify state restores exactly without visual artifacts.

### Member B: Entities, Gameplay & Polish Testing
1. **Player States & Power-ups:** Test the entire state chain (Small -> Super -> Fire -> Cape). Verify hitboxes, damage downgrade sequence, and invincibility star frames.
2. **Enemy Behaviors:** Aggro every enemy type (Goomba, Koopa, Piranha Plant, Hammer Bro, Thwomp). Verify their patrol, chase, and attack patterns trigger correctly based on proximity.
3. **Boss Fights:** Engage Boom Boom and Bowser. Verify health scaling, phase transitions, fireball dodging, and the bridge-axe trigger logic.
4. **Interactables & Blocks:** Hit Question blocks, break Brick blocks, trigger a P-Switch, and use Warp Pipes. Ensure animations and item spawns are flawless.
5. **Audio & Accessibility:** Verify music loops seamlessly, sound effects trigger on time (jump, stomp, coin), and accessibility color-blind overlays function correctly.
6. **Edge Cases & Game Feel:** Test shell-bouncing chains, 100-coin 1UP logic, coyote-time jumping, and ensure the game feels authentic and responsive.
