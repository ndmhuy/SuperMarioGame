#!/usr/bin/env python3
"""Geometric solvability and difficulty for a level, with no agent involved.

Why this exists
---------------
Every agent-in-the-loop generator in the literature — Volz et al.'s latent-space
Mario GAN, PAIRED, EA SEED's adversarial RL — scores a level by having something
play it. That confounds two completely different facts:

    "this level cannot be finished"      (a property of the level)
    "this player cannot finish it"       (a property of the player)

A failed playthrough looks identical either way, and the confound is not
hypothetical. Our heuristic agent scores 10.8% on `level_1` — a hand-vetted,
shipped, obviously-winnable level. Used naively as a fitness function it would
reject a perfect level as broken, and during curriculum training it would report
every level as "too hard" forever.

PAIRED works around this with regret (protagonist minus antagonist, so shared
incompetence cancels); Volz works around it with A*, which is its own kind of
competence and costs a search per candidate. Both are workarounds for measuring
the wrong thing.

This module measures the level directly. It computes the exact set of tiles
reachable from the spawn under the game's *own* physics constants, by
exhaustively simulating every jump the player can make — no policy, no search
heuristic, no learning. It answers "does a path exist" in milliseconds, and it
answers it about the level rather than about a player.

The decomposition that falls out
--------------------------------
    reachable & agent wins        -> too easy for this agent
    reachable & agent fails       -> DIFFICULTY (or an agent gap) — curriculum material
    not reachable                 -> BROKEN — reject, and no agent needed to know it

That second row is the whole point: it is the only row that is worth training
on, and it is exactly the row the literature cannot isolate.

Difficulty as a controllable number
-----------------------------------
Reachability is computed as a graph, so every edge carries the *slack* it was
cleared with — how much of the player's physical envelope a jump actually used.
A gap needing 96% of maximum jump distance is hard; one needing 30% is a
formality. Difficulty is then a property you can measure before generating, and
target while generating, rather than an emergent property you sample and hope
for:

    difficulty = f(min slack, mean slack over the critical path, count of
                   near-limit jumps, forced-hazard proximity)

And when a level is *not* winnable, the reachability frontier names the exact
cells where progress stops — which is a repair instruction, not just a verdict.

Usage
-----
    solvability.py <level.json|grid.txt> [...]        one line per level
    solvability.py --json <level.json>                full report
    solvability.py --render <level.json>              ascii reachability map
"""

from __future__ import annotations

import argparse
import json
import sys
import heapq
from pathlib import Path

import level_tensor as lt

# --- physics, taken from include/Utils/Constants.hpp -------------------------
# Kept as a block so a drift between this file and the header is a one-line fix
# and is obvious in a diff. If the game's numbers change, these must follow, or
# the oracle silently starts answering about a different game.
TILE = 32.0
GRAVITY = 0.5 * 3600.0        # GRAVITY * GRAVITY_SCALE = 1800 px/s^2
WALK_SPEED = 150.0            # px/s
RUN_SPEED = 300.0             # px/s
JUMP_HEIGHT = 128.0           # px, ~4 tiles
DT = 1.0 / 60.0

# v0 from the kinematic identity h = v0^2 / 2g, so the arc this module simulates
# is the arc the engine produces rather than an approximation of it.
JUMP_V0 = (2.0 * GRAVITY * JUMP_HEIGHT) ** 0.5   # ~678.8 px/s

# Player box in tiles. Small Mario is the permissive case (1x1): a level that is
# winnable at all is winnable as Small Mario, since every power-up is optional
# and being larger only makes gaps harder to fit through.
PLAYER_W = 1
PLAYER_H = 1

# Cell classes that stop movement. PLATFORM counts as solid: a moving platform
# is a surface, and treating it as air would declare winnable levels broken.
SOLID = {lt.SOLID, lt.BREAKABLE, lt.QUESTION, lt.PIPE, lt.PLATFORM}
DEADLY = {lt.HAZARD}

# How finely the jump action space is sampled. Real play is continuous; this is
# a lower bound on what is reachable, so a level called winnable always is.
# Variable jump height is what a player gets by releasing the button early.
#
# Resolution matters here, and not only for accuracy: the difficulty scalar can
# only be as fine-grained as this tuple, because a move's demand is read off the
# cheapest hold that clears it. Rise scales with the square of the hold, so
# these are spaced to give roughly even steps in *height gained* (f^2 * 4 tiles)
# rather than even steps in f.
JUMP_HOLD_FRACTIONS = (0.25, 0.35, 0.45, 0.55, 0.65, 0.75, 0.85, 0.92, 1.0)
H_SPEEDS = (RUN_SPEED, WALK_SPEED, 0.0)
MAX_AIR_FRAMES = 240          # 4 s; longer than any arc the physics can produce


