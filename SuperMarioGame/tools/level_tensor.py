#!/usr/bin/env python3
"""JSON level <-> semantic tile tensor, both directions.

This is the contract between the game's level format and any learned map
generator (see docs/level_tensor_contract.md for the reasoning behind the
vocabulary; this file is the executable half of it).

The canonical in-memory form is a **label grid**: a list of BAND_HEIGHT rows,
each a list of ints in [0, len(VOCAB)). One-hot is a derived view (`one_hot`),
so encode/decode/roundtrip need no third-party packages at all. numpy is
imported lazily and only by the corpus/npz commands.

Commands
--------
    level_tensor.py encode  <level.json> [out.txt]   JSON  -> label grid (text)
    level_tensor.py decode  <grid.txt> <out.json> [--theme overworld]
    level_tensor.py check   <level.json> [...]       roundtrip self-test
    level_tensor.py corpus  <out.npz> <level.json> [...]   sliding-window .npz

Run `check` on the campaign levels after any change to the vocabulary.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

# --- The contract ----------------------------------------------------------
#
# Bump this whenever VOCAB, BAND_TOP/BOTTOM or the encoding rules change. It is
# written into every .npz and every generated level so a weight file trained
# against one vocabulary can never be silently decoded with another — the same
# reason kAIObservationVersion exists on the RL side.
CONTRACT_VERSION = 1

# Semantic vocabulary, NOT the game's TileType enum.
#
# Ground/Ice/Conveyor are one SOLID class because they are the same thing to a
# player and to a generator: a surface you stand on. Which one gets emitted is
# the *theme's* decision, made at decode time. Teaching a generator to
# distinguish them would spend capacity on a choice that is a config field.
EMPTY, SOLID, BREAKABLE, QUESTION, PIPE, HAZARD, COIN, ENEMY, PLATFORM = range(9)

VOCAB = [
    "empty",      # 0  air
    "solid",      # 1  ground / ice / conveyor — the theme picks which
    "breakable",  # 2  brick
    "question",   # 3  ? block (tile or entity form)
    "pipe",       # 4  pipe body
    "hazard",     # 5  lava / water — touching it kills
    "coin",       # 6  collectable coin (tile or entity form)
    "enemy",      # 7  generic enemy; the theme/difficulty picks the species
    "platform",   # 8  moving / falling platform, trampoline — traversal aid
]

# The vertical band the generator works in.
#
# Measured, not guessed: across level_1..3 and bonus_1, every tile outside rows
# 0-1 (the ceiling that Underground and Castle themes draw) and every entity
# lives in rows 12..22, and rows 2..8 are empty sky in all of them. A 14-row
# band starting at 9 covers all of it with headroom to spare, and 14 is exactly
# the height of the VGLC Super Mario corpus — so a VGLC level drops in with no
# rescaling.
BAND_TOP = 9
BAND_BOTTOM = 22          # inclusive
BAND_HEIGHT = BAND_BOTTOM - BAND_TOP + 1   # 14
LEVEL_HEIGHT = 23         # what the game expects; rows outside the band are
                          # regenerated deterministically at decode time.

# --- JSON tile/entity names -> vocabulary ----------------------------------

_TILE_TO_CLASS = {
    "ground": SOLID, "ice": SOLID, "conveyor": SOLID, "used": SOLID,
    "brick": BREAKABLE,
    "question": QUESTION,
    "pipe": PIPE,
    "water": HAZARD, "lava": HAZARD,
    "coin_tile": COIN,
}

# Entities that are *level geometry* — a generator has to place them, because
# they decide where the player can stand and what the level's shape means.
_ENTITY_TO_CLASS = {
    "question_block": QUESTION,
    "pipe": PIPE,
    "coin": COIN,
    "moving_platform": PLATFORM, "falling_platform": PLATFORM,
    "trampoline": PLATFORM,
    "goomba": ENEMY, "koopa_troopa": ENEMY, "koopa_paratroopa": ENEMY,
    "spiny": ENEMY, "piranha_plant": ENEMY, "hammer_bro": ENEMY,
    "boo": ENEMY, "bullet_bill": ENEMY, "thwomp": ENEMY,
    "chain_chomp": ENEMY, "lakitu": ENEMY,
}

# Entities deliberately NOT encoded. They are level *furniture* placed by rule
# at decode time, and asking a generator to learn them adds classes without
# adding structure:
#   flagpole, star_coin  — fixed count and role, placed by the decoder
#   bowser, boom_boom    — bosses; a boss room is authored, not sampled
#   pow_block, pswitch   — puzzle items tied to hand-designed intent
_ENTITY_IGNORED = {
    "flagpole", "star_coin", "bowser", "boom_boom", "pow_block", "pswitch",
    "mushroom", "fire_flower", "star", "one_up_mushroom", "cape_feather",
    "mega_mushroom", "mini_mushroom",
}

# --- vocabulary -> JSON, per theme -----------------------------------------

_THEME_SOLID = {
    "overworld": "ground", "underground": "ground",
    "castle": "ground", "ice": "ice",
}
_THEME_BREAKABLE = {
    "overworld": "brick", "underground": "brick",
    "castle": "brick", "ice": "ice",
}
_THEME_HAZARD = {
    "overworld": "water", "underground": "water",
    "castle": "lava", "ice": "water",
}
# Which species an ENEMY cell becomes. Cycled by index so a level gets a mix
# rather than 30 identical Goombas; the generator decided *where*, this decides
# *what*, and it is a one-line change to make it difficulty-aware.
_THEME_ENEMIES = {
    "overworld": ["goomba", "goomba", "koopa_troopa"],
    "underground": ["goomba", "koopa_troopa", "piranha_plant"],
    "castle": ["koopa_troopa", "hammer_bro", "spiny"],
    "ice": ["koopa_troopa", "spiny", "goomba"],
}
_THEMES_WITH_CEILING = {"underground", "castle"}


# --- encode ----------------------------------------------------------------

def blank_grid(width: int) -> list[list[int]]:
    return [[EMPTY] * width for _ in range(BAND_HEIGHT)]


def encode(level: dict) -> tuple[list[list[int]], dict]:
    """Level JSON -> (label grid, metadata).

    Anything above BAND_TOP is dropped, which by construction is ceiling and
    sky. The metadata carries what the band cannot: width, theme, and where the
    spawn and flagpole were.
    """
    width = int(level.get("width", 200))
    grid = blank_grid(width)

    def put(x: int, y: int, cls: int) -> None:
        row = y - BAND_TOP
        if 0 <= row < BAND_HEIGHT and 0 <= x < width:
            # Later writes win, and entities are applied after tiles: a
            # question block sitting on a brick run should read as a question
            # block, since that is what the player interacts with.
            grid[row][x] = cls

    for tile in level.get("tiles", []):
        cls = _TILE_TO_CLASS.get(tile.get("type", "empty"))
        if cls is None:
            continue
        x, y, w = int(tile.get("x", 0)), int(tile.get("y", 0)), int(tile.get("w", 1))
        for dx in range(w):
            put(x + dx, y, cls)

    for ent in level.get("entities", []):
        name = ent.get("type", "")
        if name in _ENTITY_IGNORED:
            continue
        cls = _ENTITY_TO_CLASS.get(name)
        if cls is None:
            print(f"[level_tensor] unmapped entity '{name}' — skipped",
                  file=sys.stderr)
            continue
        put(int(round(float(ent.get("x", 0)))), int(round(float(ent.get("y", 0)))), cls)

    meta = {
        "contractVersion": CONTRACT_VERSION,
        "width": width,
        "theme": level.get("theme", "overworld"),
        "name": level.get("name", "Generated Level"),
        "spawnPoint": level.get("spawnPoint", {"x": 3, "y": 19}),
        # Prefer the flagpole ENTITY: the meta key was written as a default
        # (width - 2) by every saveLevel to date and can sit tiles past the
        # real flag — the entity is what the game actually completes on.
        "flagpole": next(
            ({"x": int(e["x"]), "y": int(e["y"])}
             for e in level.get("entities", []) if e.get("type") == "flagpole"),
            level.get("flagpole", {"x": width - 2, "y": 18})),
    }
    return grid, meta


# --- decode ----------------------------------------------------------------

def decode(grid: list[list[int]], meta: dict | None = None) -> dict:
    """Label grid -> level JSON the game's LevelLoader can read.

    The decoder owns everything the band does not carry: the theme's ceiling,
    the tile species behind SOLID/BREAKABLE/HAZARD, the enemy species behind
    ENEMY, and the fixed furniture (spawn, flagpole, star coins).
    """
    meta = dict(meta or {})
    width = len(grid[0]) if grid else 0
    theme = meta.get("theme", "overworld")

    tiles: list[dict] = []
    entities: list[dict] = []
    enemy_cycle = _THEME_ENEMIES.get(theme, _THEME_ENEMIES["overworld"])
    enemy_n = 0

    if theme in _THEMES_WITH_CEILING:
        tiles.append({"type": _THEME_SOLID.get(theme, "ground"), "x": 0, "y": 0, "w": width})
        tiles.append({"type": _THEME_BREAKABLE.get(theme, "brick"), "x": 0, "y": 1, "w": width})

    # Tile classes are run-length encoded along each row, the same way
    # LevelLoader::saveLevel writes them, so a generated file looks like a
    # hand-saved one and diffs against one usefully.
    tile_names = {
        SOLID: _THEME_SOLID.get(theme, "ground"),
        BREAKABLE: _THEME_BREAKABLE.get(theme, "brick"),
        HAZARD: _THEME_HAZARD.get(theme, "water"),
        COIN: "coin_tile",
        PIPE: "pipe",
    }

    for row_i, row in enumerate(grid):
        y = BAND_TOP + row_i
        x = 0
        while x < width:
            cls = row[x]
            if cls in tile_names:
                run = 1
                while x + run < width and row[x + run] == cls:
                    run += 1
                entry = {"type": tile_names[cls], "x": x, "y": y}
                if run > 1:
                    entry["w"] = run
                tiles.append(entry)
                x += run
                continue
            if cls == QUESTION:
                entities.append({"type": "question_block", "x": x, "y": y, "itemType": 0})
            elif cls == PLATFORM:
                entities.append({"type": "moving_platform", "x": x, "y": y})
            elif cls == ENEMY:
                entities.append({"type": enemy_cycle[enemy_n % len(enemy_cycle)],
                                 "x": x, "y": y})
                enemy_n += 1
            x += 1

    spawn = meta.get("spawnPoint", {"x": 3, "y": 19})
    flag = meta.get("flagpole", {"x": max(width - 2, 0), "y": 18})
    entities.append({"type": "flagpole", "x": flag["x"], "y": flag["y"]})

    return {
        "name": meta.get("name", "Generated Level"),
        "theme": theme,
        "width": width,
        "height": LEVEL_HEIGHT,
        "tileSize": 32.0,
        "spawnPoint": spawn,
        "flagpole": flag,
        "tiles": tiles,
        "entities": entities,
        "generator": {"contractVersion": CONTRACT_VERSION},
    }


# --- derived views ---------------------------------------------------------

def one_hot(grid: list[list[int]]):
    """Label grid -> numpy array (BAND_HEIGHT, W, len(VOCAB)), float32."""
    import numpy as np
    a = np.zeros((len(grid), len(grid[0]), len(VOCAB)), dtype="float32")
    for r, row in enumerate(grid):
        for c, v in enumerate(row):
            a[r, c, v] = 1.0
    return a


def windows(grid: list[list[int]], width: int = 28, stride: int = 1):
    """Sliding windows along x. This is what turns ~30 corpus levels into
    thousands of training samples, and is the only reason a GAN is trainable
    on a corpus this small."""
    n = len(grid[0])
    for x0 in range(0, max(n - width + 1, 0), stride):
        yield [row[x0:x0 + width] for row in grid]


# --- text serialisation (dependency-free, diffable, eyeball-able) ----------

_GLYPHS = "-XSQPvo*="   # index == class id; see VOCAB


def grid_to_text(grid: list[list[int]], meta: dict) -> str:
    head = "# " + json.dumps(meta, sort_keys=True)
    body = "\n".join("".join(_GLYPHS[v] for v in row) for row in grid)
    return head + "\n" + body + "\n"


def text_to_grid(text: str) -> tuple[list[list[int]], dict]:
    meta: dict = {}
    rows: list[list[int]] = []
    for line in text.splitlines():
        if line.startswith("#"):
            meta = json.loads(line[1:].strip())
            continue
        if not line:
            continue
        rows.append([_GLYPHS.index(ch) for ch in line])
    return rows, meta


# --- commands --------------------------------------------------------------

def cmd_encode(args) -> int:
    level = json.loads(Path(args.level).read_text())
    grid, meta = encode(level)
    text = grid_to_text(grid, meta)
    if args.out:
        Path(args.out).write_text(text)
        print(f"[level_tensor] {args.level} -> {args.out} "
              f"({BAND_HEIGHT}x{meta['width']})")
    else:
        sys.stdout.write(text)
    return 0


def cmd_decode(args) -> int:
    grid, meta = text_to_grid(Path(args.grid).read_text())
    if args.theme:
        meta["theme"] = args.theme
    level = decode(grid, meta)
    Path(args.out).write_text(json.dumps(level, indent=2, sort_keys=True))
    print(f"[level_tensor] {args.grid} -> {args.out} "
          f"({len(level['tiles'])} tile runs, {len(level['entities'])} entities)")
    return 0


def cmd_check(args) -> int:
    """Roundtrip self-test: JSON -> grid -> JSON -> grid must be a fixed point.

    The first hop is lossy on purpose (species collapse to classes, ignored
    furniture is dropped), so equality is asserted on the *second* hop. A
    vocabulary change that breaks decode shows up here as a diff, not as a
    level that loads and plays wrong.
    """
    failures = 0
    for path in args.levels:
        level = json.loads(Path(path).read_text())
        g1, m1 = encode(level)
        g2, m2 = encode(decode(g1, m1))

        if g1 == g2:
            filled = sum(1 for row in g1 for v in row if v != EMPTY)
            print(f"  ok    {path}  ({BAND_HEIGHT}x{m1['width']}, "
                  f"{filled} non-empty cells)")
            continue

        failures += 1
        diffs = [(r, c, g1[r][c], g2[r][c])
                 for r in range(len(g1)) for c in range(len(g1[0]))
                 if g1[r][c] != g2[r][c]]
        print(f"  FAIL  {path}  {len(diffs)} cells differ after roundtrip")
        for r, c, a, b in diffs[:8]:
            print(f"          y={BAND_TOP + r} x={c}: "
                  f"{VOCAB[a]} -> {VOCAB[b]}")
    print(f"[level_tensor] {len(args.levels) - failures}/{len(args.levels)} "
          f"levels roundtrip cleanly")
    return 1 if failures else 0


def cmd_corpus(args) -> int:
    import numpy as np
    samples = []
    for path in args.levels:
        grid, _ = encode(json.loads(Path(path).read_text()))
        for w in windows(grid, args.window, args.stride):
            samples.append(one_hot(w))
    if not samples:
        print("[level_tensor] no samples produced", file=sys.stderr)
        return 1
    arr = np.stack(samples)
    np.savez_compressed(args.out, samples=arr,
                        contractVersion=CONTRACT_VERSION, vocab=np.array(VOCAB))
    print(f"[level_tensor] {len(samples)} windows {arr.shape[1:]} -> {args.out}")
    return 0


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    e = sub.add_parser("encode", help="level JSON -> label grid text")
    e.add_argument("level")
    e.add_argument("out", nargs="?")
    e.set_defaults(func=cmd_encode)

    d = sub.add_parser("decode", help="label grid text -> level JSON")
    d.add_argument("grid")
    d.add_argument("out")
    d.add_argument("--theme", default=None,
                   help="overworld | underground | castle | ice")
    d.set_defaults(func=cmd_decode)

    c = sub.add_parser("check", help="roundtrip self-test on real levels")
    c.add_argument("levels", nargs="+")
    c.set_defaults(func=cmd_check)

    k = sub.add_parser("corpus", help="sliding-window .npz for training")
    k.add_argument("out")
    k.add_argument("levels", nargs="+")
    k.add_argument("--window", type=int, default=28)
    k.add_argument("--stride", type=int, default=1)
    k.set_defaults(func=cmd_corpus)

    args = p.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
