#!/usr/bin/env python3
"""Compose the three YouTube thumbnail candidates from fresh in-game captures.

Source frames live in saves/shots/ (gitignored, not committed); this script
and its PNG outputs are the committed deliverable. Run with:

    python3 reports/assets/thumbnail/compose.py
"""
import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
SHOTS = os.path.abspath(os.path.join(HERE, "..", "..", "..", "saves", "shots"))
OUT = HERE

W, H = 1280, 720
MARGIN = 40

HELVETICA = "/System/Library/Fonts/HelveticaNeue.ttc"
HELVETICA_BOLD_INDEX = 1
HELVETICA_COND_BLACK_INDEX = 9


def font(size, index=HELVETICA_COND_BLACK_INDEX):
    return ImageFont.truetype(HELVETICA, size, index=index)


def load(name):
    im = Image.open(os.path.join(SHOTS, name)).convert("RGB")
    assert im.size == (W, H), f"{name} is {im.size}, expected {(W, H)}"
    return im


def cover_crop(im, target_w, target_h, focus_y=0.5):
    """Resize+crop im to exactly (target_w, target_h) without distortion,
    preserving aspect ratio (crop instead of stretch). focus_y biases which
    part of the vertical extent survives the crop (0 = top, 1 = bottom)."""
    src_w, src_h = im.size
    target_aspect = target_w / target_h
    src_aspect = src_w / src_h
    if src_aspect > target_aspect:
        # Source is wider than target: crop left/right, keep full height.
        new_w = int(round(src_h * target_aspect))
        x0 = (src_w - new_w) // 2
        box = (x0, 0, x0 + new_w, src_h)
    else:
        # Source is taller than target: crop top/bottom.
        new_h = int(round(src_w / target_aspect))
        y0 = int(round((src_h - new_h) * focus_y))
        y0 = max(0, min(y0, src_h - new_h))
        box = (0, y0, src_w, y0 + new_h)
    cropped = im.crop(box)
    return cropped.resize((target_w, target_h), Image.LANCZOS)


def draw_outlined_text(draw, xy, text, fnt, fill, stroke_fill, stroke_width, anchor=None):
    draw.text(xy, text, font=fnt, fill=fill, stroke_width=stroke_width,
              stroke_fill=stroke_fill, anchor=anchor)


def backing_bar(im, box, opacity=175, radius=14):
    """Paints a rounded dark backing bar behind text so it survives any
    background, without a hard rectangular edge."""
    overlay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(overlay)
    d.rounded_rectangle(box, radius=radius, fill=(10, 10, 20, opacity))
    im.paste(Image.alpha_composite(im.convert("RGBA"), overlay).convert("RGB"), (0, 0))


# ---------------------------------------------------------------------------
# Candidate A -- clean hero shot: busy World 1-1, Mario mid-jump.
# ---------------------------------------------------------------------------

def make_a():
    base = load("thumb_busy_02_jumping.png").convert("RGB")

    # Mario himself is airborne around x=627-660, y=540-583 (measured from
    # the raw capture) -- this box stays left of x=620 so he is never
    # covered, and the enemy cluster (paratroopa ~x1058, goomba ~x1150) sits
    # well clear to the right. Only the decorative coin pillar and empty sky
    # are underneath it.
    box = (MARGIN, 410, 620, 690)
    backing_bar(base, box, opacity=190)

    draw = ImageDraw.Draw(base)
    cs202_font = font(100, HELVETICA_COND_BLACK_INDEX)
    mario_font = font(70, HELVETICA_COND_BLACK_INDEX)
    sub_font = font(30, HELVETICA_BOLD_INDEX)

    draw_outlined_text(draw, (64, 424), "CS202", cs202_font,
                        fill=(255, 221, 51), stroke_fill=(20, 10, 0), stroke_width=9)
    draw_outlined_text(draw, (64, 520), "SUPER MARIO", mario_font,
                        fill=(255, 255, 255), stroke_fill=(20, 10, 0), stroke_width=7)
    draw_outlined_text(draw, (64, 612), "Programming Systems — Group 52 · APCS K25",
                        sub_font, fill=(210, 230, 255), stroke_fill=(20, 10, 0), stroke_width=5)

    base.save(os.path.join(OUT, "thumbnail_A_clean.png"))