class Level:
    """A label grid plus the queries reachability needs."""

    def __init__(self, grid: list[list[int]], meta: dict):
        self.grid = grid
        self.meta = meta
        self.h = len(grid)
        self.w = len(grid[0]) if grid else 0

    @classmethod
    def load(cls, path: str) -> "Level":
        text = Path(path).read_text()
        if path.endswith(".json"):
            grid, meta = lt.encode(json.loads(text))
        else:
            grid, meta = lt.text_to_grid(text)
        return cls(grid, meta)

    def cell(self, tx: int, ty: int) -> int:
        if tx < 0 or tx >= self.w:
            return lt.SOLID          # off the sides is a wall
        if ty < 0:
            return lt.EMPTY          # above the level is open sky
        if ty >= self.h:
            return lt.EMPTY          # below the level is the void — not solid
        return self.grid[ty][tx]

    def solid(self, tx: int, ty: int) -> bool:
        return self.cell(tx, ty) in SOLID

    def deadly(self, tx: int, ty: int) -> bool:
        return self.cell(tx, ty) in DEADLY

    def box_blocked(self, tx: int, ty: int) -> bool:
        """Is the player's box, with its top-left at this tile, inside solid?"""
        for dx in range(PLAYER_W):
            for dy in range(PLAYER_H):
                if self.solid(tx + dx, ty + dy):
                    return True
        return False

    def box_deadly(self, tx: int, ty: int) -> bool:
        for dx in range(PLAYER_W):
            for dy in range(PLAYER_H):
                if self.deadly(tx + dx, ty + dy):
                    return True
        return False

    def standing(self, tx: int, ty: int) -> bool:
        """A valid foothold: the box fits, and something solid is underfoot."""
        if self.box_blocked(tx, ty):
            return False
        return any(self.solid(tx + dx, ty + PLAYER_H) for dx in range(PLAYER_W))

    def below_void(self, ty: int) -> bool:
        return ty > self.h + 1


def _simulate(level: Level, tx: int, ty: int, vx: float, v0: float):
    """One ballistic arc from a standing tile. Yields every tile it can land on.

    Integrated at the game's own timestep rather than solved in closed form,
    because the thing that decides whether a jump works is which tiles the box
    passes through on the way, not where the parabola ends.

    `v0` is the upward launch speed (0 for a walk-off-a-ledge fall). Yields
    (landing_tx, landing_ty, air_frames, peak_rise_tiles, horizontal_tiles).
    """
    x = tx * TILE
    y = ty * TILE
    vy = -v0
    start_x, start_y = x, y
    highest = y

    for frame in range(MAX_AIR_FRAMES):
        vy += GRAVITY * DT
        nx = x + vx * DT
        ny = y + vy * DT

        ntx, nty = int(round(nx / TILE)), int(round(ny / TILE))
        highest = min(highest, ny)

        # Horizontal wall: the arc is stopped in x but keeps falling. This is
        # what makes a too-tall step read as unreachable instead of being
        # silently teleported through.
        if level.box_blocked(ntx, int(round(y / TILE))):
            nx = x
            ntx = int(round(x / TILE))
            vx = 0.0

        if level.box_blocked(ntx, nty):
            if vy > 0:
                # Landed. The foothold is the tile just above the obstruction.
                land_y = nty - 1
                if level.standing(ntx, land_y) and not level.box_deadly(ntx, land_y):
                    yield (ntx, land_y, frame,
                           (start_y - highest) / TILE,
                           abs(ntx - tx))
                return
            # Hit a ceiling: stop rising, keep drifting.
            vy = 0.0
            ny = y

        x, y = nx, ny

        if level.box_deadly(int(round(x / TILE)), int(round(y / TILE))):
            return
        if level.below_void(int(round(y / TILE))):
            return


def _settle(level: Level, sx: int, sy: int):
    """Drop a spawn point onto the first foothold beneath it."""
    for y in range(max(sy, 0), level.h):
        if level.standing(sx, y):
            return (sx, y)
    # Spawns are given in level coordinates that may sit above the band; try the
    # whole column before giving up.
    for y in range(0, level.h):
        if level.standing(sx, y):
            return (sx, y)
    return None


