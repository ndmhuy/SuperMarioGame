#!/usr/bin/env python3
"""Assembles reports/assets/11_main_features.png -- a single contact sheet
for the Features-demonstration section (Phase 3A, scope cut to "main
features only" per the 2026-09-02 mid-session direction: core screens plus a
handful of headline gameplay cells, no per-family sheets).

Source frames are raw 1280x720 --script captures in saves/shots/ (gitignored,
not committed) from:
  tests/scripts/report_core_screens.txt      (menu / character select /
                                               world map / playing / pause)
  tests/scripts/report_showcase_walk.txt     (power-up forms, a few enemies --
                                               walked through a custom level,
                                               assets/levels/custom/report_showcase.json,
                                               authored directly as JSON because
                                               scripted events never reach
                                               ImGui, so neither the editor's
                                               palette nor console `spawn` is
                                               scriptable)

Every cell is a real in-game frame (no harness renders). Cropped to a fixed
16:9-safe center crop, downscaled to a uniform thumbnail, labelled below in
the game's own PressStart2P font.
"""
import pathlib
from PIL import Image, ImageDraw, ImageFont

# Git root is the PARENT of SuperMarioGame/ (this repo nests the app code one
# level down -- reports/ and docs/ are siblings of SuperMarioGame/, not
# children of it; see AGENTS.md's project-structure section).
GAME_DIR = pathlib.Path("/Users/huynguyen/Documents/CS202-Cpp/smg-lanes/shots/SuperMarioGame")
REPO_ROOT = GAME_DIR.parent
SHOTS = GAME_DIR / "saves" / "shots"
OUT = REPO_ROOT / "reports" / "assets" / "11_main_features.png"
FONT_PATH = GAME_DIR / "assets" / "font" / "PressStart2P.ttf"

THUMB_W, THUMB_H = 320, 180
LABEL_H = 30
PAD = 10
COLS = 5

SECTIONS = [
    ("CORE SCREENS", [
        ("core_menu.png", "Main Menu"),
        ("core_character_select.png", "Character Select"),
        ("core_world_map.png", "World Map"),
        ("core_playing.png", "Playing (1-1)"),
        ("core_pause.png", "Pause"),
    ]),
    ("POWER-UP FORMS (one continuous walk)", [
        ("showcase_power_mushroom.png", "Super (mushroom)"),
        ("showcase_power_cape_feather.png", "Cape (feather)"),
        ("showcase_power_fire_flower.png", "Fire (flower)"),
        ("showcase_power_mini_mushroom.png", "Mini"),
        ("showcase_power_star.png", "Star"),
        ("showcase_power_mega_mushroom.png", "Mega"),
    ]),
    ("ENEMIES (same walk)", [
        ("showcase_enemy_koopa_troopa.png", "Koopa Troopa"),
        ("showcase_enemy_thwomp.png", "Thwomp"),
        ("showcase_enemy_piranha_plant.png", "Piranha Plant"),
    ]),
]

font = ImageFont.truetype(str(FONT_PATH), 11)
title_font = ImageFont.truetype(str(FONT_PATH), 14)


def load_thumb(name):
    p = SHOTS / name
    im = Image.open(p).convert("RGB")
    # Source is already 1280x720 (16:9) -- no cropping needed, just downscale.
    im = im.resize((THUMB_W, THUMB_H), Image.LANCZOS)
    return im


def render_section(title, items):
    rows = (len(items) + COLS - 1) // COLS
    title_h = 26
    w = COLS * THUMB_W + (COLS + 1) * PAD
    h = title_h + rows * (THUMB_H + LABEL_H + PAD) + PAD
    canvas = Image.new("RGB", (w, h), (18, 18, 28))
    draw = ImageDraw.Draw(canvas)
    draw.text((PAD, 4), title, font=title_font, fill=(255, 210, 60))
    for i, (fname, label) in enumerate(items):
        col = i % COLS
        row = i // COLS
        x = PAD + col * (THUMB_W + PAD)
        y = title_h + PAD + row * (THUMB_H + LABEL_H + PAD)
        thumb = load_thumb(fname)
        canvas.paste(thumb, (x, y))
        draw.rectangle([x, y, x + THUMB_W - 1, y + THUMB_H - 1], outline=(90, 90, 110), width=1)
        # Label, centered, word-wrapped to the cell width if needed.
        tw = draw.textlength(label, font=font)
        tx = x + max(0, (THUMB_W - tw) // 2)
        draw.text((tx, y + THUMB_H + 6), label, font=font, fill=(230, 230, 230))
    return canvas


def main():
    sections = [render_section(title, items) for title, items in SECTIONS]
    width = max(s.width for s in sections)
    gap = 16
    total_h = sum(s.height for s in sections) + gap * (len(sections) - 1) + 2 * PAD
    sheet = Image.new("RGB", (width, total_h), (10, 10, 16))
    y = PAD
    for s in sections:
        sheet.paste(s, (0, y))
        y += s.height + gap
    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(OUT)
    print("wrote", OUT, sheet.size)


if __name__ == "__main__":
    main()