# ---------------------------------------------------------------------------
# Candidate B -- 2x2 collage: gameplay, boss, lighting, editor.
# ---------------------------------------------------------------------------

def make_b():
    canvas = Image.new("RGB", (W, H), (5, 5, 10))

    bar_h = 128
    grid_top = bar_h
    grid_h = H - bar_h
    cell_w, cell_h = W // 2, grid_h // 2

    panels = [
        ("thumb_busy_01_arrival.png", 1.0),
        ("thumb_boss_01_t0.png", 1.0),
        ("thumb_cave_02_fire_mario_airborne.png", 1.0),
        ("thumb_editor_01_level_under_edit.png", 0.0),
    ]
    positions = [(0, grid_top), (cell_w, grid_top), (0, grid_top + cell_h), (cell_w, grid_top + cell_h)]

    for (name, focus_y), (px, py) in zip(panels, positions):
        im = load(name)
        panel = cover_crop(im, cell_w, cell_h, focus_y=focus_y)
        canvas.paste(panel, (px, py))

    # Thin separators so the four frames don't visually bleed into each other.
    draw = ImageDraw.Draw(canvas)
    sep = 4
    draw.rectangle((cell_w - sep // 2, grid_top, cell_w + sep // 2, H), fill=(5, 5, 10))
    draw.rectangle((0, grid_top + cell_h - sep // 2, W, grid_top + cell_h + sep // 2), fill=(5, 5, 10))

    # Title bar across the top, full width, solid -- never clipped by a grid line.
    draw.rectangle((0, 0, W, bar_h), fill=(10, 10, 20))
    main_font = font(58, HELVETICA_COND_BLACK_INDEX)
    sub_font = font(26, HELVETICA_BOLD_INDEX)
    draw_outlined_text(draw, (MARGIN, 16), "CS202 · SUPER MARIO GAME", main_font,
                        fill=(255, 221, 51), stroke_fill=(0, 0, 0), stroke_width=5)
    draw_outlined_text(draw, (MARGIN, 84), "Programming Systems — Group 52 · APCS K25",
                        sub_font, fill=(230, 230, 230), stroke_fill=(0, 0, 0), stroke_width=3)

    canvas.save(os.path.join(OUT, "thumbnail_B_collage.png"))


# ---------------------------------------------------------------------------
# Candidate C -- technical: dynamic-lighting cave + credibility line.
# ---------------------------------------------------------------------------

def make_c():
    base = load("thumb_cave_00_dark_with_light.png").convert("RGB")

    # The top third of this frame is near-pure black in-engine; a light
    # backing bar still goes behind the text so it also survives on a
    # brighter recapture of the same scene.
    box = (MARGIN, 120, 1170, 400)
    backing_bar(base, box, opacity=140)

    draw = ImageDraw.Draw(base)
    main_font = font(92, HELVETICA_COND_BLACK_INDEX)
    sub_font = font(30, HELVETICA_BOLD_INDEX)
    credibility_font = font(34, HELVETICA_BOLD_INDEX)

    draw_outlined_text(draw, (64, 140), "CS202 · SUPER MARIO", main_font,
                        fill=(255, 221, 51), stroke_fill=(0, 0, 0), stroke_width=8)
    draw_outlined_text(draw, (64, 250), "Programming Systems — Group 52 · APCS K25",
                        sub_font, fill=(230, 230, 230), stroke_fill=(0, 0, 0), stroke_width=4)
    draw_outlined_text(draw, (64, 320), "C++17 · SFML 3 · 110 features · 10+ design patterns",
                        credibility_font, fill=(140, 220, 255), stroke_fill=(0, 0, 0), stroke_width=4)

    base.save(os.path.join(OUT, "thumbnail_C_technical.png"))


def make_downscales():
    for name in ["thumbnail_A_clean.png", "thumbnail_B_collage.png", "thumbnail_C_technical.png"]:
        im = Image.open(os.path.join(OUT, name))
        small = im.resize((320, 180), Image.LANCZOS)
        small.save(os.path.join(OUT, name.replace(".png", "_320x180.png")))


if __name__ == "__main__":
    make_a()
    make_b()
    make_c()
    make_downscales()
    print("done")
