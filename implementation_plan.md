# Implementation Plan — Standalone Enemy Behavior Test Suite

This plan details the design and implementation of a brand-new, isolated testing environment in `tests/verify_enemies_behavior.cpp` featuring side-by-side selection UI, dynamic terrain toggles, visual threat gizmos, and one-by-one behavior testing for all 11 enemies.

---

## 1. User Review Required

> [!IMPORTANT]
> **Key Architecture Decisions**
> 1. **Isolated Executable**: We will create a fresh test runner `tests/verify_enemies_behavior.cpp` and register it as `verify_enemies_behavior` target in `SuperMarioGame/CMakeLists.txt`.
> 2. **Side-by-Side Selection Layout**:
>    - **Left Sidebar**: List of 11 enemies (Goomba, KoopaTroopa, KoopaParatroopa, Boo, Thwomp, PiranhaPlant, ChainChomp, Lakitu, Spiny, HammerBro, BulletBill).
>    - **Center Room**: Visual render canvas displaying active enemy, mock player, and terrains.
>    - **Right Panel**: Environment configuration, interactive gizmo controls, and active enemy status.

---

## 2. Interactive Features & Custom Settings

### A. Environment & Terrain Toggles
Interactive checkboxes in the settings panel allow toggling block elements:
- **Flat Ground**: A base floor at \(y = 560\).
- **Ledge Block**: An elevated platform at \(y = 380\) to test edge-turn vs drop-off patrol.
- **Vertical Wall**: Solid boundary blocks to verify reflection rebounds on contact.
- **Piranha Pipe**: Green pipe block to house Piranha Plant.
- **Hammer Platform**: Higher tier platform for HammerBro hopping.

### B. Threat Range & Debug Gizmos
- **Show Bounding Box (AABB)**: Render outline green outlines of active boxes.
- **Show Attack Range Radius**: Render translucent circular range rings around attackers.
  - **Yellow / Gold**: Player is outside threat radius.
  - **Red / Crimson**: Player is inside threat radius (triggers attack actions).
- **Show Velocity Arrow**: Render cyan-to-yellow direction line showing vector force.

### C. Behavior Functions Under Test
1. **Goomba / KoopaTroopa / Spiny**: Verify `PatrolStrategy` (walking, wall-bouncing, edge-clamping).
2. **KoopaParatroopa**: Verify `FlyStrategy` (patrol flying loop).
3. **Boo**: Verify `ChaseStrategy` (chases only when player turns away, covers face when watched).
4. **Thwomp**: Verify `ProximityTriggerStrategy` (motionless at home, vibrating telegraph ramp-up when triggered, rapid drop, floor pause, slow rise).
5. **PiranhaPlant**: Verify `TimerEmergenceStrategy` (rises from pipe, retreats/stays suppressed when player stands near).
6. **ChainChomp**: Verify `TetheredChaseStrategy` (tether anchor point and range lunges).
7. **Lakitu**: Verify Lakitu movement pattern and dynamic `Spiny` egg throwing spawning.
8. **HammerBro**: Verify `HammerThrowStrategy` (facing player, throwing black hammer projectiles in parabolic paths, hopping).
9. **BulletBill**: Verify `LinearStrategy` (firing horizontally from cannon nozzle).

---

## 3. Proposed Changes

### [MODIFY] [CMakeLists.txt](file:///F:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/CMakeLists.txt)
- Register `verify_enemies_behavior` executable target with graphics and ImGui linkages.

### [NEW] [verify_enemies_behavior.cpp](file:///F:/My%20folder%20%28Gold%29/Uh%20school%20stuffs/University/2025-2026/CS202/Lab/SuperMarioGame/SuperMarioGame/tests/verify_enemies_behavior.cpp)
- Core visual suite implementing side-by-side layout, dynamic terrains, mock invincible player, threat range calculations, and rotating hammer projectile spawns.

---

## 4. Verification Plan

### Manual Verification
1. Re-generate CMake and compile target:
   ```powershell
   cmake --build . --target verify_enemies_behavior
   ```
2. Run visual executable:
   ```powershell
   .\Debug\verify_enemies_behavior.exe
   ```
3. Verify each of the 11 enemies one-by-one by selecting them from the left sidebar and observing their reactions.
