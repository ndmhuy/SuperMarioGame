import os
import struct
import zlib
import json

def create_particle_texture(output_path):
    width = 64
    height = 64

    pixels = bytearray(width * height * 4)

    def set_pixel(x, y, r, g, b, a):
        if 0 <= x < width and 0 <= y < height:
            idx = (y * width + x) * 4
            pixels[idx] = r
            pixels[idx + 1] = g
            pixels[idx + 2] = b
            pixels[idx + 3] = a

    # 1. Tile (0..15, 0..15): Crisp White Circle / Dot (Stomp, WallDust)
    for y in range(16):
        for x in range(16):
            dx = x - 7.5
            dy = y - 7.5
            dist_sq = dx * dx + dy * dy
            if dist_sq <= 36.0:  # radius 6
                if dist_sq <= 25.0:
                    set_pixel(x, y, 255, 255, 255, 255)
                else:
                    alpha = int(255 * (1.0 - (dist_sq - 25.0) / 11.0))
                    set_pixel(x, y, 255, 255, 255, alpha)

    # 2. Tile (16..31, 0..15): 4-Point Star / Sparkle (CoinSparkle, Combo)
    for y in range(16):
        for x in range(16, 32):
            rx = x - 16
            dx = abs(rx - 7.5)
            dy = abs(y - 7.5)
            if (dx <= 1.5 and dy <= 7.0) or (dy <= 1.5 and dx <= 7.0) or (dx <= 3.5 and dy <= 3.5):
                alpha = 255
                if dx > 5.5 or dy > 5.5:
                    alpha = 200
                set_pixel(x, y, 255, 255, 255, alpha)

    # 3. Tile (32..47, 0..15): Brick Fragment (BrickBreak)
    for y in range(16):
        for x in range(32, 48):
            rx = x - 32
            if 2 <= rx <= 13 and 2 <= y <= 13:
                r, g, b = 210, 100, 40
                if rx <= 3 or y <= 3:  # Bevel highlight
                    r, g, b = 255, 170, 100
                elif rx >= 12 or y >= 12:  # Bevel shadow
                    r, g, b = 120, 40, 10
                set_pixel(x, y, r, g, b, 255)

    # 4. Tile (48..63, 0..15): Water Bubble Ring (WaterBubble)
    for y in range(16):
        for x in range(48, 64):
            rx = x - 48
            dx = rx - 7.5
            dy = y - 7.5
            dist_sq = dx * dx + dy * dy
            if 9.0 <= dist_sq <= 36.0:  # ring thickness
                if dx < 0 and dy < 0:
                    set_pixel(x, y, 255, 255, 255, 240)  # Top-left white highlight
                else:
                    set_pixel(x, y, 160, 220, 255, 220)  # Light cyan ring

    # 5. Tile (0..15, 16..31): Smoke Cloud (DeathPoof)
    for y in range(16, 32):
        for x in range(16):
            ry = y - 16
            c1 = (x - 5)**2 + (ry - 8)**2 <= 16
            c2 = (x - 10)**2 + (ry - 8)**2 <= 16
            c3 = (x - 7.5)**2 + (ry - 5)**2 <= 16
            c4 = (x - 7.5)**2 + (ry - 11)**2 <= 16
            if c1 or c2 or c3 or c4:
                set_pixel(x, y, 245, 245, 245, 235)

    # 6. Tile (16..31, 16..31): Fire / Ember Flame (LavaEmber)
    for y in range(16, 32):
        for x in range(16, 32):
            rx = x - 16
            ry = y - 16
            dx = abs(rx - 7.5)
            if dx <= (15 - ry) * 0.6 and ry >= 2:
                r, g, b = 255, 160, 20
                if ry <= 6:
                    r, g, b = 255, 240, 100  # Flame tip highlight
                elif ry > 10:
                    r, g, b = 220, 50, 10   # Flame base dark orange
                set_pixel(x, y, r, g, b, 245)

    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0)
        raw_data.extend(pixels[y * width * 4:(y + 1) * width * 4])

    compressed = zlib.compress(raw_data, 9)

    png = bytearray()
    png.extend(b'\x89PNG\r\n\x1a\n')

    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    png.extend(struct.pack('>I', len(ihdr_data)) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc))

    idat_crc = zlib.crc32(b'IDAT' + compressed)
    png.extend(struct.pack('>I', len(compressed)) + b'IDAT' + compressed + struct.pack('>I', idat_crc))

    iend_crc = zlib.crc32(b'IEND')
    png.extend(struct.pack('>I', 0) + b'IEND' + struct.pack('>I', iend_crc))

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(png)
    print(f"Saved particle texture PNG to {output_path}")

