# Weekly Report Writing Rules & Guidelines

These guidelines define the structure, process, and standards for producing weekly progress reports for the CS202 Super Mario Game project. All project contributors and AI agents must follow these rules strictly to ensure accurate, unified documentation.

---

## 1. Directory & Naming Conventions

* **Directory Structure**: Every weekly report must have its own directory under the root `docs/` folder, named `Group52_XX/` where `XX` is the two-digit zero-padded week number.
  - *Example*: `docs/Group52_05/` for Week 5, `docs/Group52_06/` for Week 6.
* **Markdown File**: The main report markdown file must be named exactly `52.md` inside that directory.
  - *Path Example*: `docs/Group52_05/52.md`
* **PDF Document**: If a compiled PDF is generated, it must be named `52.pdf` in the same directory.
  - *Path Example*: `docs/Group52_05/52.pdf`

---

## 2. Scheduling & Synchronization

* **Deadline**: Weekly reports must be compiled and pushed every Saturday by **23:59** of the current week.
* **Branching & Merging Policy**:
  - The report must be drafted and committed on the `dev` branch.
  - Before writing the report, the reporter must fetch all remote branches (`git fetch --all`) to capture the progress made on task branches (e.g. `A/...` and `B/...`).
  - Do **NOT** perform any auto-merges of feature branches to `dev` to write the report. Read the status of files and git history from the branches directly.
  - Compile summaries from the git log and local `logs/agent_history.log`.

---

## 3. Mandatory Report Structure

The report `52.md` must strictly adhere to the following sections:

### Title
```markdown
# Weekly Report: [WXX]
```
*(Replace `XX` with the zero-padded week number, e.g., `[W05]`)*

### Section 1: General Information
Provide metadata in a bulleted list:
* **Group ID:** 52
* **Group Name:** Group 52
* **Class:** 25A01
* **Project Name:** Super Mario Game
* **Date range:** `YYYY-MM-DD` – `YYYY-MM-DD` *(Sunday to Saturday)*
* **Github Repository:** URL of the repository

### Section 2: Tasks Completed This Week
Group completed tasks by developer and branch:
* Split into sub-sections for Member A and Member B:
  - **Member A**: Engine, Physics & Player/Item Entities
  - **Member B**: Input, Sound & Gameplay Systems
* For each developer, group tasks under headers denoting the branch name:
  - *Example*: `#### Branch: A/physics-input-test (Physics Refactoring & Camera System)`
* List each task as a bullet point with a bold title (e.g. `* **Task 1: Title**: description`).
* Provide a clickable path link to the main code header or implementation file updated during the task (e.g. `[Camera](file:///Users/huynguyen/Documents/CS202-Cpp/SuperMarioGame/SuperMarioGame/include/Graphics/Camera.hpp)`).

### Section 3: AI Usage Declaration
A bulleted list explaining what aspects of the design, algorithms, syntax transitions, or debug workflows were assisted by AI systems during the week.

### Section 4: Tasks Planned for Next Week
Specify the individual plans for the upcoming week for each member.

### Section 5: Issues & Resolutions
Record all critical bugs, compiler errors, or coordinate integration issues resolved this week. Each issue must be formatted as:
* **Issue Title**:
  - **Problem**: Describe the symptom and root cause technically.
  - **Resolution**: Detail the exact code logic, mathematical equations, or data structures introduced to fix the bug.

---

## 4. Documentation Quality Guidelines

* **Precision**: Avoid vague sentences (e.g., "fixed character movement"). Write technical details instead (e.g., "Moved passive horizontal deceleration/friction, active horizontal acceleration, and run speed limit scaling into the `PhysicsEngine::update()` loop").
* **File Links**: Use relative or absolute markdown links to reference file paths. Ensure all paths are clickable.
* **Accuracy**: Double check commit SHAs and filenames against the actual git status and file tree. Do not invent details.
