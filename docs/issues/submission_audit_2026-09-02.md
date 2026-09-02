# Submission audit — 2026-09-02 (INCOMPLETE — stopped early at the user's instruction)

**What this is.** The Phase 6 independent audit of the CS202 SuperMarioGame submission package
(the generated report, `submission_documents/`, weeklies W11–W13, and the claims the sweep's own
plan §8 makes). Written by an agent that authored none of the audited material, against
`dev @ 87469f9` in worktree `smg-lanes/audit`, branch `A/docs/submission-audit`.

**Who it is for.** The lane owner(s) who fix the items in §3 before Phase 7 packages the submission,
and the re-audit that follows. Every finding names file and line so a fix lane need not re-derive it.

**Status: INCOMPLETE.** The audit was stopped on 2026-09-02 ~21:05 by the coordinator on the user's
instruction (session quota). Every check in §2 marked PASS/FAIL was actually run; §4 lists the
checks that were **never run** so the gap is on the record rather than looking like a pass.

**Tally: 33 checks run — 19 PASS, 12 FAIL (5 of them minor), 1 RESOLVED-on-branch, 1 user-accepted gap.**
Two more findings are SUSPECTED only (not verified) and are labelled as such. The 12 FAILs are prose
and packaging defects; no defect in the game's code was found or claimed.

**What was audited.** `dev @ 87469f9` only. The §6/§7 rewrite lives on the unmerged branch
`A/docs/report-patterns-oop @ 7f3d5c9`; where it changes a verdict this is stated per finding, from
reading that branch's `reports/report_content.py` (not from a build of it). The `FACTS` fix lives on
the unmerged `A/docs/facts-and-index-counts @ 1f1ec63`.

---

## 1. Build and render (the report as it stands on `dev`)

`Report/SuperMarioGame/build.sh` run in this worktree at 20:52: exit 0, **70 pages**, `head 87469f9`,
`Overfull \hbox warnings: 0` — **the 5 pt guard ran and passed** on `dev`'s prose. (The coordinator
reports the guard now **fails** at 50.71 pt on the unmerged §7.2 prose from `A/docs/report-patterns-oop`;
not observed here because that branch was not built.) All 70 pages were rendered with
`pdftoppm -r 150` and each one looked at.

Regenerated outputs (`Report/…/Group52_SuperMarioGame_Report.pdf`, `content/report_body.tex`,
`reports/*.html`) were restored to HEAD afterwards; nothing generated is committed by this lane.

---

## 2. PASS / FAIL per check

Legend: **V** = verified by command or by reading the source; **S** = suspected, not verified.

