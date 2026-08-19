#!/usr/bin/env python3
"""Populate the authored campaign levels with enemies and question-block contents.

Why this exists
---------------
The seven shipped levels carried 2-3 enemies each across 200 tiles — roughly one
per 67 tiles, where Super Mario Bros. 1-1 runs about one per 14 — and the three
sub-levels had none at all. Separately, every one of the 59 question blocks
defaulted to holding a coin, so once the spawn path was fixed there were still
no power-ups anywhere in the game.

This places entities against the level's *actual* terrain rather than by hand:
it reconstructs the tile grid, finds real standing room, and refuses to drop an
enemy over a pit, inside geometry, or on top of the spawn or the flagpole.

Usage:
    python3 scripts/populate_levels.py --dry-run    # report only
    python3 scripts/populate_levels.py              # write the JSON
"""

import argparse
import json
import pathlib
import random
import sys

LEVELS = pathlib.Path("SuperMarioGame/assets/levels")

SOLID = {"ground", "brick", "question_block", "pipe", "ice", "conveyor", "used"}

# Enemy mix per theme. Ground enemies only — flyers and Thwomps need placement
# rules of their own, and a bad flyer is more disruptive than a missing one.
THEMES = {
    "level_1": ["goomba", "goomba", "goomba", "koopa_troopa"],
    "level_2": ["goomba", "koopa_troopa", "koopa_troopa", "spiny"],
    "level_3": ["goomba", "koopa_troopa", "spiny", "hammer_bro"],
    "bonus_1": ["goomba", "koopa_troopa"],
    "_sub":    ["goomba", "koopa_troopa"],
}

# Question-block contents, matching QuestionBlock::Content in the C++ header.
COIN, MUSHROOM, FIRE, CAPE, STAR, MINI, MEGA, ONEUP = range(8)

# Roughly one in three blocks holds something; the rest stay coins.
CONTENT_MIX = (
    [MUSHROOM] * 5 + [FIRE] * 4 + [CAPE] * 2 + [STAR] * 2 + [ONEUP] * 1 + [MINI] * 1
)

TILE = 32
SPAWN_CLEAR = 12   # tiles of calm at the start
GOAL_CLEAR = 8     # tiles of calm before the flagpole


def build_grid(level):
    """Reconstruct a {(x, y): type} grid from the run-length tile list."""
    grid = {}
    for t in level.get("tiles", []):
        ttype = t.get("type", "empty")
        x, y, w = t.get("x", 0), t.get("y", 0), t.get("w", 1)
        for dx in range(w):
            grid[(x + dx, y)] = ttype
    return grid


def standing_spots(grid, width, height):
    """Tiles with solid ground underfoot and two tiles of clear headroom."""
    spots = []
    for x in range(width):
        for y in range(height - 1):
            below = grid.get((x, y + 1))
            if below not in SOLID:
                continue
            if grid.get((x, y)) in SOLID:
                continue
            if grid.get((x, y - 1)) in SOLID:
                continue          # needs headroom to stand and walk
            spots.append((x, y))
            break                 # highest standing surface in this column only
    return spots


def has_run_room(grid, x, y, width_needed=3):
    """A patrolling enemy needs a few tiles of continuous floor, not a ledge."""
    for dx in range(-width_needed, width_needed + 1):
        if grid.get((x + dx, y + 1)) not in SOLID:
            return False
        if grid.get((x + dx, y)) in SOLID:
            return False
    return True


def populate(path, spacing, rng, dry_run):
    level = json.loads(path.read_text())
    name = path.stem
    width, height = level["width"], level["height"]
    grid = build_grid(level)
    entities = level.setdefault("entities", [])

    # --- keep-out zones -----------------------------------------------------
    occupied = {(int(e.get("x", -1)), int(e.get("y", -1))) for e in entities}
    flag_x = [int(e["x"]) for e in entities if e.get("type") == "flagpole"]
    goal_x = min(flag_x) if flag_x else width

    existing_enemies = sum(
        1 for e in entities
        if e.get("type") in {"goomba", "koopa_troopa", "koopa_paratroopa",
                             "spiny", "hammer_bro", "piranha_plant", "boo"}
    )

    # --- enemies ------------------------------------------------------------
    key = "_sub" if name.endswith("_sub") else name
    pool = THEMES.get(key, THEMES["_sub"])

    spots = [
        (x, y) for (x, y) in standing_spots(grid, width, height)
        if SPAWN_CLEAR <= x <= goal_x - GOAL_CLEAR
        and has_run_room(grid, x, y)
        and (x, y) not in occupied
    ]

    added, last_x = 0, -999
    for (x, y) in spots:
        if x - last_x < spacing:
            continue
        entities.append({"type": rng.choice(pool), "x": x, "y": y})
        occupied.add((x, y))
        last_x = x
        added += 1

    # --- question-block contents -------------------------------------------
    blocks = [e for e in entities if e.get("type") == "question_block"]
    filled = 0
    for i, block in enumerate(blocks):
        if "itemType" in block:
            continue
        # Deterministic-ish spread: every third block holds something.
        block["itemType"] = CONTENT_MIX[i % len(CONTENT_MIX)] if i % 3 == 0 else COIN
        if block["itemType"] != COIN:
            filled += 1

    density = width / max(existing_enemies + added, 1)
    print(f"  {name:16} enemies {existing_enemies:2} -> {existing_enemies + added:2}"
          f"  (1 per {density:.0f} tiles)   power-up blocks: {filled}/{len(blocks)}")

    if not dry_run:
        path.write_text(json.dumps(level, indent=2) + "\n")
    return added, filled


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--spacing", type=int, default=17,
                    help="minimum tiles between enemies in main levels")
    ap.add_argument("--seed", type=int, default=20260818)
    args = ap.parse_args()

    if not LEVELS.is_dir():
        sys.exit(f"level directory not found: {LEVELS} (run from the repo root)")

    rng = random.Random(args.seed)
    print(f"{'level':18} result")
    print("-" * 74)
    total_e = total_p = 0
    for path in sorted(LEVELS.glob("*.json")):
        # Sub-levels are short bonus rooms; keep them light.
        spacing = args.spacing * 2 if path.stem.endswith("_sub") else args.spacing
        e, p = populate(path, spacing, rng, args.dry_run)
        total_e += e
        total_p += p
    print("-" * 74)
    print(f"  {total_e} enemies added, {total_p} question blocks given contents"
          f"{' (dry run — nothing written)' if args.dry_run else ''}")


if __name__ == "__main__":
    main()
