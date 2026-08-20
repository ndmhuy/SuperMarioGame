#!/usr/bin/env python3
"""Derive Super / Fire / Cape player frames from the existing _small frames.

Why this exists
---------------
The player atlas ships art for two forms per character, `_small` and `_tiny`.
Super, Fire and Cape became reachable when the power-up chain was fixed (audit
B-2), and every one of them then rendered as small Mario, because
`Player::refreshStateAnimations()` had no prefix to ask for.

Drawing three new forms x four characters by hand is out of scope for this
project, so this is the documented palette-swap fallback the completion plan
allows.  It is a *derivation*, not original art:

  * Super  - the `_small` frame at 2x vertical scale (nearest-neighbour).  The
             ratio is not arbitrary: SmallState's hitbox is 24x30 and
             SuperState's is 24x60, so doubling the height keeps sprite and
             hitbox in the same proportion.
  * Fire   - the Super frame with the outfit recoloured white and the hat/accent
             recoloured red, matching the NES fire palette.
  * Cape   - the Super frame with the outfit recoloured cape-yellow.

Every character's art uses a 3-colour NES palette, which is why a flat colour
map is enough; the maps below were read off the shipped atlas, not guessed.

Usage
-----
    python3 tools/powerup-frames/gen_powerup_frames.py

Run from the repository root.  It rewrites
`SuperMarioGame/assets/spriteSheet/player/{player.png,player.json}` in place and
is idempotent: generated frames are recognised by name, discarded, and rebuilt
from the base frames on every run.
"""

import json
import os
import sys

try:
    from PIL import Image
except ImportError:  # pragma: no cover - developer tooling
    sys.exit("Pillow is required: pip install pillow")

ATLAS_DIR = os.path.join("SuperMarioGame", "assets", "spriteSheet", "player")
PNG_PATH = os.path.join(ATLAS_DIR, "player.png")
JSON_PATH = os.path.join(ATLAS_DIR, "player.json")

CHARACTERS = ("mario", "luigi", "toad", "peach")
GENERATED_FORMS = ("super", "fire", "cape")

# Vertical scale from the _small source to the Super/Fire/Cape frame.
SUPER_SCALE_Y = 2

FIRE_RED = (216, 40, 0)
FIRE_WHITE = (255, 255, 255)
CAPE_YELLOW = (252, 188, 0)

# Colour maps, keyed by character.  Read from the shipped atlas: each character
# is drawn with exactly three opaque colours (skin, outfit, hat/accent), so a
# flat source->target map recolours a whole form without touching the linework.
FIRE_PALETTE = {
    # blue overalls -> white, brown-red hat/shirt -> fire red
    "mario": {(0, 0, 179): FIRE_WHITE, (189, 68, 0): FIRE_RED},
    # blue overalls -> white, green hat/shirt -> fire red
    "luigi": {(0, 0, 179): FIRE_WHITE, (0, 224, 0): FIRE_RED},
    # blue vest -> fire red; the cap is already white
    "toad": {(0, 0, 179): FIRE_RED},
    # pink dress -> white, brown hair (two shades) -> fire red
    "peach": {(255, 103, 190): FIRE_WHITE, (96, 56, 0): FIRE_RED, (120, 39, 0): (150, 20, 0)},
}

# Cape recolours the hat/shirt accent rather than the outfit: a yellow cape over
# the normal costume reads as Cape Mario, whereas recolouring the overalls just
# turned the whole sprite into a gold figure.
CAPE_PALETTE = {
    "mario": {(189, 68, 0): CAPE_YELLOW},
    "luigi": {(0, 224, 0): CAPE_YELLOW},
    "toad": {(255, 255, 255): CAPE_YELLOW},
    "peach": {(96, 56, 0): CAPE_YELLOW, (120, 39, 0): (214, 150, 0)},
}

PADDING = 1


def is_generated(name):
    """True for a frame this script produced on a previous run."""
    return any("_%s_" % form in name or name.endswith("_" + form) for form in GENERATED_FORMS)