def _moves_from(level: Level, node: tuple[int, int]):
    """Every foothold reachable in one move, with the envelope fraction it costs."""
    tx, ty = node
    moves = []

    # Walk one tile, then settle (step up one, or fall any distance).
    for step in (-1, 1):
        nx = tx + step
        if level.box_blocked(nx, ty):
            # Step up exactly one tile without jumping, as the engine allows.
            if not level.box_blocked(nx, ty - 1) and level.standing(nx, ty - 1):
                moves.append(((nx, ty - 1), 0.0, "step"))
            continue
        if level.standing(nx, ty):
            moves.append(((nx, ty), 0.0, "walk"))
        else:
            # Walked off an edge — fall.
            for land in _simulate(level, tx, ty, step * WALK_SPEED, 0.0):
                lx, ly, _f, _rise, run = land
                moves.append(((lx, ly), run / _max_run(), "fall"))

    # Jumps: every combination of hold length, horizontal speed, direction.
    for hold in JUMP_HOLD_FRACTIONS:
        for speed in H_SPEEDS:
            for direction in ((-1, 1) if speed else (0,)):
                for land in _simulate(level, tx, ty,
                                      direction * speed, JUMP_V0 * hold):
                    lx, ly, _f, rise, run = land
                    # Slack: how much of the maximum envelope this used.
                    # 0 is trivial, 1 is at the physical limit.
                    used = max(run / _max_run(), rise / (JUMP_HEIGHT / TILE))
                    moves.append(((lx, ly), min(used, 1.0), "jump"))

    return moves


def reachable(level: Level, start: tuple[int, int]):
    """Bottleneck search over the reachability graph.

    Plain flood-fill answers "can the player get there". That is necessary but
    it is not difficulty, and the difference is the point of this module.

    Difficulty is a **bottleneck** (minimax) quantity: the hardest move a player
    is *forced* to make is the minimum, over every route to a tile, of the
    hardest move on that route. Averaging along one arbitrary path gets this
    badly wrong in both directions — it calls a level hard when an easy detour
    exists, and it hides the single brutal jump that actually makes players quit
    behind a hundred trivial ones.

    So this is Dijkstra with `max` in place of `+`: the label of a node is the
    hardest move needed to reach it by the kindest available route. That label
    at the goal *is* the level's required difficulty, and the edge that sets it
    is the one to tune or repair.

    Returns (labels, parents) — labels[node] in 0..1.
    """
    settled = _settle(level, *start)
    if settled is None:
        return {}, {}

    labels: dict = {settled: 0.0}
    parents: dict = {settled: None}
    heap = [(0.0, settled)]
    done: set = set()

    while heap:
        cost, node = heapq.heappop(heap)
        if node in done:
            continue
        done.add(node)

        for target, edge_cost, _kind in _moves_from(level, node):
            # The route's difficulty is its hardest single move, not the sum.
            route = max(cost, edge_cost)
            if route < labels.get(target, 2.0):
                labels[target] = route
                parents[target] = node
                heapq.heappush(heap, (route, target))

    return labels, parents


def _max_run() -> float:
    """Horizontal tiles covered by a full-height jump at run speed."""
    air = 2.0 * JUMP_V0 / GRAVITY
    return (RUN_SPEED * air) / TILE


def _empty_report(reason: str, goal_col: int) -> dict:
    return {
        "winnable": False, "reason": reason, "reachableTiles": 0,
        "furthestColumn": 0, "goalColumn": goal_col, "widthFraction": 0.0,
        "frontier": [], "difficulty": 0.0, "requiredDifficulty": 0.0,
        "hardestOptional": 0.0, "slackAtGoal": 1.0, "route": [],
    }


def analyse(level: Level) -> dict:
    spawn = level.meta.get("spawnPoint", {"x": 3, "y": 19})
    flag = level.meta.get("flagpole", {"x": level.w - 2, "y": 18})
    goal_col = min(int(flag["x"]), level.w - 1)

    start = (int(spawn["x"]), max(int(spawn["y"]) - lt.BAND_TOP, 0))
    labels, parents = reachable(level, start)

    if not labels:
        return _empty_report("spawn has no foothold", goal_col)

    furthest = max(x for x, _ in labels)

    # The goal is a column, not a tile: a flagpole is tall and can be touched at
    # any height, so any foothold in its column (or the one before) finishes.
    at_goal = [n for n in labels if n[0] >= goal_col - 1]
    winnable = bool(at_goal)

    # The level's required difficulty: the kindest route to the goal is still
    # this hard. Nothing about a player enters this number.
    required = min((labels[n] for n in at_goal), default=1.0)

    route = []
    if winnable:
        node = min(at_goal, key=lambda n: labels[n])
        while node is not None:
            route.append(node)
            node = parents.get(node)
        route.reverse()

    # What the level asks of a completionist, versus what it asks to finish.
    # A wide gap here is a well-made level: optional challenge for the players
    # who want it, an easy line for the ones who do not.
    hardest_optional = max(labels.values()) if labels else 0.0

    return {
        "winnable": winnable,
        "reason": "" if winnable else "reachability stops short of the goal",
        "reachableTiles": len(labels),
        "furthestColumn": furthest,
        "goalColumn": goal_col,
        "widthFraction": round(min(furthest / max(goal_col, 1), 1.0), 4),
        # If it is broken, these columns are where progress stops — a repair
        # instruction rather than just a verdict.
        "frontier": sorted({x for x, _ in labels if x >= furthest - 1})[:12],
        "requiredDifficulty": round(required, 4),
        "difficulty": round(required, 4),
        "hardestOptional": round(hardest_optional, 4),
        "slackAtGoal": round(1.0 - required, 4),
        "routeLength": len(route),
        # The winning route itself, in WORLD tile coordinates (band offset
        # already applied). Each node is a foothold the kindest route stands
        # on, in order from spawn to goal. This is what the game feeds into
        # the agent's goal channel: dxToGoal/dyToGoal point at the next node
        # of a certified path instead of at the map's right edge.
        "route": [[x, y + lt.BAND_TOP] for (x, y) in route],
        "bottleneck": _bottleneck(labels, parents, route),
    }