| # | Check | Result | Evidence |
| :-- | :--- | :--- | :--- |
| 1 | Report builds; Overfull guard ran | **PASS** (V) | build log: 0 warnings, exit 0, 70 pp |
| 2 | Every PDF page inspected: no text overlapping a column, no clipped figure, no broken table | **PASS** (V) | 70/70 pages viewed; none found |
| 3 | Empty / near-empty pages | **FAIL (minor)** (V) | p.6 carries two sentences then blank (Figure 6 floated to p.7); p.4 half-empty; p.8 figure then 40 % blank. Float placement, not overlap |
| 4 | Detailed-UML legibility | **PASS with caveat** (V) | Landscape parts (pp.54–58, 60–64) legible; Enemy part 2 and Item/Block/Strategy portrait figures (pp.51, 52, 55, 66) are ~4 pt effective — readable only zoomed, consistent with lane 5D's 3.7–8.6 pt measurement |
| 5 | Tracked PDF deliverables current | **FAIL** (V) | `Report/SuperMarioGame/Group52_SuperMarioGame_Report.pdf` and `submission_documents/Group52_SuperMarioGame_Report.pdf` are byte-identical, **58 pages, CreationDate 2026-08-31**; the source at `87469f9` builds 70 pages. `submission_documents/Group52_SuperMarioGame_Report.md` (a third copy) says "Verified against commit a159276 · 2026-08-31". Committed `reports/Group52_SuperMarioGame_FinalReport.html` stamp: `edbae74` |
| 6 | Report date stamps consistent | **FAIL (minor)** (V) | `reports/report_content.py:99` hard-codes `2026-08-31` on the HTML cover; the PDF cover uses `\today` (`content/title.tex:31`) → "2nd September 2026". Two editions, two dates |
| 7 | 12 singletons, not 4 (or 13) | **FAIL on dev / fixed on r5a** (V) | Code: 12 (`grep 'static .*& *getInstance' include` → 11 + `EventBus.hpp:65`). `report_content.py:378` says **"13 managers"**. r5a `:495,:652` says twelve |
| 8 | 9 game states incl. `EditorState`, no `StatisticsState` | **FAIL** (V) | Code: 9 (`grep ': public IGameState'`), no `StatisticsState`. Report says **Eight** at `report_content.py:237` (§4 table), `:334` (Fig. 10 caption), `:1434` (§16.3) and "(8 screens)" at `:387`. r5a fixes `:334`/§7 but **leaves `:237` and `:1800` at Eight** |
| 9 | 8 movement strategies, no Swim | **PASS** (V) | 8 `: public IMovementStrategy`; no `SwimStrategy` anywhere; report says eight (`:340`, p.14, p.19) |
| 10 | `Hud` subscribes to nothing | **PASS** (V) | `grep subscribe\|EventBus include/Graphics/Hud.hpp src/Graphics/Hud.cpp` → nothing; dev report does not claim it; r5a `:935` says so explicitly |
| 11 | `ICommand` has no undo / serialization | **PASS** (V) | `include/Core/ICommand.hpp`: `execute(Character&)` only. Dev `:394` attributes undo to `IEditorCommand` (7 concretes, verified) — acceptable; r5a `:811` states it outright |
| 12 | Object Pool = three projectile types | **PASS** (V) | `PlayingState.hpp:842-844` (`Fireball`, `Hammer`, `BossFireball`); report `:405` lists exactly those. Note `:619` "Particles run on an object pool" — `ParticleSystem.hpp:54` is a `std::vector<Particle>` with an `active` flag, not `ObjectPool<T>`; wording, not a claim of the pattern |
| 13 | `GameSnapshot` is a public aggregate | **PASS** (V) | `GameSnapshot.hpp:44-60` plain `struct`; report §13.1.5 (p.42) says so |
| 14 | `Thwomp`/`FallingPlatform` not State participants | **FAIL** (V) | `report_content.py:387` lists both under **State**; `:288` Fig. 6b caption "Ten blocks, three of them stateful. FallingPlatform and Thwomp are State machines" — **Thwomp is an `Enemy`** (`Thwomp.hpp:7`), it is not in the Block figure, and "three" names two; `:1430` repeats it. r5a rewrites `:288` ("two of them stateful … not State-pattern participants") but **still names Thwomp in the Block caption and leaves `:1796` unchanged** |
| 15 | 29 event types | **FAIL on dev / fixed on r5a** (V) | `EventBus.hpp:8-46` → 29. Dev `:383` says **"35+ event types"**. r5a `:926` says 29 |
| 16 | Visitor not claimed | **PASS** (V) | No "Visitor" in dev report, README, SPEC, submission docs; r5a `:554,:1114` lists it as deliberately rejected |
| 17 | Lane 2E fixes not described as unfixed | **PASS** (V) | `Camera.hpp:69` `const sf::View& getView() const`; `DevPanel.cpp` raw `new` = 0; `SoundManager.hpp:93` `std::vector<EventBus::ScopedSubscription>`; `Entity.hpp:126` `poolTag()`. Dev prose does not mention them; r5a `:377,:387,:434,:510` describes them as done |
| 18 | No doc claims the third HARD axe was unreachable or fixed | **PASS** (V) | `level_3.json` unchanged since `6af4f8e` (`git log 6af4f8e..HEAD -- assets/levels/` empty); report §12.4.1 (p.38) records it as withdrawn; `features_list.md` does not claim it |
| 19 | `UiRenderer::wrapText` not listed as a feature | **PASS** (V) | Only mention: `features_list.md:252` ("deleted") and the `UiRenderer.hpp:106` comment explaining its absence |
| 20 | Prose counts equal `FACTS` | **PASS** (V) | Every number in the Abstract, §12, §13, §16.3 matches the printed `FACTS` block |
| 21 | `FACTS` equals reality | **FAIL on dev → RESOLVED on branch** (V) | `ctests` extracted **29**, `ctest -N` in the main tree's build → **32** (3 explicit `add_test(NAME …)` guards). Printed as 29 on pp.1, 39, 48. `A/docs/facts-and-index-counts @ 1f1ec63` adds the explicit-`add_test` regex and `verify_ctest_parity()`; diff read and is correct. Not on `dev` at audit time. All other FACTS re-counted and match (harnesses 40, targets 38, enemies 11+2, items 13, blocks 10, players 5, sessions 296, weeklies 10, commits 495) |
| 22 | "40 harnesses; 38 of them are built by CMake" | **FAIL (minor)** (V) | `report_content.py:1087`, `:1399`: all 40 are built — the two guards via `add_executable` (`CMakeLists.txt:258`, `:283`); 38 is the `add_verify_test` count, not "built" |
| 23 | Hard-typed numbers in report vs measured | **FAIL** (V) | `:175` Member A "179" commits / `:182` "57" while the prose says "from `git shortlog -sne`" — `git shortlog -sn HEAD` → 419 / 57+4; `Member_Contributions.md:20` says 419 measured at `30f00d8`. `:212` "SoundManager subscribes to 19 event types" — 20 (`m_subscriptions.emplace_back` ×20, 20 distinct `EventType::`). `:1372` "`ctest … # 13 verification targets`" — stale (32). r5a leaves all four |
| 24 | Features list 97 → 127: sampled new items have a production call path | **PASS** (V) | 23 of 30 traced to `src/Core` or `src/Entities` (not tests): #98 `PlayingState.cpp:2011,2790`; #99 `DevPanel.cpp:422`; #100 `DevPanel.cpp:378-391`; #101 `EditorState.cpp:158,353` + 7 `IEditorCommand`; #102 `MenuState.cpp:145`; #103 `EditorState.cpp:293,405`; #104 42 `EntityType` values (counted); #105 `DevPanel.cpp:193-197`; #106 `PlayingState.cpp:644`; #107 `AchievementManager.cpp:59`, `StatisticsTracker.cpp:47`, `GameOverState.cpp:72`; #108 `PlayingState.cpp:1826`; #109 `DebugConsole.cpp:284`; #110 `MapGenerator.cpp:649`; #113 `PlayingState.cpp:3102`; #115 `Lakitu.cpp:56,105`; #117 `PlayingState.cpp:2748`; #118 `Hud.cpp:365-403`; #119 `PlayingState.cpp:906`; #120 `MenuState.cpp:99-106`; #121 `PlayingState.cpp:1020`; #123 `SoundManager.cpp:189` (0.12); #126 `MenuState.cpp:239`, `Serializer.cpp:429`; #127 `PlayingState.cpp:2451`. Sub-claims **not verified**: #99's "100-second" cycle, #101's exact hotkeys, #105's F2–F11 |
| 25 | Features-list preamble placement claims | **PASS** (V) | Parsed `assets/levels/*.json`: Paratroopa ×3 + Lakitu ×1 (level_1), Boo ×2 (level_2), Thwomp ×2 + Bullet Bill ×3 + Chain Chomp ×1 (level_3); `hidden_block` 12 total; no `itemType` 5 or 6 in any question block. 7 level files ✓ |
| 26 | Placeholders | **PASS** (V) | Only the three deliberate `[INSERT … LINK HERE]` in `demo_video_links.md:5-7` |
| 27 | Links / paths resolve | **FAIL** (V) | 307 references checked. Dangling: `docs/Group52_11/52.md:36,38,243` → `../issues/code_audit_2026-08-18.md`, `../issues/member_a_fix_plan.md` (archived by Phase 1 to `docs/archive/2026-09-02_*.md`); `docs/Group52_13/52.md:43` → `SuperMarioGame/include/Graphics/AnimationManager.hpp` (deleted). Report: all 18 anchors resolve; `docs/rl_training.md` (`:1295`) is correctly described as living on the unmerged `A/mapgen-gan-plan` — not a defect |
| 28 | g-rule-21 learning records | **known, user-accepted gap — not a FAIL** | Descoped by the user. No document links to the two undelivered records (dev and r5a grep clean); `docs/learning/mid-frame-entity-spawn-crash.html` exists and is the only one referenced |
| 29 | Weeklies W11/W12/W13 | **PASS** (V) | All five mandatory sections present in each; commit counts re-run on `dev`: W11 100 (98/2), W12 1, W13 to 09-02 18:54 175 (172/3) |
| 30 | `Member_Contributions` stamp and numbers | **PASS** (V) | "measured at 30f00d8 on 2026-09-02"; `git shortlog -sn 30f00d8` → 419 / 57 / 4 = 480 ✓; 157 task rows ✓ |
| 31 | AI declaration dates vs log | **PASS with caveat** (V) | "from 2026-08-18": first entry `[2026-08-18 15:02:00]` is the one that adds `CLAUDE.md` — consistent; R-batch start `[2026-08-31 08:45:00]` ✓. "Antigravity was the primary tool" is **not verifiable from the log** (no entry before 08-31 names a tool) |
| 32 | `class_diagram.md` current | **FAIL (minor)** (V) | Header says GENERATED; regenerating with `tools/gen_class_diagram.py --mermaid` differs by 416 lines (missing `poolTag`, `translate`, `onLeftLevel`…). Last commit `2200088`. The report (p.5, p.26) cites it as the cured example of drift |
| 33 | README claims | **FAIL** (V) | `README.md:106` "10 game states (incl. EditorState)" — 9; `:107` "SoundManager (21)" — 20; `:68` "Special Sections: Swimming" — descoped in SPEC §21. (First two also reported by lane 5A's log.) |

**SUSPECTED (not verified — no time):**
- S1. Report §16.3 `report_content.py:1402` "Achievements 12" vs `features_list.md` #94 "ten tracked milestones". One of them is wrong; `AchievementManager` was not read.
- S2. `submission_documents/*.pdf` for `features_list`, `testing_tasks`, `Member_Contributions`, `AI_Usage_Declaration` were not compared against their `.md`; given finding #5 they may also be stale.

---

## 3. Prioritised fix list

1. **Rebuild and re-copy the tracked report PDFs before packaging** (finding 5): run `Report/SuperMarioGame/build.sh` on the final `dev`, copy to `submission_documents/Group52_SuperMarioGame_Report.pdf`; regenerate or archive `submission_documents/Group52_SuperMarioGame_Report.md` (third copy, g-rule-15). Do the same for the committed `reports/*.html`.
2. **Merge `A/docs/facts-and-index-counts @ 1f1ec63`** (finding 21) so pp.1/39/48 print 32.
3. **`reports/report_content.py` counts** — on whichever branch lands: `:237` Eight→nine; `:1434`/r5a `:1800` Eight→Nine; `:378` 13→12; `:383` 35+→29; `:387` remove `FallingPlatform, Thwomp` from State; `:288` and `:1430` (r5a `:1796`) drop Thwomp from the *Block* caption and fix "three"→"two"; `:212` 19→20; `:175/:182` replace 179/57 with `git shortlog` values or route through `FACTS`; `:1087`/`:1399` "38 built"→"38 via add_verify_test, all 40 built"; `:1372` "13 verification targets"→`{F['ctests']}`; `:99` date → today's or derive from git.
4. **README.md** `:68`, `:106`, `:107` (finding 33).
5. **Weeklies** `docs/Group52_11/52.md:36,38,243` → `../archive/2026-09-02_code_audit_2026-08-18.md`, `../archive/2026-09-02_member_a_fix_plan.md`; `docs/Group52_13/52.md:43` unlink or point at the deleting commit.
6. **`class_diagram.md`**: regenerate (`cd SuperMarioGame && python3 tools/gen_class_diagram.py --mermaid > ../class_diagram.md`) or add it to the report build.
7. Resolve S1 (achievement count) by reading `AchievementManager`.
8. Optional: `\FloatBarrier` or `[H]` on Figure 6/6a to reclaim pp.4/6/8.

---

## 4. Checks NOT run (stopped early)

- No game run, no `verify_*` binary executed, no `ctest` run by this lane (the 32 count is from `ctest -N` in the main tree's existing build).
- The unmerged `A/docs/report-patterns-oop` was **not built or rendered**; its §6/§7 was checked by grep only. Its reported 50.71 pt overfull was not observed here.
- `submission_documents/*.pdf` staleness vs `.md` (S2); `Member_Contributions.xlsx` parity with `.md`.
- `demo_video_requirements.md`, `testing_tasks.md` row *content* (only its 1–127 range coverage was checked — complete).
- Report §8–§10 technical narrative (collision fixes, crash analysis) — read, not traced to commits.
- "29 effects / 12 music tracks" (`:212`, `:1401`) — not counted.
- Feature sub-claims listed under finding 24; features 1–97 not re-sampled.
- Weeklies W04–W10; `docs/learning/index.html` content.
- `SPEC.md` §2.2 was **not** treated as a defect (it is the frozen spec the report argues against).

---

## 5. Commands used (for the re-audit)

```bash
cd smg-lanes/audit && git fetch --all && git rev-list --left-right --count dev...origin/dev
Report/SuperMarioGame/build.sh                     # 0 overfull, 70 pp
pdftoppm -r 150 -png Report/SuperMarioGame/Group52_SuperMarioGame_Report.pdf pages/p
(cd /path/to/main-tree/SuperMarioGame/build && ctest -N | tail -1)   # Total Tests: 32
grep -rn 'static .*& *getInstance' SuperMarioGame/include | wc -l     # 11 (+ EventBus)
grep -rn ': public IGameState' SuperMarioGame/include | wc -l         # 9
grep -c 'm_subscriptions.emplace_back' SuperMarioGame/src/Core/SoundManager.cpp   # 20
git shortlog -sn 30f00d8 ; git shortlog -sn HEAD
python3 SuperMarioGame/tools/gen_class_diagram.py --mermaid | diff - class_diagram.md | wc -l
```
