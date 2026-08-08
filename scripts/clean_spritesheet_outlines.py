#!/usr/bin/env python3
"""
Spritesheet Outline Cleaner Tool
Detects and removes light-blue debug bounding box outlines (RGB 56, 188, 248) from PNG sprite sheets,
restoring transparent background pixels and unblending sprite border pixels.

Does NOT overwrite original files for safety! Saves to <filename>_cleaned.png.
"""

import sys
import os
import zlib
import struct
import json

def read_png_rgba(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    assert data[:8] == b'\x89PNG\r\n\x1a\n', "Not a valid PNG file"
    
    idx = 8
    width = 0
    height = 0
    idat_chunks = []

    while idx < len(data):
        length, chunk_type = struct.unpack('>I4s', data[idx:idx+8])
        idx += 8
        chunk_data = data[idx:idx+length]
        idx += length
        crc = data[idx:idx+4]
        idx += 4

        if chunk_type == b'IHDR':
            width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack('>IIBBBBB', chunk_data)
            assert bit_depth == 8 and color_type == 6, "PNG must be 8-bit RGBA"
        elif chunk_type == b'IDAT':
            idat_chunks.append(chunk_data)
        elif chunk_type == b'IEND':
            break

    decompressed = zlib.decompress(b''.join(idat_chunks))
    stride = width * 4 + 1
    raw_pixels = bytearray(height * width * 4)

    def paeth_predictor(a, b, c):
        p = a + b - c
        pa = abs(p - a)
        pb = abs(p - b)
        pc = abs(p - c)
        if pa <= pb and pa <= pc: return a
        elif pb <= pc: return b
        else: return c

    prev_row = bytearray(width * 4)

    for y in range(height):
        filter_type = decompressed[y * stride]
        row_data = decompressed[y * stride + 1 : (y + 1) * stride]
        current_row = bytearray(width * 4)

        for x in range(width * 4):
            filt = row_data[x]
            a = current_row[x - 4] if x >= 4 else 0
            b = prev_row[x]
            c = prev_row[x - 4] if x >= 4 else 0

            if filter_type == 0: val = filt
            elif filter_type == 1: val = (filt + a) & 0xFF
            elif filter_type == 2: val = (filt + b) & 0xFF
            elif filter_type == 3: val = (filt + ((a + b) // 2)) & 0xFF
            elif filter_type == 4: val = (filt + paeth_predictor(a, b, c)) & 0xFF
            else: val = filt

            current_row[x] = val
            raw_pixels[(y * width * 4) + x] = val

        prev_row = current_row

    return width, height, raw_pixels

def write_png_rgba(filepath, width, height, pixels):
    def make_chunk(chunk_type, data):
        length = len(data)
        crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
        return struct.pack('>I', length) + chunk_type + data + struct.pack('>I', crc)

    header = b'\x89PNG\r\n\x1a\n'
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr = make_chunk(b'IHDR', ihdr_data)

    raw_data = bytearray()
    stride = width * 4
    for y in range(height):
        raw_data.append(0) # Filter type 0 (None)
        raw_data.extend(pixels[y * stride : (y + 1) * stride])

    compressed = zlib.compress(bytes(raw_data), level=9)
    idat = make_chunk(b'IDAT', compressed)
    iend = make_chunk(b'IEND', b'')

    with open(filepath, 'wb') as f:
        f.write(header + ihdr + idat + iend)

def is_light_blue(r, g, b, a):
    if a == 0: return False
    # Light blue bounding box color is rgba(56, 189, 248, 0.5)
    return b > 170 and g > 120 and (b - r) > 35

def clean_spritesheet(png_path, json_path, out_path):
    width, height, pixels = read_png_rgba(png_path)
    with open(json_path, 'r') as f:
        meta = json.load(f)

    frames = meta.get("frames", {})
    cleaned_count = 0

    def get_pixel(x, y):
        if 0 <= x < width and 0 <= y < height:
            idx = (y * width + x) * 4
            return pixels[idx], pixels[idx+1], pixels[idx+2], pixels[idx+3]
        return 0, 0, 0, 0

    def set_pixel(x, y, r, g, b, a):
        idx = (y * width + x) * 4
        pixels[idx] = r
        pixels[idx+1] = g
        pixels[idx+2] = b
        pixels[idx+3] = a

    # Process border pixels of every frame
    for f_data in frames.values():
        fr = f_data["frame"]
        fx, fy, fw, fh = fr["x"], fr["y"], fr["w"], fr["h"]

        border_points = set()
        for x in range(fx, fx + fw):
            border_points.add((x, fy))
            border_points.add((x, fy + fh - 1))
        for y in range(fy, fy + fh):
            border_points.add((fx, y))
            border_points.add((fx + fw - 1, y))

        for px, py in border_points:
            r, g, b, a = get_pixel(px, py)

            if is_light_blue(r, g, b, a):
                # Check neighbors to determine if inside sprite or outside on background
                neighbors = [
                    get_pixel(px-1, py), get_pixel(px+1, py),
                    get_pixel(px, py-1), get_pixel(px, py+1)
                ]
                
                # Count non-light-blue opaque neighbors
                valid_sprite_neighbors = [
                    n for n in neighbors if n[3] > 0 and not is_light_blue(*n)
                ]

                if not valid_sprite_neighbors:
                    # Pure background outline pixel -> restore to transparent
                    set_pixel(px, py, 0, 0, 0, 0)
                else:
                    # Border pixel intersecting sprite body -> sample color from valid neighbor
                    nr, ng, nb, na = valid_sprite_neighbors[0]
                    set_pixel(px, py, nr, ng, nb, na)

                cleaned_count += 1

    write_png_rgba(out_path, width, height, pixels)
    print(f"Cleaned {cleaned_count} outline pixels.")
    print(f"Saved cleaned image to: {out_path}")
    print(f"Original file intact: {png_path}")

def main():
    if len(sys.argv) < 3:
        print("Usage: python clean_spritesheet_outlines.py <input.png> <input.json> [output.png]")
        sys.exit(1)

    png_path = sys.argv[1]
    json_path = sys.argv[2]
    out_path = sys.argv[3] if len(sys.argv) > 3 else png_path.replace(".png", "_cleaned.png")

    clean_spritesheet(png_path, json_path, out_path)

if __name__ == "__main__":
    main()
