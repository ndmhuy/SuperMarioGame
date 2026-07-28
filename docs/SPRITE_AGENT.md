# Sprite Creating Agent (`sprite_artist`) — Documentation & Guidelines

> **Subagent Name**: `sprite_artist`  
> **Role**: Pixel Art & Sprite Sheet Creation Specialist  
> **Target Framework**: C++17 / SFML 3.0.2  
> **Style Target**: 16-bit SNES Super Mario World Aesthetic  

---

## 1. Overview & Purpose

The **Sprite Creating Agent** (`sprite_artist`) is an automated subagent specialized in:
- Generating and sourcing 2D pixel art assets for players, enemies, items, blocks, tilesets, and HUD icons.
- Formatting raw images into standardized grid sprite sheets (PNG with RGBA transparency).
- Generating animation configuration metadata (`entities.json`) used by the C++ engine (`SpriteSheetAnimator`, `EntityFactory`, `ResourceManager`).
- Processing images using Python tools (Pillow/OpenCV) for background removal, palette adjusting, and sprite packing.

---

## 2. Dimensional & Aesthetic Standards

As specified in `SPEC.md` and `sprites_list.md`:

| Asset Category | Base Grid Dimensions | Notes & Animation States |
| :--- | :--- | :--- |
| **Small Mario / Players** | 32 × 32 px | States: `idle`, `walk` (3), `run` (3), `jump`, `fall`, `crouch`, `slide`, `skid`, `damaged`, `death` |
| **Super / Fire / Cape Players** | 32 × 64 px | States: Same as Small + `shoot` (Fire), `glide`/`swoop`/`spin` (Cape) |
| **Mini Power-up Form** | 16 × 16 px | Half-size collision box & sprite |
| **Mega Power-up Form** | 128 × 128 px | 4-tile giant form |
| **Enemies** | 32 × 32 px or 32 × 64 px | Goomba (32x32), Koopa (32x48/64), Bowser (64x64/128x128), Thwomp (32x48) |
| **Items & Power-ups** | 32 × 32 px | Super Mushroom, Fire Flower, Coin, Star, 1-UP, Feather, POW Block, P-Switch |
| **Blocks & Tilesets** | 32 × 32 px per tile | Brick, Question, Pipes, Flagpole, Moving/Falling Platforms, Ice, Conveyor |
| **HUD & UI** | Variable | Mini heads, coin counter icon, boss health bar segments |

---

## 3. Directory Layout

All assets generated or configured by `sprite_artist` are organized under `SuperMarioGame/assets/`:

```text
SuperMarioGame/assets/
├── spriteSheet/
│   ├── player/
│   │   ├── mario_small.png
│   │   ├── mario_super.png
│   │   ├── luigi_small.png
│   │   └── ...
│   ├── enemies/
│   │   ├── goomba.png
│   │   ├── koopa.png
│   │   └── ...
│   ├── items/
│   │   ├── powerups.png
│   │   └── coins.png
│   └── tilesets/
│       ├── grass_level.png
│       ├── cave_level.png
│       └── castle_level.png
└── config/
    └── entities.json
```

---

## 4. Standard Sprite Metadata Format (`test.json` Standard)

All generated sprite sheet JSON metadata files MUST adhere to the standard `test.json` (TexturePacker Hash JSON) format:

```json
{
  "frames": {
    "frame_name": {
      "frame": { "x": 0, "y": 0, "w": 32, "h": 32 },
      "rotated": false,
      "trimmed": false,
      "spriteSourceSize": { "x": 0, "y": 0, "w": 32, "h": 32 },
      "sourceSize": { "w": 32, "h": 32 }
    }
  },
  "meta": {
    "app": "Sprite Sheet Maker",
    "version": "1.0",
    "image": "sheet.png",
    "format": "RGBA8888",
    "size": { "w": 256, "h": 192 },
    "scale": "1"
  }
}
```

---

## 5. Subagent System Prompt Reference

When invoking `sprite_artist` via `invoke_subagent`, the system prompt configured is:

```text
You are the Sprite Creating Agent (sprite_artist) for the 16-bit SNES-style Super Mario Game project written in C++17 with SFML 3.0.2.

### YOUR RESPONSIBILITIES:
1. Sprite & Animation Generation (16-bit SNES Super Mario style).
2. Asset Processing & Slicing (Python Pillow/OpenCV grid slicing & transparency).
3. Metadata & JSON Config Creation: ALL sprite sheet JSON files MUST adhere strictly to the standard test.json (TexturePacker Hash) format with top-level "frames" and "meta" objects.
4. SFML Compatibility & IntRect alignment.
```

---

## 6. Slicing Helper Script Example (Python)

```python
from PIL import Image

def slice_sprite_sheet(sheet_path, frame_w, frame_h, output_prefix):
    img = Image.open(sheet_path).convert("RGBA")
    w, h = img.size
    cols = w // frame_w
    rows = h // frame_h
    
    idx = 0
    for r in range(rows):
        for c in range(cols):
            box = (c * frame_w, r * frame_h, (c + 1) * frame_w, (r + 1) * frame_h)
            frame = img.crop(box)
            frame.save(f"{output_prefix}_{idx}.png")
            idx += 1
```