def recolour(image, palette):
    out = image.copy()
    pixels = out.load()
    width, height = out.size
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            replacement = palette.get((r, g, b))
            if replacement is not None:
                pixels[x, y] = (replacement[0], replacement[1], replacement[2], a)
    return out


def shelf_pack(images, max_width):
    """Lay images out in rows, returning positions and the canvas size."""
    positions = {}
    x = PADDING
    y = PADDING
    row_height = 0
    canvas_width = 0
    for name, img in images:
        w, h = img.size
        if x + w + PADDING > max_width and x > PADDING:
            x = PADDING
            y += row_height + PADDING
            row_height = 0
        positions[name] = (x, y)
        x += w + PADDING
        row_height = max(row_height, h)
        canvas_width = max(canvas_width, x)
    return positions, (max(canvas_width, max_width), y + row_height + PADDING)


def main():
    if not os.path.exists(PNG_PATH):
        sys.exit("Atlas not found at %s - run this from the repository root." % PNG_PATH)

    atlas = Image.open(PNG_PATH).convert("RGBA")
    data = json.load(open(JSON_PATH))
    frames = data["frames"]

    # 1. Collect the base (hand-drawn) frames, dropping anything a previous run
    #    of this script added so the output does not depend on the input.
    base = []
    for name in sorted(frames):
        if is_generated(name):
            continue
        rect = frames[name]["frame"]
        crop = atlas.crop((rect["x"], rect["y"],
                           rect["x"] + rect["w"], rect["y"] + rect["h"]))
        base.append((name, crop))

    base_by_name = dict(base)

    # 2. Derive the three missing forms from each _small frame.
    generated = []
    for character in CHARACTERS:
        prefix = character + "_small"
        for name, image in base:
            if not name.startswith(prefix + "_") and name != prefix:
                continue
            suffix = name[len(prefix):]          # "_idle", "_walk_0", ...
            w, h = image.size

            super_img = image.resize((w, h * SUPER_SCALE_Y), Image.NEAREST)
            generated.append((character + "_super" + suffix, super_img))
            generated.append((character + "_fire" + suffix,
                              recolour(super_img, FIRE_PALETTE[character])))
            generated.append((character + "_cape" + suffix,
                              recolour(super_img, CAPE_PALETTE[character])))

    if not generated:
        sys.exit("No _small frames found - nothing to derive.")

    all_frames = base + sorted(generated, key=lambda pair: pair[0])
    max_width = max(256, max(img.size[0] for _, img in all_frames) + 2 * PADDING)
    # A wider sheet keeps the canvas roughly square rather than a long ribbon.
    max_width = max(max_width, 384)

    positions, canvas_size = shelf_pack(all_frames, max_width)

    out_image = Image.new("RGBA", canvas_size, (0, 0, 0, 0))
    out_frames = {}
    for name, img in all_frames:
        x, y = positions[name]
        out_image.paste(img, (x, y))
        w, h = img.size
        out_frames[name] = {
            "frame": {"x": x, "y": y, "w": w, "h": h},
            "rotated": False,
            "trimmed": False,
            "spriteSourceSize": {"x": 0, "y": 0, "w": w, "h": h},
            "sourceSize": {"w": w, "h": h},
        }

    data["frames"] = out_frames
    data.setdefault("meta", {})
    data["meta"]["size"] = {"w": canvas_size[0], "h": canvas_size[1]}
    data["meta"]["generatedBy"] = "tools/powerup-frames/gen_powerup_frames.py"

    out_image.save(PNG_PATH)
    with open(JSON_PATH, "w") as handle:
        json.dump(data, handle, indent=2)

    print("Wrote %d frames (%d base + %d derived) into a %dx%d atlas."
          % (len(out_frames), len(base), len(generated), canvas_size[0], canvas_size[1]))
    unused = [n for n in base_by_name if n not in out_frames]
    if unused:
        print("WARNING: dropped base frames:", unused)


if __name__ == "__main__":
    main()
