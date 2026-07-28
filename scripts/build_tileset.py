import os
import sys
import zlib
import struct
import json
import math

class TileCanvas:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = bytearray(width * height * 4) # RGBA

    def set_pixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            idx = (y * self.width + x) * 4
            self.pixels[idx]     = color[0]
            self.pixels[idx + 1] = color[1]
            self.pixels[idx + 2] = color[2]
            self.pixels[idx + 3] = color[3] if len(color) > 3 else 255

    def draw_rect(self, x, y, w, h, fill_color, border_color=None):
        for py in range(y, y + h):
            for px in range(x, x + w):
                if border_color and (py == y or py == y + h - 1 or px == x or px == x + w - 1):
                    self.set_pixel(px, py, border_color)
                elif fill_color:
                    self.set_pixel(px, py, fill_color)

    def draw_circle(self, cx, cy, radius, fill_color, border_color=None):
        r_sq = radius * radius
        for py in range(int(cy - radius - 1), int(cy + radius + 2)):
            for px in range(int(cx - radius - 1), int(cx + radius + 2)):
                dist_sq = (px - cx) ** 2 + (py - cy) ** 2
                if dist_sq <= r_sq:
                    if border_color and dist_sq >= (radius - 1) ** 2:
                        self.set_pixel(px, py, border_color)
                    elif fill_color:
                        self.set_pixel(px, py, fill_color)

    def save_png(self, filepath):
        os.makedirs(os.path.dirname(os.path.abspath(filepath)), exist_ok=True)
        png_sig = b'\x89PNG\r\n\x1a\n'
        ihdr_data = struct.pack('>IIBBBBB', self.width, self.height, 8, 6, 0, 0, 0)
        ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
        ihdr_chunk = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)

        raw_bytes = bytearray()
        row_len = self.width * 4
        for y in range(self.height):
            raw_bytes.append(0) # No filter
            start = y * row_len
            raw_bytes.extend(self.pixels[start : start + row_len])

        compressed = zlib.compress(raw_bytes, level=9)
        idat_crc = zlib.crc32(b'IDAT' + compressed)
        idat_chunk = struct.pack('>I', len(compressed)) + b'IDAT' + compressed + struct.pack('>I', idat_crc)

        iend_crc = zlib.crc32(b'IEND')
        iend_chunk = struct.pack('>I', 0) + b'IEND' + struct.pack('>I', iend_crc)

        with open(filepath, 'wb') as f:
            f.write(png_sig + ihdr_chunk + idat_chunk + iend_chunk)
        print(f"Saved PNG to {filepath}")

def hex_to_rgba(h, a=255):
    h = h.lstrip('#')
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)

# Palette definitions (authentic 16-bit SNES SMW palette)
C_TRANSPARENT = (0, 0, 0, 0)
C_BLACK       = (16, 16, 16, 255)
C_WHITE       = (255, 255, 255, 255)

# Grass & Dirt Palette
C_GRASS_HI    = hex_to_rgba("#80E818")
C_GRASS_MID   = hex_to_rgba("#38A000")
C_GRASS_SH    = hex_to_rgba("#185800")
C_DIRT_HI     = hex_to_rgba("#E89048")
C_DIRT_MID    = hex_to_rgba("#C86018")
C_DIRT_SH     = hex_to_rgba("#883808")
C_DIRT_DARK   = hex_to_rgba("#381808")
C_ROCK_GRAY   = hex_to_rgba("#685848")

# Cave Palette
C_CAVE_HI     = hex_to_rgba("#78A8D8")
C_CAVE_MID    = hex_to_rgba("#385070")
C_CAVE_DARK   = hex_to_rgba("#203048")
C_CAVE_SH     = hex_to_rgba("#101828")

# Castle Palette
C_CASTLE_HI   = hex_to_rgba("#A8A8B0")
C_CASTLE_MID  = hex_to_rgba("#686870")
C_CASTLE_SH   = hex_to_rgba("#383840")
C_CASTLE_DARK = hex_to_rgba("#181820")

# Brick Palette
C_BRICK_HI    = hex_to_rgba("#F89050")
C_BRICK_MID   = hex_to_rgba("#C84808")
C_BRICK_SH    = hex_to_rgba("#701800")
C_BRICK_DARK  = hex_to_rgba("#280800")

# Question Block Palette
C_GOLD_HI     = hex_to_rgba("#FFF8A0")
C_GOLD_MID    = hex_to_rgba("#F8B000")
C_GOLD_SH     = hex_to_rgba("#B87000")
C_GOLD_DARK   = hex_to_rgba("#482000")
C_HIT_HI      = hex_to_rgba("#A08068")
C_HIT_MID     = hex_to_rgba("#785840")
C_HIT_SH      = hex_to_rgba("#483020")

