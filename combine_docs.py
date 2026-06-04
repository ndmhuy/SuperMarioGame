import os

files_to_read = [
    ("FEATURE_PROPOSAL.md", "Part 2: Feature Expansion Proposal"),
    ("CS202_FinalProject_SuperMario_Spec.md", "Part 3: Original Assignment Specification"),
    ("SPEC.md", "Part 4: Global Specification (Frozen)"),
    ("implementation_plan.md", "Part 5: System Architecture & Implementation Plan"),
    ("TASKS.md", "Part 6: Sequential Task Checklist"),
    ("AGENTS.md", "Part 7: Agent Instructions & Guidelines"),
]

intro = """# CS202 Final Project: Super Mario Game — Proposal, Specification & Implementation Plan

> **Student**: Nguyễn Đình Minh Huy
> **Student ID**: 25125083
> **Course**: CS202 — Object-Oriented Programming (C++)
> **Date**: June 3, 2026
> **Project**: Super Mario Bros. (2D Platformer in C++17 with SFML & ImGui)

---

## Part 1: Official Proposal & Request for Expanded Architectural Scope

### 1.1 Executive Request
Dear Teaching Assistant (TA),

I am formally requesting approval to pursue an **expanded architectural scope** of **110 features** (an increase of 116% over the default 51-feature specification) for my CS202 Final Project. This expansion is designed to demonstrate a professional-grade mastery of Object-Oriented Programming (OOP) principles, software design patterns, and systems programming in C++17.

### 1.2 Justification of Competency & Past Performance
In my past two projects (including a Personal Finance Manager and a Data Structures Visualization), I have consistently designed systems with highly rigorous OOP principles, maintaining strict encapsulation, low coupling, and robust polymorphism. By demonstrating these design principles in previous assignments, I have verified my competence in basic and advanced OOP concepts. 

For this final project, I want to challenge myself by diving deeper into the architecture of a complex game engine. Expanding the project scope to 110 features allows me to explore and implement advanced system architecture, memory management, and cross-cutting design patterns. I have structured the codebase with 25+ concrete entities and 10+ design patterns to prove my capabilities in software engineering and OOP design. Because of my proven competency, I require more space for the architecture and want to delve into deeper parts of the implementation plan. I ensure my competencies in OOP.

### 1.3 Key Elements Demonstrating OOP Excellence in the Expanded Scope
The proposed 110-feature architecture utilizes OOP in the following ways:
1. **Deep Inheritance Hierarchy**: A 4-level deep abstract tree:
   `Entity` -> `Character` -> `Enemy` / `Player` -> 25+ concrete leaf classes (e.g., `BoomBoom`, `ChainChomp`, `Trampoline`, `MovingPlatform`).
2. **Advanced Polymorphism**: The physics engine treats all objects as `Entity*` in a Spatial Hash grid, resolving collisions polymorphically while specific movements are delegated to swappable strategies.
3. **10 Software Design Patterns**:
   - **Factory Pattern**: Dynamically spawning 25+ entity types from JSON layouts.
   - **Singleton Pattern**: Managing resource lifecycles (`ResourceManager`), audio channels (`SoundManager`), achievements (`AchievementManager`), and the overall game loop (`Game`).
   - **State Pattern**: Controlling game states (9 screens) and character forms (5 player states, plus platform lifecycles and Thwomp state machines).
   - **Observer Pattern**: Using a decoupled `EventBus` to notify systems of 15+ game events.
   - **Strategy Pattern**: Driving enemy AI with 7+ swappable strategies (Patrol, Chase, Fly, Swim, tether, etc.).
   - **Command Pattern**: Encapsulating player input actions, debug console commands, and replay logs.
   - **Decorator Pattern**: Dynamically wrapping player states for temporary power-ups (Star, Mega) without inheritance bloat.
   - **Memento Pattern**: Capturing state snapshots for a time-rewind system.
   - **Object Pool Pattern**: Recycling memory for high-frequency fireballs and particles.
   - **Template Method Pattern**: Structuring the execution pipeline of AI strategies.
4. **Data-Driven Architecture**: Fully isolating level layouts and entity attributes into external JSON files, adhering to the Open/Closed Principle.

I guarantee my readiness to deliver this high-quality engine within the project timeline, and I appreciate your review of this consolidated implementation plan.

---

"""

out_file = "/Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/25125083.md"
base_dir = "/Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/"

with open(out_file, "w") as f_out:
    f_out.write(intro)
    
    for filename, section_title in files_to_read:
        filepath = os.path.join(base_dir, filename)
        f_out.write(f"## {section_title} ({filename})\n\n")
        with open(filepath, "r") as f_in:
            f_out.write(f_in.read())
        f_out.write("\n\n---\n\n")

print(f"Successfully wrote everything to {out_file}")
