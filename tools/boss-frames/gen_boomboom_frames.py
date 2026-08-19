#!/usr/bin/env python3
"""Derive Boom Boom's frames from the shipped Boomerang Bro art.

Why this exists
---------------
Task 9.2 needs a mid-boss for Level 2. The enemy atlas has art for seventeen
enemies and Boom Boom is not one of them — the closest thing in it is Boomerang
Bro, which is the same silhouette family (a shelled bro) and, usefully, already
has a wind-up/throw pose to stand in for the spin attack.

So this is the same documented palette-swap fallback the power-up frames use.
It is a *derivation*, not original art:

  * scaled 1.75x, because Boom Boom is a boss and has to read as bigger than the
    Hammer Bros he shares a silhouette with;
  * recoloured to Boom Boom's palette — orange-tan skin, grey-blue shell —
    leaving the black linework alone.

Boomerang Bro is drawn with four opaque colours, which is why a flat colour map
is enough. They were read off the shipped atlas, not guessed.

Frames produced (from move_0, move_1, throw_0, throw_1):
    boom_boom_walk_0, boom_boom_walk_1, boom_boom_spin_0, boom_boom_spin_1

Usage
-----
    python3 tools/boss-frames/gen_boomboom_frames.py

Run from the repository root. Rewrites
`SuperMarioGame/assets/spriteSheet/enemy_projectile/{...png,...json}` in place.

Unlike the player tool this one is *append-only*: every existing frame keeps its
coordinates and only the new rows are added below. Repacking a seventeen-enemy
atlas to add one boss would put every other enemy at risk for no gain. It is
still idempotent — generated frames are recognised by name, dropped, and rebuilt
from the source frames on every run.
"""

import json
import os
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover - developer tooling
    sys.exit("Pillow is required: pip install pillow")

ATLAS_DIR = os.path.join("SuperMarioGame", "assets", "spriteSheet", "enemy_projectile")
PNG_PATH = os.path.join(ATLAS_DIR, "enemy_projectile.png")
JSON_PATH = os.path.join(ATLAS_DIR, "enemy_projectile.json")

GENERATED_PREFIX = "boom_boom_"
SCALE = 1.75
PADDING = 1

# source frame -> generated frame
SOURCES = {
    "boomerang_bro_move_0": "boom_boom_walk_0",
    "boomerang_bro_move_1": "boom_boom_walk_1",
    "boomerang_bro_throw_0": "boom_boom_spin_0",
    "boomerang_bro_throw_1": "boom_boom_spin_1",
}

# Boomerang Bro's four opaque colours -> Boom Boom's.
# Black linework is deliberately absent from this map: recolouring outlines
# turns a sprite to mush at this size.
PALETTE = {
    (88, 216, 84): (248, 152, 56),    # green skin  -> orange-tan
    (255, 255, 255): (168, 176, 200),  # white shell -> grey-blue
    (248, 56, 0): (216, 40, 0),        # red accent  -> deeper red
}


def recolour(image):
    out = image.copy()
    pixels = out.load()
    width, height = out.size
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            replacement = PALETTE.get((r, g, b))
            if replacement is not None:
                pixels[x, y] = (replacement[0], replacement[1], replacement[2], a)
    return out


def main():
    if not os.path.exists(PNG_PATH):
        sys.exit("Atlas not found at %s - run this from the repository root." % PNG_PATH)

    atlas = Image.open(PNG_PATH).convert("RGBA")
    data = json.load(open(JSON_PATH))
    frames = data["frames"]

    # 1. Drop whatever a previous run added, so the output does not depend on
    #    the input, and find where the hand-drawn art ends.
    base = {name: rect for name, rect in frames.items()
            if not name.startswith(GENERATED_PREFIX)}
    if not base:
        sys.exit("No base frames found.")

    base_bottom = max(r["frame"]["y"] + r["frame"]["h"] for r in base.values())
    atlas_width = atlas.size[0]

    # 2. Derive the frames.
    derived = []
    for source_name, target_name in sorted(SOURCES.items(), key=lambda kv: kv[1]):
        if source_name not in base:
            sys.exit("Source frame %s is missing from the atlas." % source_name)
        rect = base[source_name]["frame"]
        crop = atlas.crop((rect["x"], rect["y"],
                           rect["x"] + rect["w"], rect["y"] + rect["h"]))
        scaled = crop.resize((max(1, round(rect["w"] * SCALE)),
                              max(1, round(rect["h"] * SCALE))), Image.NEAREST)
        derived.append((target_name, recolour(scaled)))

    # 3. Lay the new frames out in a shelf below the existing art.
    x = PADDING
    y = base_bottom + PADDING
    row_height = 0
    positions = {}
    for name, image in derived:
        w, h = image.size
        if x + w + PADDING > atlas_width and x > PADDING:
            x = PADDING
            y += row_height + PADDING
            row_height = 0
        positions[name] = (x, y)
        x += w + PADDING
        row_height = max(row_height, h)

    new_height = y + row_height + PADDING

    # 4. Rebuild: existing art untouched at its own coordinates, new rows below.
    out_image = Image.new("RGBA", (atlas_width, new_height), (0, 0, 0, 0))
    out_image.paste(atlas.crop((0, 0, atlas_width, min(base_bottom, atlas.size[1]))), (0, 0))

    out_frames = dict(base)
    for name, image in derived:
        px, py = positions[name]
        out_image.paste(image, (px, py))
        w, h = image.size
        out_frames[name] = {
            "frame": {"x": px, "y": py, "w": w, "h": h},
            "rotated": False,
            "trimmed": False,
            "spriteSourceSize": {"x": 0, "y": 0, "w": w, "h": h},
            "sourceSize": {"w": w, "h": h},
        }

    data["frames"] = out_frames
    data.setdefault("meta", {})
    data["meta"]["size"] = {"w": atlas_width, "h": new_height}
    data["meta"]["derivedFrames"] = "tools/boss-frames/gen_boomboom_frames.py"

    out_image.save(PNG_PATH)
    with open(JSON_PATH, "w") as handle:
        json.dump(data, handle, indent=2)

    print("Added %d Boom Boom frames below %d existing ones; atlas is now %dx%d."
          % (len(derived), len(base), atlas_width, new_height))


if __name__ == "__main__":
    main()