# Pipe Palette
C_PIPE_HI     = hex_to_rgba("#A8F8A8")
C_PIPE_MIDHI  = hex_to_rgba("#00D800")
C_PIPE_MID    = hex_to_rgba("#009000")
C_PIPE_SH     = hex_to_rgba("#004800")
C_PIPE_DARK   = hex_to_rgba("#002000")

# Ice Palette
C_ICE_HI      = hex_to_rgba("#E0F8FF")
C_ICE_MID     = hex_to_rgba("#70D8F8")
C_ICE_SH      = hex_to_rgba("#2090C8")
C_ICE_DARK    = hex_to_rgba("#104070")

# Conveyor Palette
C_CONV_STEEL_HI = hex_to_rgba("#808088")
C_CONV_STEEL    = hex_to_rgba("#484850")
C_CONV_STEEL_SH = hex_to_rgba("#202028")
C_CONV_BELT     = hex_to_rgba("#181818")
C_CONV_ARROW    = hex_to_rgba("#F8D000")

# Water Palette
C_WATER_SURF  = hex_to_rgba("#E0F8FF", 220)
C_WATER_MID   = hex_to_rgba("#3090F8", 220)
C_WATER_SH    = hex_to_rgba("#1050B8", 240)
C_WATER_DEEP  = hex_to_rgba("#083078", 255)

# POW Block Palette
C_POW_HI      = hex_to_rgba("#70B0F8")
C_POW_MID     = hex_to_rgba("#1860D8")
C_POW_SH      = hex_to_rgba("#083080")

# Platform & Flag Palette
C_WOOD_HI     = hex_to_rgba("#F8B060")
C_WOOD_MID    = hex_to_rgba("#C87020")
C_WOOD_SH     = hex_to_rgba("#703808")
C_FLAG_RED_HI = hex_to_rgba("#F86060")
C_FLAG_RED    = hex_to_rgba("#E01818")
C_FLAG_RED_SH = hex_to_rgba("#880000")


