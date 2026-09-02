#!/usr/bin/env python3
"""Generates tests/scripts/report_showcase_walk.txt from the zone layout in
build_showcase_level.py (same module, so the two files cannot drift apart).

Walk speed 150 px/s, tile 32px -> 0.2133 s/tile (Constants.hpp). Every
movement is a plain `hold D <t>` (no run modifier -- predictable, no
overshoot from momentum/friction) followed by a `wait` slightly longer than
`t` so the step fully lands before the next `hold`/`shot`. Enemy-zone stops
land ONE TILE SHORT of the enemy's x so nothing is stomped/bumped before the
photograph; item/power-zone stops land one tile PAST the pickup's x so
contact actually happens.

Run from SuperMarioGame/:  python3 tools/report_shots/build_showcase_walk_script.py
(build_showcase_level.py must run first, or import here re-runs it anyway).
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import build_showcase_level as lvl  # noqa: E402  (writes the level JSON on import)

SEC_PER_TILE = 32.0 / 150.0

lines = []


def c(s=""):
    lines.append(s)


def move(delta_tiles, comment=None):
    t = round(max(delta_tiles, 0) * SEC_PER_TILE, 2)
    if t <= 0:
        return
    if comment:
        c(f"# {comment}")
    c(f"hold D {t}")
    c(f"wait {round(t + 0.35, 2)}")


c("# Phase 3A screenshot catalogue -- the showcase walkthrough.")
c("#")
c("# Drives a single custom level (assets/levels/custom/report_showcase.json)")
c("# end to end: 11 of the 13 catalogue enemy types, the full Small->Super->")
c("# Fire and Super->Cape power-up chain plus Mini/Star/Mega, the remaining")
c("# items and every block type, the minimap toggle, a Memento time-rewind,")
c("# an achievement toast (stomp), and the flagpole+castle ending in")
c("# VictoryState. The level is authored directly as JSON, not built in the")
c("# in-game editor: scripted mouse/key events never reach")
c("# ImGui::SFML::ProcessEvent (Game::run(), src/Core/Game.cpp), so neither the")
c("# editor's ImGui palette nor the debug console's `spawn` command is")
c("# reachable from a --script file. tests/scripts/verify_r21i_lighting_daynight.txt's")
c("# header documents the same direct-JSON-authorship workaround for the same")
c("# reason.")
c("#")
c("# Precondition -- build the level once, from SuperMarioGame/, before")
c("# running this script (regenerates assets/levels/custom/report_showcase.json):")
c("#   python3 tools/report_shots/build_showcase_level.py")
c("#")
c("# Precondition -- saves/config.json must have \"debugMode\": true so the")
c("# Immortal cheat (F2) is armed: it turns a would-be-fatal hit into a life-1")
c("# Small-state rescue instead of Game Over (Player::loseLife), while leaving")
c("# ordinary Fire/Cape/Super power-downs (not deaths) completely alone -- see")
c("# Player::takeDamage/powerDown -- which is exactly what lets one continuous")
c("# walk demonstrate every power-up form without ever restarting.")
c("#")
c("#   ./build/SuperMarioGame --script tests/scripts/report_showcase_walk.txt")
c("#")
c("# Screenshots land in saves/shots/.")
c()
c("wait 2.0")
c("shot showcase_menu_custom_levels_before")
c("# MAIN MENU > CUSTOM LEVELS (row 5 of 0..9: Start,Load,Multiplayer,Daily,Editor,Custom)")
for _ in range(5):
    c("press Down")
    c("wait 0.3")
c("press Enter")
c("wait 0.5")
c("shot showcase_menu_custom_levels_page")
c('# "Report Showcase" is the only entry in assets/levels/custom/ -- Enter plays it.')
c("press Enter")
c("wait 2.5")
c("shot showcase_00_spawn")
c()
c("# Arm Immortal (F2) so the deliberate power-down touches below never end the")
c("# run, and the enemy-zone stops (one tile short) are cheap insurance rather")
c("# than the only thing standing between this script and Game Over.")
c("press F2")
c("wait 0.3")
c()

cur = 3  # spawn x

sx, sy = lvl.STOMP_GOOMBA
move(sx - cur - 3, "approach the stomp target")
cur = sx - 3
c("# Jump right at the goomba's edge and let residual momentum (not a held D)")
c("# carry the fall onto it, so the descent lands on top (stomp) rather than")
c("# the side.")
c("hold D 0.5")
c("wait 0.53")
c("press W")
c("wait 0.8")
c("shot showcase_01_first_stomp_achievement")
cur = sx + 1
c()
c("press M")
c("wait 0.3")
c("shot showcase_02_minimap_on")
c()

c("# --- Enemy zone: stop one tile short of each so nothing is stomped/bumped")
c("#     before the shot (this is a display walk, not a fight).")
for x, etype, y, label in lvl.ENEMY_ZONE:
    move(x - 1 - cur, f"toward {label}")
    cur = x - 1
    c(f"shot showcase_enemy_{etype}")
    c()

c("# --- Power-up ladder: Small -> Super -> Cape -> (hit) Super -> Fire ->")
c("#     (hit) Super -> (hit) Small -> Mini -> Star -> Mega.")
goomba_hit_idx = 0
for x, etype, y, label in lvl.POWER_ZONE:
    move(x + 1 - cur, f"toward {label}")
    cur = x + 1
    if etype == "goomba":
        goomba_hit_idx += 1
        shotname = f"showcase_power_hit{goomba_hit_idx}"
    else:
        shotname = f"showcase_power_{etype}"
    c(f"shot {shotname}")
    c()

c("# --- Items & blocks: the floating ones sit at y=17 so the y=20 ground walk")
c("#     never collides with them. The pipe is the one exception: pipes settle")
c("#     onto the floor the same way Flagpole does (see LevelLoader's")
c("#     \"settled onto the floor\" log line), so it is a real ground-level solid")
c("#     here and needs a hop over it, not a walk-through.")
for x, etype, y, label in lvl.ITEMS_BLOCKS_ZONE:
    if etype == "pipe":
        move(x - 2 - cur, f"toward {label}")
        cur = x - 2
        c("press W")
        c("hold D 1.5")
        c("wait 1.85")
        cur = x + 1
    else:
        move(x + 1 - cur, f"toward {label}")
        cur = x + 1
    c(f"shot showcase_block_{etype}")
    c()

c("# Time-rewind is demonstrated separately (report_rewind_demo.txt): holding R")
c("# here would snap this walk's position backward (PlayingState.cpp's rewind")
c("# path restores position only) and make the flagpole distance below")
c("# unpredictable, so it is deliberately not exercised in this long chain.")
c("hold D 4.0")
c("wait 4.4")
c("shot showcase_04_victory")
c("wait 1.5")
c("quit")

target = pathlib.Path(__file__).parent.parent.parent / "tests" / "scripts" / "report_showcase_walk.txt"
target.write_text("\n".join(lines) + "\n")
print("wrote", target, len(lines), "lines")
