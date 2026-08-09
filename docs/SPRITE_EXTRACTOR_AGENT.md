# Sprite Extractor & Reader Agent Specification

> **Role**: Automated Sprite Sheet Reader, Frame Data Extractor, and Metadata Generator.

---

## Responsibilities
1. **Reference Image Parsing**: Parse input reference sprite sheet images, analyze grid layouts, and isolate character section regions.
2. **Background Color Keying**: Identify solid background backdrop colors (e.g. SNES blue/purple `#404070` or transparent keys) and replace them with alpha transparency `(0, 0, 0, 0)`.
3. **Bounding Box Calculation**: Detect non-background pixel clusters to calculate bounding boxes `{"x": x, "y": y, "w": w, "h": h}` for each animation frame.
4. **Target Sprite Sheet Generation**: Re-compose extracted frames onto clean, standardized game sprite atlas PNGs (`players.png`, `enemies.png`, `items.png`, `tileset_blocks.png`, `particles.png`).
5. **JSON Metadata Output**: Produce standard TexturePacker schema JSON metadata files matching the project's `test.json` format.

---

## Standard JSON Schema
```json
{
  "frames": {
    "frame_name": {
      "frame": {"x": 0, "y": 0, "w": 32, "h": 32},
      "rotated": false,
      "trimmed": false,
      "spriteSourceSize": {"x": 0, "y": 0, "w": 32, "h": 32},
      "sourceSize": {"w": 32, "h": 32}
    }
  },
  "meta": {
    "app": "Sprite Extractor Agent",
    "version": "1.0",
    "image": "target.png",
    "format": "RGBA8888",
    "size": {"w": 1024, "h": 1024},
    "scale": "1"
  }
}
```