def generate_tileset():
    cols, rows = 8, 6
    tile_size = 32
    canvas = TileCanvas(cols * tile_size, rows * tile_size)

    # -------------------------------------------------------------
    # ROW 0: Ground & Bricks
    # -------------------------------------------------------------
    
    # 0,0: ground_grass_top
    ox, oy = 0 * 32, 0 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_DIRT_MID, C_DIRT_DARK)
    # Dirt texture detail
    for py in range(oy + 8, oy + 31):
        for px in range(ox + 1, ox + 31):
            if (px + py * 3) % 7 == 0:
                canvas.set_pixel(px, py, C_DIRT_SH)
            elif (px * 5 + py) % 11 == 0:
                canvas.set_pixel(px, py, C_DIRT_HI)
            elif (px + py) % 13 == 0:
                canvas.set_pixel(px, py, C_ROCK_GRAY)
    # Grass layer top (y=0..7)
    canvas.draw_rect(ox, oy, 32, 8, C_GRASS_MID)
    canvas.draw_rect(ox, oy, 32, 2, C_GRASS_HI)
    canvas.draw_rect(ox, oy + 7, 32, 1, C_GRASS_SH)
    # Grass blade tufts dipping into dirt
    for dx in range(0, 32, 4):
        canvas.set_pixel(ox + dx, oy + 8, C_GRASS_MID)
        canvas.set_pixel(ox + dx + 1, oy + 8, C_GRASS_MID)
        canvas.set_pixel(ox + dx + 1, oy + 9, C_GRASS_SH)

    # 1,0: ground_dirt
    ox, oy = 1 * 32, 0 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_DIRT_MID, C_DIRT_DARK)
    for py in range(oy + 1, oy + 31):
        for px in range(ox + 1, ox + 31):
            if (px * 3 + py * 7) % 9 == 0:
                canvas.set_pixel(px, py, C_DIRT_SH)
            elif (px * 7 + py * 3) % 13 == 0:
                canvas.set_pixel(px, py, C_DIRT_HI)
            elif (px * 2 + py * 4) % 17 == 0:
                canvas.set_pixel(px, py, C_ROCK_GRAY)

    # 2,0: ground_underground
    ox, oy = 2 * 32, 0 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_CAVE_MID, C_CAVE_SH)
    for py in range(oy + 1, oy + 31):
        for px in range(ox + 1, ox + 31):
            if (px * 4 + py * 2) % 7 == 0:
                canvas.set_pixel(px, py, C_CAVE_DARK)
            elif (px + py * 3) % 11 == 0:
                canvas.set_pixel(px, py, C_CAVE_HI)
    # Cracks in rock
    for i in range(5):
        canvas.set_pixel(ox + 6 + i, oy + 10 + i, C_CAVE_DARK)
        canvas.set_pixel(ox + 20 - i, oy + 18 + i, C_CAVE_DARK)

    # 3,0: ground_castle
    ox, oy = 3 * 32, 0 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_CASTLE_MID, C_CASTLE_DARK)
    # Brick pattern (2 rows of stones)
    canvas.draw_rect(ox, oy + 15, 32, 2, C_CASTLE_DARK)
    # Vertical mortar cuts
    canvas.draw_rect(ox + 15, oy, 2, 15, C_CASTLE_DARK)
    canvas.draw_rect(ox + 7, oy + 17, 2, 15, C_CASTLE_DARK)
    canvas.draw_rect(ox + 23, oy + 17, 2, 15, C_CASTLE_DARK)
    # Bevels on stone blocks
    canvas.draw_rect(ox + 1, oy + 1, 14, 1, C_CASTLE_HI)
    canvas.draw_rect(ox + 17, oy + 1, 14, 1, C_CASTLE_HI)
    canvas.draw_rect(ox + 1, oy + 17, 6, 1, C_CASTLE_HI)
    canvas.draw_rect(ox + 9, oy + 17, 14, 1, C_CASTLE_HI)
    canvas.draw_rect(ox + 25, oy + 17, 6, 1, C_CASTLE_HI)

    # 4,0: brick
    ox, oy = 4 * 32, 0 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_BRICK_MID, C_BRICK_DARK)
    # Top & Left bevels
    canvas.draw_rect(ox + 1, oy + 1, 30, 1, C_BRICK_HI)
    canvas.draw_rect(ox + 1, oy + 1, 1, 30, C_BRICK_HI)
    # Horizontal mortar lines (4 rows)
    for row in range(1, 4):
        canvas.draw_rect(ox + 1, oy + row * 8, 30, 1, C_BRICK_SH)
        canvas.draw_rect(ox + 1, oy + row * 8 + 1, 30, 1, C_BRICK_DARK)
    # Vertical mortar joints
    for r in range(4):
        y_start = oy + r * 8 + 2
        if r % 2 == 0:
            canvas.draw_rect(ox + 8, y_start, 1, 6, C_BRICK_DARK)
            canvas.draw_rect(ox + 24, y_start, 1, 6, C_BRICK_DARK)
        else:
            canvas.draw_rect(ox + 16, y_start, 1, 6, C_BRICK_DARK)

    # 5,0 .. 7,0 & 2,5: shattered brick debris
    debris_coords = [(5*32, 0), (6*32, 0), (7*32, 0), (2*32, 5*32)]
    offsets = [(8, 8), (14, 6), (6, 14), (12, 12)]
    for i, (dox, doy) in enumerate(debris_coords):
        bx, by = dox + offsets[i][0], doy + offsets[i][1]
        canvas.draw_rect(bx, by, 12, 10, C_BRICK_MID, C_BRICK_DARK)
        canvas.draw_rect(bx + 1, by + 1, 10, 1, C_BRICK_HI)
        canvas.draw_rect(bx + 1, by + 1, 1, 8, C_BRICK_HI)

    # -------------------------------------------------------------
    # ROW 1: Question Blocks & Pipe Top
    # -------------------------------------------------------------
    
    # Question block template helper
    def draw_q_block(ox, oy, fill_c, hi_c, sh_c, dark_c, q_shift_y=0, sparkle=False):
        canvas.draw_rect(ox, oy, 32, 32, fill_c, dark_c)
        canvas.draw_rect(ox + 1, oy + 1, 30, 1, hi_c)
        canvas.draw_rect(ox + 1, oy + 1, 1, 30, hi_c)
        canvas.draw_rect(ox + 1, oy + 30, 30, 1, sh_c)
        canvas.draw_rect(ox + 30, oy + 1, 1, 30, sh_c)
        # Corner bolts
        bolts = [(ox+3, oy+3), (ox+27, oy+3), (ox+3, oy+27), (ox+27, oy+27)]
        for bx, by in bolts:
            canvas.draw_rect(bx, by, 2, 2, dark_c)
        # Question mark '?'
        qy = oy + 7 + q_shift_y
        qx = ox + 11
        # Top loop
        canvas.draw_rect(qx + 2, qy, 6, 2, C_WHITE)
        canvas.draw_rect(qx, qy + 2, 3, 3, C_WHITE)
        canvas.draw_rect(qx + 7, qy + 2, 3, 4, C_WHITE)
        # Stem
        canvas.draw_rect(qx + 5, qy + 5, 3, 4, C_WHITE)
        canvas.draw_rect(qx + 3, qy + 8, 3, 3, C_WHITE)
        # Dot
        canvas.draw_rect(qx + 3, qy + 13, 3, 3, C_WHITE)

        if sparkle:
            canvas.draw_rect(ox + 23, oy + 5, 3, 3, C_WHITE)
            canvas.set_pixel(ox + 24, oy + 4, C_WHITE)
            canvas.set_pixel(ox + 24, oy + 8, C_WHITE)

    # 0,1: question_0
    draw_q_block(0*32, 1*32, C_GOLD_MID, C_GOLD_HI, C_GOLD_SH, C_GOLD_DARK, q_shift_y=0)

    # 1,1: question_1 (bright pulse)
    draw_q_block(1*32, 1*32, hex_to_rgba("#F8D020"), hex_to_rgba("#FFFFC0"), C_GOLD_MID, C_GOLD_DARK, q_shift_y=-1)

    # 2,1: question_2 (amber)
    draw_q_block(2*32, 1*32, hex_to_rgba("#E89000"), C_GOLD_MID, hex_to_rgba("#904000"), C_GOLD_DARK, q_shift_y=0)

    # 3,1: question_3 (sparkle)
    draw_q_block(3*32, 1*32, C_GOLD_MID, C_GOLD_HI, C_GOLD_SH, C_GOLD_DARK, q_shift_y=1, sparkle=True)

    # 4,1: question_empty
    ox, oy = 4 * 32, 1 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_HIT_MID, C_HIT_SH)
    canvas.draw_rect(ox + 1, oy + 1, 30, 1, C_HIT_HI)
    canvas.draw_rect(ox + 1, oy + 1, 1, 30, C_HIT_HI)
    canvas.draw_rect(ox + 4, oy + 4, 24, 24, C_HIT_SH, C_HIT_HI)
    bolts = [(ox+2, oy+2), (ox+28, oy+2), (ox+2, oy+28), (ox+28, oy+28)]
    for bx, by in bolts:
        canvas.draw_rect(bx, by, 2, 2, C_HIT_SH)

    # 5,1: pipe_top_left
    ox, oy = 5 * 32, 1 * 32
    canvas.draw_rect(ox, oy, 32, 12, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 2, oy + 1, 3, 10, C_PIPE_HI)
    canvas.draw_rect(ox + 5, oy + 1, 4, 10, C_PIPE_MIDHI)
    canvas.draw_rect(ox + 2, oy + 12, 30, 20, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 4, oy + 12, 3, 20, C_PIPE_HI)
    canvas.draw_rect(ox + 7, oy + 12, 4, 20, C_PIPE_MIDHI)

    # 6,1: pipe_top_right
    ox, oy = 6 * 32, 1 * 32
    canvas.draw_rect(ox, oy, 32, 12, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 20, oy + 1, 8, 10, C_PIPE_SH)
    canvas.draw_rect(ox + 28, oy + 1, 3, 10, C_PIPE_DARK)
    canvas.draw_rect(ox, oy + 12, 30, 20, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 18, oy + 12, 8, 20, C_PIPE_SH)
    canvas.draw_rect(ox + 26, oy + 12, 3, 20, C_PIPE_DARK)

    # 7,1: pipe_body_left
    ox, oy = 7 * 32, 1 * 32
    canvas.draw_rect(ox + 2, oy, 30, 32, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 4, oy, 3, 32, C_PIPE_HI)
    canvas.draw_rect(ox + 7, oy, 4, 32, C_PIPE_MIDHI)

    # -------------------------------------------------------------
    # ROW 2: Pipe Body Right, Ice, Conveyor, Water Surface 0 & 1
    # -------------------------------------------------------------

    # 0,2: pipe_body_right
    ox, oy = 0 * 32, 2 * 32
    canvas.draw_rect(ox, oy, 30, 32, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 18, oy, 8, 32, C_PIPE_SH)
    canvas.draw_rect(ox + 26, oy, 3, 32, C_PIPE_DARK)

    # 1,2: ice
    ox, oy = 1 * 32, 2 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_ICE_MID, C_ICE_DARK)
    canvas.draw_rect(ox + 1, oy + 1, 30, 1, C_ICE_HI)
    canvas.draw_rect(ox + 1, oy + 1, 1, 30, C_ICE_HI)
    for i in range(12):
        canvas.set_pixel(ox + 4 + i, oy + 4 + i, C_ICE_HI)
        canvas.set_pixel(ox + 5 + i, oy + 4 + i, C_WHITE)
        canvas.set_pixel(ox + 14 + i, oy + 10 + i, C_ICE_HI)

    # Conveyor belt drawing helper
    def draw_conveyor(ox, oy, arrow_offset):
        canvas.draw_rect(ox, oy, 32, 32, C_CONV_STEEL, C_CONV_STEEL_SH)
        canvas.draw_rect(ox + 1, oy + 1, 30, 1, C_CONV_STEEL_HI)
        canvas.draw_rect(ox + 1, oy + 2, 30, 10, C_CONV_BELT)
        for a in range(-16, 48, 12):
            ax = ox + a + arrow_offset
            if ox <= ax < ox + 30:
                canvas.draw_rect(int(ax), oy + 5, 2, 4, C_CONV_ARROW)
                canvas.draw_rect(int(ax + 2), oy + 6, 2, 2, C_CONV_ARROW)
        canvas.draw_circle(ox + 8, oy + 22, 5, C_CONV_STEEL_SH, C_CONV_STEEL_HI)
        canvas.draw_circle(ox + 24, oy + 22, 5, C_CONV_STEEL_SH, C_CONV_STEEL_HI)

    # 2,2 .. 5,2: conveyor_0 .. conveyor_3
    for c in range(4):
        draw_conveyor((2 + c) * 32, 2 * 32, arrow_offset=c * 3)

    # 6,2 & 7,2: water_surface_0 & 1
    def draw_water_surf(ox, oy, phase):
        canvas.draw_rect(ox, oy + 8, 32, 24, C_WATER_MID)
        canvas.draw_rect(ox, oy + 20, 32, 12, C_WATER_SH)
        for px in range(32):
            wy = int(oy + 6 + math.sin((px + phase * 8) * 0.3) * 2)
            canvas.set_pixel(ox + px, wy, C_WHITE)
            canvas.set_pixel(ox + px, wy + 1, C_WATER_SURF)
            canvas.set_pixel(ox + px, wy + 2, C_WATER_SURF)

    draw_water_surf(6 * 32, 2 * 32, phase=0)
    draw_water_surf(7 * 32, 2 * 32, phase=1)

    # -------------------------------------------------------------
    # ROW 3: Water Surface 2 & 3, Water Depth, Coins, POW Block
    # -------------------------------------------------------------

    draw_water_surf(0 * 32, 3 * 32, phase=2)
    draw_water_surf(1 * 32, 3 * 32, phase=3)

    # 2,3: water_depth
    ox, oy = 2 * 32, 3 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_WATER_SH)
    canvas.draw_rect(ox, oy + 16, 32, 16, C_WATER_DEEP)
    bubbles = [(ox+6, oy+8), (ox+22, oy+14), (ox+12, oy+24), (ox+26, oy+4)]
    for bx, by in bubbles:
        canvas.draw_circle(bx, by, 2, C_WATER_SURF)

    def draw_coin(ox, oy, width_px):
        cx = ox + 16
        hw = width_px // 2
        canvas.draw_rect(cx - hw, oy + 6, width_px, 20, C_GOLD_MID, C_GOLD_DARK)
        canvas.draw_rect(cx - hw + 1, oy + 7, max(1, width_px - 2), 1, C_GOLD_HI)
        canvas.draw_rect(cx - hw + 1, oy + 7, 1, 18, C_GOLD_HI)
        if width_px >= 10:
            canvas.draw_rect(cx - 1, oy + 10, 2, 12, C_GOLD_DARK)

    # 3,3 .. 6,3: coin_0 .. coin_3
    draw_coin(3 * 32, 3 * 32, 16)
    draw_coin(4 * 32, 3 * 32, 12)
    draw_coin(5 * 32, 3 * 32, 4)
    draw_coin(6 * 32, 3 * 32, 12)

    # 7,3: pow_block
    ox, oy = 7 * 32, 3 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_POW_MID, C_BLACK)
    canvas.draw_rect(ox + 1, oy + 1, 30, 1, C_POW_HI)
    canvas.draw_rect(ox + 1, oy + 1, 1, 30, C_POW_HI)
    canvas.draw_rect(ox + 1, oy + 30, 30, 1, C_POW_SH)
    canvas.draw_rect(ox + 30, oy + 1, 1, 30, C_POW_SH)
    canvas.draw_rect(ox + 5, oy + 10, 2, 12, C_WHITE)
    canvas.draw_rect(ox + 7, oy + 10, 4, 2, C_WHITE)
    canvas.draw_rect(ox + 9, oy + 12, 2, 4, C_WHITE)
    canvas.draw_rect(ox + 7, oy + 15, 4, 2, C_WHITE)
    canvas.draw_rect(ox + 13, oy + 10, 6, 12, C_WHITE)
    canvas.draw_rect(ox + 15, oy + 12, 2, 8, C_POW_MID)
    canvas.draw_rect(ox + 21, oy + 10, 2, 12, C_WHITE)
    canvas.draw_rect(ox + 27, oy + 10, 2, 12, C_WHITE)
    canvas.draw_rect(ox + 24, oy + 16, 2, 6, C_WHITE)

    # -------------------------------------------------------------
    # ROW 4: POW Vibrate, P-Switches, Flagpole, Flags
    # -------------------------------------------------------------

    # 0,4: pow_block_vibrate
    ox, oy = 0 * 32, 4 * 32
    canvas.draw_rect(ox + 2, oy + 2, 28, 28, C_POW_MID, C_BLACK)
    canvas.draw_rect(ox + 3, oy + 3, 26, 1, C_POW_HI)
    canvas.draw_rect(ox + 7, oy + 11, 18, 10, C_WHITE)

    # 1,4: p_switch_idle
    ox, oy = 1 * 32, 4 * 32
    canvas.draw_rect(ox, oy + 22, 32, 10, C_CONV_STEEL, C_BLACK)
    canvas.draw_rect(ox + 1, oy + 23, 30, 1, C_CONV_STEEL_HI)
    canvas.draw_rect(ox + 5, oy + 6, 22, 16, C_POW_MID, C_BLACK)
    canvas.draw_rect(ox + 6, oy + 7, 20, 1, C_POW_HI)
    canvas.draw_rect(ox + 13, oy + 9, 2, 10, C_WHITE)
    canvas.draw_rect(ox + 15, oy + 9, 4, 2, C_WHITE)
    canvas.draw_rect(ox + 17, oy + 11, 2, 3, C_WHITE)
    canvas.draw_rect(ox + 15, oy + 13, 4, 2, C_WHITE)

    # 2,4: p_switch_pressed
    ox, oy = 2 * 32, 4 * 32
    canvas.draw_rect(ox, oy + 22, 32, 10, C_CONV_STEEL, C_BLACK)
    canvas.draw_rect(ox + 1, oy + 23, 30, 1, C_CONV_STEEL_HI)
    canvas.draw_rect(ox + 5, oy + 20, 22, 4, C_POW_SH, C_BLACK)

    # 3,4: flagpole_top
    ox, oy = 3 * 32, 4 * 32
    canvas.draw_circle(ox + 16, oy + 12, 8, C_GOLD_MID, C_GOLD_DARK)
    canvas.draw_circle(ox + 14, oy + 10, 3, C_GOLD_HI)
    canvas.draw_rect(ox + 13, oy + 20, 6, 12, C_PIPE_MID, C_BLACK)

    # 4,4: flagpole_pole
    ox, oy = 4 * 32, 4 * 32
    canvas.draw_rect(ox + 12, oy, 8, 32, C_PIPE_MID, C_PIPE_DARK)
    canvas.draw_rect(ox + 13, oy, 2, 32, C_PIPE_HI)

    # 5,4: flagpole_base
    ox, oy = 5 * 32, 4 * 32
    canvas.draw_rect(ox, oy, 32, 32, C_CASTLE_MID, C_CASTLE_DARK)
    canvas.draw_rect(ox + 12, oy, 8, 12, C_CASTLE_DARK)

    # 6,4 & 7,4: flag_0 & flag_1
    def draw_flag(ox, oy, wave_shift):
        canvas.draw_rect(ox, oy, 4, 32, C_PIPE_MID, C_PIPE_DARK)
        fx, fy = ox + 4, oy + 4
        canvas.draw_rect(fx, fy + wave_shift, 24, 16, C_FLAG_RED, C_FLAG_RED_SH)
        canvas.draw_rect(fx + 1, fy + 1 + wave_shift, 22, 1, C_FLAG_RED_HI)
        canvas.draw_circle(fx + 12, fy + 8 + wave_shift, 3, C_GOLD_HI)

    draw_flag(6 * 32, 4 * 32, wave_shift=0)
    draw_flag(7 * 32, 4 * 32, wave_shift=2)

    # -------------------------------------------------------------
    # ROW 5: Platforms & Debris
    # -------------------------------------------------------------

    # 0,5: platform_wood
    ox, oy = 0 * 32, 5 * 32
    canvas.draw_rect(ox, oy + 4, 32, 24, C_WOOD_MID, C_WOOD_SH)
    canvas.draw_rect(ox + 1, oy + 5, 30, 2, C_WOOD_HI)
    canvas.draw_rect(ox, oy + 12, 32, 1, C_WOOD_SH)
    canvas.draw_rect(ox, oy + 20, 32, 1, C_WOOD_SH)
    canvas.draw_rect(ox, oy + 4, 3, 24, C_CONV_STEEL_HI)
    canvas.draw_rect(ox + 29, oy + 4, 3, 24, C_CONV_STEEL_HI)

    # 1,5: platform_stone
    ox, oy = 1 * 32, 5 * 32
    canvas.draw_rect(ox, oy + 4, 32, 24, C_CASTLE_MID, C_CASTLE_DARK)
    canvas.draw_rect(ox + 1, oy + 5, 30, 1, C_CASTLE_HI)
    canvas.set_pixel(ox + 10, oy + 10, C_BLACK)
    canvas.set_pixel(ox + 11, oy + 11, C_BLACK)
    canvas.set_pixel(ox + 12, oy + 12, C_BLACK)
    canvas.set_pixel(ox + 13, oy + 11, C_BLACK)
    canvas.set_pixel(ox + 14, oy + 10, C_BLACK)

    return canvas

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.abspath(os.path.join(script_dir, ".."))
    target_png = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "tilesets", "tileset_blocks.png")
    target_json = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "tilesets", "tileset_blocks.json")

    canvas = generate_tileset()
    canvas.save_png(target_png)

    # Metadata JSON mapping
    tile_map = {
        "tileSize": 32,
        "columns": 8,
        "rows": 6,
        "tiles": {
            "ground_grass_top": {"x": 0, "y": 0, "w": 32, "h": 32},
            "ground_dirt": {"x": 32, "y": 0, "w": 32, "h": 32},
            "ground_underground": {"x": 64, "y": 0, "w": 32, "h": 32},
            "ground_castle": {"x": 96, "y": 0, "w": 32, "h": 32},
            "brick": {"x": 128, "y": 0, "w": 32, "h": 32},
            "brick_debris_0": {"x": 160, "y": 0, "w": 32, "h": 32},
            "brick_debris_1": {"x": 192, "y": 0, "w": 32, "h": 32},
            "brick_debris_2": {"x": 224, "y": 0, "w": 32, "h": 32},
            "question_0": {"x": 0, "y": 32, "w": 32, "h": 32},
            "question_1": {"x": 32, "y": 32, "w": 32, "h": 32},
            "question_2": {"x": 64, "y": 32, "w": 32, "h": 32},
            "question_3": {"x": 96, "y": 32, "w": 32, "h": 32},
            "question_empty": {"x": 128, "y": 32, "w": 32, "h": 32},
            "pipe_top_left": {"x": 160, "y": 32, "w": 32, "h": 32},
            "pipe_top_right": {"x": 192, "y": 32, "w": 32, "h": 32},
            "pipe_body_left": {"x": 224, "y": 32, "w": 32, "h": 32},
            "pipe_body_right": {"x": 0, "y": 64, "w": 32, "h": 32},
            "ice": {"x": 32, "y": 64, "w": 32, "h": 32},
            "conveyor_0": {"x": 64, "y": 64, "w": 32, "h": 32},
            "conveyor_1": {"x": 96, "y": 64, "w": 32, "h": 32},
            "conveyor_2": {"x": 128, "y": 64, "w": 32, "h": 32},
            "conveyor_3": {"x": 160, "y": 64, "w": 32, "h": 32},
            "water_surface_0": {"x": 192, "y": 64, "w": 32, "h": 32},
            "water_surface_1": {"x": 224, "y": 64, "w": 32, "h": 32},
            "water_surface_2": {"x": 0, "y": 96, "w": 32, "h": 32},
            "water_surface_3": {"x": 32, "y": 96, "w": 32, "h": 32},
            "water_depth": {"x": 64, "y": 96, "w": 32, "h": 32},
            "coin_0": {"x": 96, "y": 96, "w": 32, "h": 32},
            "coin_1": {"x": 128, "y": 96, "w": 32, "h": 32},
            "coin_2": {"x": 160, "y": 96, "w": 32, "h": 32},
            "coin_3": {"x": 192, "y": 96, "w": 32, "h": 32},
            "pow_block": {"x": 224, "y": 96, "w": 32, "h": 32},
            "pow_block_vibrate": {"x": 0, "y": 128, "w": 32, "h": 32},
            "p_switch_idle": {"x": 32, "y": 128, "w": 32, "h": 32},
            "p_switch_pressed": {"x": 64, "y": 128, "w": 32, "h": 32},
            "flagpole_top": {"x": 96, "y": 128, "w": 32, "h": 32},
            "flagpole_pole": {"x": 128, "y": 128, "w": 32, "h": 32},
            "flagpole_base": {"x": 160, "y": 128, "w": 32, "h": 32},
            "flag_0": {"x": 192, "y": 128, "w": 32, "h": 32},
            "flag_1": {"x": 224, "y": 128, "w": 32, "h": 32},
            "platform_wood": {"x": 0, "y": 160, "w": 32, "h": 32},
            "platform_stone": {"x": 32, "y": 160, "w": 32, "h": 32}
        },
        "animations": {
            "questionBlock": {
                "frames": [
                    {"x": 0, "y": 32, "w": 32, "h": 32},
                    {"x": 32, "y": 32, "w": 32, "h": 32},
                    {"x": 64, "y": 32, "w": 32, "h": 32},
                    {"x": 96, "y": 32, "w": 32, "h": 32}
                ],
                "frameDuration": 0.15
            },
            "pipe": {
                "top_left": {"x": 160, "y": 32, "w": 32, "h": 32},
                "top_right": {"x": 192, "y": 32, "w": 32, "h": 32},
                "body_left": {"x": 224, "y": 32, "w": 32, "h": 32},
                "body_right": {"x": 0, "y": 64, "w": 32, "h": 32}
            },
            "ice": {
                "idle": {"x": 32, "y": 64, "w": 32, "h": 32}
            },
            "conveyor": {
                "animation": [
                    {"x": 64, "y": 64, "w": 32, "h": 32},
                    {"x": 96, "y": 64, "w": 32, "h": 32},
                    {"x": 128, "y": 64, "w": 32, "h": 32},
                    {"x": 160, "y": 64, "w": 32, "h": 32}
                ],
                "frameDuration": 0.1
            },
            "water": {
                "surfaceAnimation": [
                    {"x": 192, "y": 64, "w": 32, "h": 32},
                    {"x": 224, "y": 64, "w": 32, "h": 32},
                    {"x": 0, "y": 96, "w": 32, "h": 32},
                    {"x": 32, "y": 96, "w": 32, "h": 32}
                ],
                "depth": {"x": 64, "y": 96, "w": 32, "h": 32},
                "frameDuration": 0.2
            },
            "coin": {
                "animation": [
                    {"x": 96, "y": 96, "w": 32, "h": 32},
                    {"x": 128, "y": 96, "w": 32, "h": 32},
                    {"x": 160, "y": 96, "w": 32, "h": 32},
                    {"x": 192, "y": 96, "w": 32, "h": 32}
                ],
                "frameDuration": 0.15
            },
            "pow_block": {
                "idle": {"x": 224, "y": 96, "w": 32, "h": 32},
                "vibrate": {"x": 0, "y": 128, "w": 32, "h": 32}
            },
            "p_switch": {
                "idle": {"x": 32, "y": 128, "w": 32, "h": 32},
                "pressed": {"x": 64, "y": 128, "w": 32, "h": 32}
            },
            "flagpole": {
                "top": {"x": 96, "y": 128, "w": 32, "h": 32},
                "pole": {"x": 128, "y": 128, "w": 32, "h": 32},
                "base": {"x": 160, "y": 128, "w": 32, "h": 32},
                "flagAnimation": [
                    {"x": 192, "y": 128, "w": 32, "h": 32},
                    {"x": 224, "y": 128, "w": 32, "h": 32}
                ]
            },
            "platform": {
                "wood": {"x": 0, "y": 160, "w": 32, "h": 32},
                "stone": {"x": 32, "y": 160, "w": 32, "h": 32}
            }
        }
    }

    os.makedirs(os.path.dirname(os.path.abspath(target_json)), exist_ok=True)
    with open(target_json, "w") as f:
        json.dump(tile_map, f, indent=2)
    print(f"Saved metadata JSON to {target_json}")

if __name__ == "__main__":
    main()