def _bottleneck(labels: dict, parents: dict, route: list) -> dict:
    """Where on the winning route the hardest forced move happens.

    This is the actionable half of the difficulty number: it names one tile. To
    make a level easier, edit there; to tune a generated level to a target
    difficulty, that edge is the knob.
    """
    if len(route) < 2:
        return {}
    worst, worst_at = 0.0, None
    prev_label = labels.get(route[0], 0.0)
    for node in route[1:]:
        label = labels.get(node, 0.0)
        if label > prev_label and label >= worst:
            worst, worst_at = label, node
        prev_label = max(prev_label, label)
    if worst_at is None:
        return {}
    return {"x": worst_at[0], "y": worst_at[1] + lt.BAND_TOP,
            "demand": round(worst, 4)}


def render(level: Level) -> str:
    spawn = level.meta.get("spawnPoint", {"x": 3, "y": 19})
    start = (int(spawn["x"]), max(int(spawn["y"]) - lt.BAND_TOP, 0))
    visited, _ = reachable(level, start)
    out = []
    for y in range(level.h):
        row = ""
        for x in range(level.w):
            if (x, y) in visited:
                row += "o"
            elif level.solid(x, y):
                row += "X"
            elif level.deadly(x, y):
                row += "!"
            else:
                row += "."
        out.append(row)
    return "\n".join(out)


def write_waypoints(level_path: str, report: dict) -> str:
    """Write the certified route next to the level as `<stem>.waypoints.json`.

    Pixel coordinates target the player's FEET: x is the tile centre, y is the
    bottom edge of the foothold tile — the exact place a standing player's feet
    rest. AIController compares these against its own feet position, so both
    sides speak the same point on the body.
    """
    path = Path(level_path)
    out = path.with_name(path.stem + ".waypoints.json")
    waypoints = [{"x": (tx + 0.5) * TILE, "y": (ty + 1.0) * TILE}
                 for tx, ty in report.get("route", [])]
    out.write_text(json.dumps({
        "version": 1,
        "source": "tools/solvability.py",
        "level": path.name,
        "winnable": report["winnable"],
        "requiredDifficulty": report["requiredDifficulty"],
        "waypoints": waypoints,
    }, indent=1) + "\n")
    return str(out)


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("levels", nargs="+")
    p.add_argument("--json", action="store_true", help="full report per level")
    p.add_argument("--render", action="store_true", help="ascii reachability map")
    p.add_argument("--waypoints", action="store_true",
                   help="write <level>.waypoints.json sidecar next to each level")
    args = p.parse_args(argv)

    failures = 0
    for path in args.levels:
        level = Level.load(path)
        report = analyse(level)
        if args.waypoints:
            if report["winnable"]:
                print(f"  wrote {write_waypoints(path, report)}"
                      f"  ({report['routeLength']} nodes)")
            else:
                failures += 1
                print(f"  BROKEN — no sidecar for {path}")
            continue
        if args.render:
            print(f"=== {path} ===")
            print(render(level))
            continue
        if args.json:
            print(json.dumps({"level": path, **report}, indent=2))
            continue
        mark = "winnable" if report["winnable"] else "BROKEN  "
        if not report["winnable"]:
            failures += 1
        bn = report.get("bottleneck") or {}
        where = f"x={bn['x']},y={bn['y']}" if bn else "-"
        print(f"  {mark}  {Path(path).name:<22} "
              f"reach={report['widthFraction']*100:5.1f}%  "
              f"required={report['requiredDifficulty']:.3f}  "
              f"optional={report['hardestOptional']:.3f}  "
              f"tiles={report['reachableTiles']:<5} "
              f"bottleneck@{where}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    raise SystemExit(main())
