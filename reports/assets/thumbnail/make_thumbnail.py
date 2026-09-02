#!/usr/bin/env python3
"""Compose the YouTube thumbnail for the CS202 SuperMarioGame demo video.

Scope (per the coordinator, after two rounds of review): ship ONE thumbnail,
the "technical" candidate -- the dynamic-lighting cave, cropped and zoomed on
Mario so he actually reads at small size, with a credibility line that only
states verified facts.

Source frame lives in saves/shots/ (gitignored, not committed); this script
and its PNG output are the committed deliverable. Run with:

    python3 reports/assets/thumbnail/make_thumbnail.py
"""
import os
from PIL import Image, ImageDraw, ImageFont, ImageEnhance

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


def draw_outlined_text(draw, xy, text, fnt, fill, stroke_fill, stroke_width):
    draw.text(xy, text, font=fnt, fill=fill, stroke_width=stroke_width, stroke_fill=stroke_fill)


def backing_bar(im, box, opacity=175, radius=14):
    """Paints a rounded dark backing bar behind text so it survives any
    background, without a hard rectangular edge."""
    overlay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(overlay)
    d.rounded_rectangle(box, radius=radius, fill=(10, 10, 20, opacity))
    im.paste(Image.alpha_composite(im.convert("RGBA"), overlay).convert("RGB"), (0, 0))


def make_c():
    src = load("thumb_cave_00_dark_with_light.png")

    # Mario (small form) plus the fire flower sit at roughly x:95-220,
    # y:605-660 in the raw 1280x720 capture -- a ~30px-tall speck in the
    # bottom-left corner, per the coordinator's review. Crop tight around
    # that pair (keeping the dark cave ceiling above them, where the title
    # goes) and upscale with NEAREST -- this is pixel art, so NEAREST keeps
    # every edge crisp; LANCZOS/BICUBIC would blur it into mush.
    crop_box = (0, 520, 355, 720)  # 355x200, exactly 16:9
    cropped = src.crop(crop_box)
    game_layer = cropped.resize((W, H), Image.NEAREST)

    # The cave is very dark by design (that is the point of the lighting
    # demo), which reads as "underlit" rather than "atmospheric" once
    # cropped this tight. Lift it moderately so Mario and the flower read
    # clearly; the text layer below is untouched by this.
    game_layer = ImageEnhance.Brightness(game_layer).enhance(1.4)

    base = game_layer.convert("RGB")

    # Same title block, same position, as the earlier reviewed-and-approved
    # candidate C -- only the credibility line's content changed (below).
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
    # "110 features" removed: that is SPEC's scope figure, not a verified
    # delivered count (submission_documents/features_list.md lists 97, and a
    # parallel lane is still reconciling the true number). Only claims that
    # hold up go on a public thumbnail.
    draw_outlined_text(draw, (64, 320), "C++17 · SFML 3 · 10+ design patterns",
                        credibility_font, fill=(140, 220, 255), stroke_fill=(0, 0, 0), stroke_width=4)

    base.save(os.path.join(OUT, "thumbnail_C_technical.png"))
    small = base.resize((320, 180), Image.LANCZOS)
    small.save(os.path.join(OUT, "thumbnail_C_technical_320x180.png"))


if __name__ == "__main__":
    make_c()
    print("done")