def generate_particle_json(json_path, image_name="particles.png"):
    presets = {
        "Stomp": {
            "rect": [0, 0, 16, 16],
            "count": 6,
            "minVelocity": [-140.0, -30.0],
            "maxVelocity": [140.0, 0.0],
            "acceleration": [0.0, -30.0],
            "startColor": [220, 220, 220, 200],
            "endColor": [220, 220, 220, 0],
            "minLifetime": 0.25,
            "maxLifetime": 0.45,
            "startScale": 1.0,
            "endScale": 0.3
        },
        "WallDust": {
            "rect": [0, 0, 16, 16],
            "count": 4,
            "minVelocity": [-30.0, -30.0],
            "maxVelocity": [30.0, 10.0],
            "acceleration": [0.0, 0.0],
            "startColor": [200, 200, 200, 180],
            "endColor": [200, 200, 200, 0],
            "minLifetime": 0.2,
            "maxLifetime": 0.4,
            "startScale": 0.6,
            "endScale": 0.1
        },
        "CoinSparkle": {
            "rect": [16, 0, 16, 16],
            "count": 8,
            "minVelocity": [-60.0, -60.0],
            "maxVelocity": [60.0, 60.0],
            "acceleration": [0.0, 0.0],
            "startColor": [255, 215, 0, 255],
            "endColor": [255, 215, 0, 0],
            "minLifetime": 0.3,
            "maxLifetime": 0.5,
            "startScale": 0.8,
            "endScale": 0.1
        },
        "Combo": {
            "rect": [16, 0, 16, 16],
            "count": 6,
            "minVelocity": [-80.0, -160.0],
            "maxVelocity": [80.0, -60.0],
            "acceleration": [0.0, 120.0],
            "startColor": [255, 255, 120, 255],
            "endColor": [255, 100, 255, 0],
            "minLifetime": 0.5,
            "maxLifetime": 0.9,
            "startScale": 1.0,
            "endScale": 0.2
        },
        "BrickBreak": {
            "rect": [32, 0, 16, 16],
            "count": 4,
            "minVelocity": [-120.0, -300.0],
            "maxVelocity": [120.0, -100.0],
            "acceleration": [0.0, 700.0],
            "startColor": [255, 255, 255, 255],
            "endColor": [255, 255, 255, 0],
            "minLifetime": 0.6,
            "maxLifetime": 0.9,
            "startScale": 1.0,
            "endScale": 0.4
        },
        "WaterBubble": {
            "rect": [48, 0, 16, 16],
            "count": 3,
            "minVelocity": [-15.0, -50.0],
            "maxVelocity": [15.0, -20.0],
            "acceleration": [0.0, -15.0],
            "startColor": [180, 220, 255, 180],
            "endColor": [180, 220, 255, 0],
            "minLifetime": 0.8,
            "maxLifetime": 1.4,
            "startScale": 0.5,
            "endScale": 0.9
        },
        "DeathPoof": {
            "rect": [0, 16, 16, 16],
            "count": 12,
            "minVelocity": [-100.0, -100.0],
            "maxVelocity": [100.0, 100.0],
            "acceleration": [0.0, 0.0],
            "startColor": [240, 240, 240, 255],
            "endColor": [240, 240, 240, 0],
            "minLifetime": 0.4,
            "maxLifetime": 0.7,
            "startScale": 1.2,
            "endScale": 0.2
        },
        "LavaEmber": {
            "rect": [16, 16, 16, 16],
            "count": 3,
            "minVelocity": [-25.0, -90.0],
            "maxVelocity": [25.0, -30.0],
            "acceleration": [0.0, -10.0],
            "startColor": [255, 120, 0, 255],
            "endColor": [255, 0, 0, 0],
            "minLifetime": 0.7,
            "maxLifetime": 1.2,
            "startScale": 0.6,
            "endScale": 0.1
        }
    }

    frames = {}
    preset_meta = {}

    for name, data in presets.items():
        rx, ry, rw, rh = data["rect"]
        frames[name] = {
            "frame": {"x": rx, "y": ry, "w": rw, "h": rh},
            "rotated": False,
            "trimmed": False,
            "spriteSourceSize": {"x": 0, "y": 0, "w": rw, "h": rh},
            "sourceSize": {"w": rw, "h": rh}
        }
        preset_meta[name] = {k: v for k, v in data.items() if k != "rect"}

    texture_data = {
        "frames": frames,
        "meta": {
            "app": "Sprite Sheet Maker",
            "version": "1.0",
            "image": image_name,
            "format": "RGBA8888",
            "size": {"w": 64, "h": 64},
            "scale": "1",
            "presets": preset_meta
        }
    }

    os.makedirs(os.path.dirname(os.path.abspath(json_path)), exist_ok=True)
    with open(json_path, 'w') as f:
        json.dump(texture_data, f, indent=2)
    print(f"Saved particle metadata JSON to {json_path} in test.json format")

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.abspath(os.path.join(script_dir, ".."))
    
    particles_png = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "particles", "particles.png")
    particles_json = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "particles", "particles.json")
    test_png = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "test", "test.png")
    config_json = os.path.join(root_dir, "SuperMarioGame", "assets", "config", "particles.json")

    create_particle_texture(particles_png)
    create_particle_texture(test_png)
    generate_particle_json(particles_json, "particles.png")
    generate_particle_json(config_json, "particles.png")
