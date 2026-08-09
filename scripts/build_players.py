import os
import zlib
import struct
import json
import math

class PlayerCanvas:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = bytearray(width * height * 4)

    def set_pixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            idx = (int(y) * self.width + int(x)) * 4
            self.pixels[idx]     = color[0]
            self.pixels[idx + 1] = color[1]
            self.pixels[idx + 2] = color[2]
            self.pixels[idx + 3] = color[3] if len(color) > 3 else 255

    def draw_rect(self, x, y, w, h, fill_color, border_color=None):
        for py in range(int(y), int(y + h)):
            for px in range(int(x), int(x + w)):
                if border_color and (py == int(y) or py == int(y + h - 1) or px == int(x) or px == int(x + w - 1)):
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
            raw_bytes.append(0)
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

# Color Palettes
C_TRANSPARENT = (0, 0, 0, 0)
C_BLACK       = (20, 10, 10, 255)

# Mario Palette
M_RED_HI      = hex_to_rgba("#FF7060")
M_RED         = hex_to_rgba("#F83820")
M_RED_SH      = hex_to_rgba("#980000")
M_RED_DARK    = hex_to_rgba("#480000")

M_BLUE_HI     = hex_to_rgba("#5080FF")
M_BLUE        = hex_to_rgba("#2048D8")
M_BLUE_SH     = hex_to_rgba("#082080")

# Luigi Palette
L_GREEN_HI    = hex_to_rgba("#60E040")
L_GREEN       = hex_to_rgba("#20A020")
L_GREEN_SH    = hex_to_rgba("#006000")
L_GREEN_DARK  = hex_to_rgba("#003000")

L_BLUE_HI     = hex_to_rgba("#4060F0")
L_BLUE        = hex_to_rgba("#1830A0")
L_BLUE_SH     = hex_to_rgba("#081860")

M_SKIN_HI     = hex_to_rgba("#FFF0C8")
M_SKIN        = hex_to_rgba("#FFD8A0")
M_SKIN_SH     = hex_to_rgba("#D08040")

M_GLOVE_HI    = hex_to_rgba("#FFFFFF")
M_GLOVE       = hex_to_rgba("#E8EEF8")
M_GLOVE_SH    = hex_to_rgba("#A0A8C0")

M_BOOT_HI     = hex_to_rgba("#A05820")
M_BOOT        = hex_to_rgba("#703810")
M_BOOT_SH     = hex_to_rgba("#381808")
M_BROWN       = M_BOOT
M_BROWN_SH    = M_BOOT_SH

M_YELLOW      = hex_to_rgba("#FFE000")
M_WHITE       = hex_to_rgba("#FFFFFF")
M_WHITE_SH    = hex_to_rgba("#B0B8C8")

M_CAPE_HI     = hex_to_rgba("#FFF880")
M_CAPE        = hex_to_rgba("#FFE020")
M_CAPE_SH     = hex_to_rgba("#C09800")
M_DUST        = hex_to_rgba("#E8E8E8", 220)


# =========================================================================
# MARIO DRAWING FUNCTIONS
# =========================================================================
def draw_small_mario_detailed(canvas, ox, oy, pose="idle", step=0, shirt_c=M_RED, overall_c=M_BLUE):
    py_cap = oy + (8 if pose in ["crouch", "ground_pound"] else (2 if pose == "death" else 4))
    if pose == "death":
        canvas.draw_rect(ox + 8, py_cap, 14, 5, shirt_c, M_RED_DARK)
        canvas.draw_rect(ox + 10, py_cap + 1, 10, 1, M_RED_HI if shirt_c == M_RED else M_WHITE)
    elif pose in ["run", "skid"]:
        canvas.draw_rect(ox + 9, py_cap + 1, 14, 5, shirt_c, M_RED_DARK)
        canvas.draw_rect(ox + 11, py_cap + 1, 10, 1, M_RED_HI if shirt_c == M_RED else M_WHITE)
        canvas.draw_rect(ox + 19, py_cap + 3, 7, 2, shirt_c)
    else:
        canvas.draw_rect(ox + 9, py_cap, 14, 5, shirt_c, M_RED_DARK)
        canvas.draw_rect(ox + 11, py_cap, 10, 1, M_RED_HI if shirt_c == M_RED else M_WHITE)
        canvas.draw_rect(ox + 17, py_cap + 2, 7, 2, shirt_c)

    py_face = py_cap + 4
    if pose == "climb":
        canvas.draw_rect(ox + 9, py_face, 13, 6, M_BROWN)
    else:
        canvas.draw_rect(ox + 7, py_face + 1, 4, 6, M_BOOT)
        canvas.draw_rect(ox + 9, py_face, 11, 7, M_SKIN)
        canvas.draw_rect(ox + 10, py_face, 8, 1, M_SKIN_HI)
        canvas.draw_rect(ox + 9, py_face + 6, 10, 1, M_SKIN_SH)

        if pose == "death":
            canvas.draw_rect(ox + 11, py_face + 2, 3, 3, C_BLACK)
            canvas.draw_rect(ox + 17, py_face + 2, 3, 3, C_BLACK)
            canvas.draw_rect(ox + 13, py_face + 5, 4, 2, M_BOOT_SH)
        elif pose in ["damaged", "ground_pound"]:
            canvas.set_pixel(ox + 17, py_face + 2, C_BLACK)
            canvas.set_pixel(ox + 18, py_face + 3, C_BLACK)
            canvas.set_pixel(ox + 19, py_face + 2, C_BLACK)
            canvas.draw_rect(ox + 15, py_face + 4, 5, 2, M_BOOT)
        else:
            canvas.draw_rect(ox + 17, py_face + 2, 2, 3, C_BLACK)
            canvas.set_pixel(ox + 17, py_face + 2, M_WHITE)
            canvas.draw_rect(ox + 19, py_face + 2, 4, 3, M_SKIN)
            canvas.set_pixel(ox + 20, py_face + 2, M_SKIN_HI)
            canvas.draw_rect(ox + 15, py_face + 4, 7, 3, M_BOOT, M_BOOT_SH)

    py_body = py_face + 6

    canvas.draw_rect(ox + 9, py_body + 1, 14, 6, shirt_c)
    canvas.draw_rect(ox + 11, py_body + 3, 10, 7, overall_c, M_BLUE_SH if overall_c == M_BLUE else M_RED_SH)
    canvas.draw_rect(ox + 12, py_body + 4, 2, 2, M_YELLOW)
    canvas.draw_rect(ox + 17, py_body + 4, 2, 2, M_YELLOW)

    if pose in ["idle", "crouch"]:
        canvas.draw_rect(ox + 6, py_body + 2, 4, 5, shirt_c)
        canvas.draw_rect(ox + 5, py_body + 5, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 21, py_body + 2, 4, 5, shirt_c)
        canvas.draw_rect(ox + 22, py_body + 5, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
    elif pose in ["walk", "run"]:
        if step == 0:
            canvas.draw_rect(ox + 4, py_body, 5, 5, shirt_c)
            canvas.draw_rect(ox + 3, py_body + 3, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 22, py_body + 4, 5, 5, shirt_c)
            canvas.draw_rect(ox + 24, py_body + 7, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        elif step == 1:
            canvas.draw_rect(ox + 6, py_body + 2, 4, 5, shirt_c)
            canvas.draw_rect(ox + 5, py_body + 6, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 21, py_body + 2, 4, 5, shirt_c)
            canvas.draw_rect(ox + 22, py_body + 6, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        else:
            canvas.draw_rect(ox + 22, py_body, 5, 5, shirt_c)
            canvas.draw_rect(ox + 24, py_body + 3, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 4, py_body + 4, 5, 5, shirt_c)
            canvas.draw_rect(ox + 3, py_body + 7, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
    elif pose in ["jump", "flutter", "double_jump"]:
        canvas.draw_rect(ox + 19, py_body - 6, 5, 8, shirt_c)
        canvas.draw_rect(ox + 19, py_body - 10, 5, 5, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 5, py_body + 3, 5, 5, shirt_c)
        canvas.draw_rect(ox + 4, py_body + 6, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
    elif pose in ["fall", "slide", "skid"]:
        canvas.draw_rect(ox + 3, py_body + 1, 5, 5, shirt_c)
        canvas.draw_rect(ox + 2, py_body + 4, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 22, py_body + 1, 5, 5, shirt_c)
        canvas.draw_rect(ox + 24, py_body + 4, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        if pose == "skid":
            canvas.draw_rect(ox + 2, oy + 26, 4, 3, M_DUST)
            canvas.draw_rect(ox + 4, oy + 28, 4, 3, M_DUST)
    elif pose == "wall_slide":
        canvas.draw_rect(ox + 22, py_body - 2, 5, 6, shirt_c)
        canvas.draw_rect(ox + 24, py_body - 5, 4, 4, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 5, py_body + 2, 4, 5, shirt_c)
    elif pose == "climb":
        if step == 0:
            canvas.draw_rect(ox + 4, py_body - 2, 4, 6, shirt_c)
            canvas.draw_rect(ox + 3, py_body - 5, 4, 4, M_GLOVE_HI)
            canvas.draw_rect(ox + 21, py_body + 2, 4, 6, shirt_c)
        else:
            canvas.draw_rect(ox + 21, py_body - 2, 4, 6, shirt_c)
            canvas.draw_rect(ox + 22, py_body - 5, 4, 4, M_GLOVE_HI)
            canvas.draw_rect(ox + 4, py_body + 2, 4, 6, shirt_c)
    elif pose in ["swim", "ground_pound"]:
        canvas.draw_rect(ox + 3, py_body + 2, 6, 4, shirt_c)
        canvas.draw_rect(ox + 2, py_body + 4, 4, 4, M_GLOVE_HI)
        canvas.draw_rect(ox + 22, py_body + 2, 6, 4, shirt_c)
        canvas.draw_rect(ox + 24, py_body + 4, 4, 4, M_GLOVE_HI)
    elif pose in ["damaged", "death"]:
        canvas.draw_rect(ox + 3, py_body + 1, 6, 5, shirt_c)
        canvas.draw_rect(ox + 2, py_body - 2, 5, 5, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 23, py_body + 1, 6, 5, shirt_c)
        canvas.draw_rect(ox + 25, py_body - 2, 5, 5, M_GLOVE_HI, M_GLOVE_SH)

    py_leg = py_body + 8

    if pose in ["idle", "crouch", "damaged"]:
        canvas.draw_rect(ox + 10, py_leg, 5, 4, overall_c)
        canvas.draw_rect(ox + 17, py_leg, 5, 4, overall_c)
        canvas.draw_rect(ox + 8, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 17, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
    elif pose in ["walk", "run"]:
        if step == 0:
            canvas.draw_rect(ox + 13, py_leg, 6, 4, overall_c)
            canvas.draw_rect(ox + 12, py_leg + 3, 9, 5, M_BOOT, M_BOOT_SH)
        elif step == 1:
            canvas.draw_rect(ox + 9, py_leg, 5, 4, overall_c)
            canvas.draw_rect(ox + 18, py_leg, 5, 4, overall_c)
            canvas.draw_rect(ox + 7, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 15, py_leg, 6, 4, overall_c)
            canvas.draw_rect(ox + 16, py_leg + 3, 10, 5, M_BOOT, M_BOOT_SH)
    elif pose in ["skid", "slide"]:
        canvas.draw_rect(ox + 8, py_leg, 6, 4, overall_c)
        canvas.draw_rect(ox + 18, py_leg, 6, 4, overall_c)
        canvas.draw_rect(ox + 6, py_leg + 3, 9, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 18, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
    elif pose in ["jump", "wall_slide"]:
        canvas.draw_rect(ox + 8, py_leg - 1, 6, 4, overall_c)
        canvas.draw_rect(ox + 6, py_leg + 2, 8, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 18, py_leg + 2, 5, 4, overall_c)
        canvas.draw_rect(ox + 18, py_leg + 5, 7, 4, M_BOOT, M_BOOT_SH)
    elif pose == "flutter":
        # LUIGI SPECIAL: Flutter kick legs in air
        if step == 0:
            canvas.draw_rect(ox + 6, py_leg, 6, 4, overall_c)
            canvas.draw_rect(ox + 4, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_leg - 2, 6, 4, overall_c)
            canvas.draw_rect(ox + 18, py_leg + 1, 8, 5, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 6, py_leg - 2, 6, 4, overall_c)
            canvas.draw_rect(ox + 4, py_leg + 1, 8, 5, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_leg, 6, 4, overall_c)
            canvas.draw_rect(ox + 18, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
    elif pose == "double_jump":
        # LUIGI SPECIAL: 360 Spin Flip in mid-air
        canvas.draw_rect(ox + 8, py_leg - 3, 14, 8, overall_c)
        canvas.draw_rect(ox + 6, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 16, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
    elif pose in ["fall", "swim"]:
        canvas.draw_rect(ox + 7, py_leg, 6, 4, overall_c)
        canvas.draw_rect(ox + 19, py_leg, 6, 4, overall_c)
        canvas.draw_rect(ox + 5, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 19, py_leg + 3, 8, 5, M_BOOT, M_BOOT_SH)
    elif pose in ["climb", "ground_pound"]:
        if step == 0:
            canvas.draw_rect(ox + 9, py_leg, 5, 4, overall_c)
            canvas.draw_rect(ox + 8, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_leg - 2, 5, 4, overall_c)
            canvas.draw_rect(ox + 18, py_leg + 1, 7, 5, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 9, py_leg - 2, 5, 4, overall_c)
            canvas.draw_rect(ox + 8, py_leg + 1, 7, 5, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_leg, 5, 4, overall_c)
            canvas.draw_rect(ox + 18, py_leg + 3, 7, 5, M_BOOT, M_BOOT_SH)
    elif pose == "death":
        canvas.draw_rect(ox + 8, py_leg + 3, 6, 5, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 18, py_leg + 3, 6, 5, M_BOOT, M_BOOT_SH)


def draw_super_mario_detailed(canvas, ox, oy, pose="idle", step=0, shirt_c=M_RED, overall_c=M_BLUE, has_cape=False):
    if has_cape:
        if pose == "crouch":
            canvas.draw_rect(ox + 4, oy + 32, 8, 20, M_CAPE, M_CAPE_SH)
            canvas.draw_rect(ox + 5, oy + 34, 5, 3, M_CAPE_HI)
        elif pose in ["glide", "swoop"]:
            canvas.draw_rect(ox + 1, oy + 22, 30, 24, M_CAPE, M_CAPE_SH)
            canvas.draw_rect(ox + 3, oy + 24, 26, 3, M_CAPE_HI)
        elif pose == "spin":
            canvas.draw_rect(ox, oy + 28, 32, 20, M_CAPE, M_CAPE_SH)
        else:
            canvas.draw_rect(ox + 3, oy + 22, 8, 34, M_CAPE, M_CAPE_SH)
            canvas.draw_rect(ox + 4, oy + 24, 5, 2, M_CAPE_HI)

    py_head = oy + (16 if pose == "crouch" else 4)
    canvas.draw_rect(ox + 9, py_head, 14, 6, shirt_c, M_RED_DARK)
    canvas.draw_rect(ox + 11, py_head + 1, 10, 2, M_RED_HI if shirt_c == M_RED else M_WHITE)
    canvas.draw_rect(ox + 18, py_head + 3, 8, 3, shirt_c)

    if pose == "climb":
        canvas.draw_rect(ox + 9, py_head + 6, 13, 10, M_BOOT)
    else:
        canvas.draw_rect(ox + 7, py_head + 6, 5, 10, M_BOOT)
        canvas.draw_rect(ox + 10, py_head + 6, 12, 11, M_SKIN)
        canvas.draw_rect(ox + 11, py_head + 6, 9, 2, M_SKIN_HI)
        if pose == "damaged":
            canvas.set_pixel(ox + 19, py_head + 8, C_BLACK)
            canvas.set_pixel(ox + 20, py_head + 9, C_BLACK)
            canvas.set_pixel(ox + 21, py_head + 8, C_BLACK)
        else:
            canvas.draw_rect(ox + 19, py_head + 8, 3, 4, C_BLACK)
            canvas.set_pixel(ox + 19, py_head + 8, M_WHITE)
        canvas.draw_rect(ox + 22, py_head + 9, 4, 3, M_SKIN)
        canvas.set_pixel(ox + 23, py_head + 9, M_SKIN_HI)
        canvas.draw_rect(ox + 17, py_head + 11, 8, 4, M_BOOT, M_BOOT_SH)

    if has_cape:
        canvas.draw_rect(ox + 11, py_head + 16, 4, 3, M_RED)

    py_torso = py_head + 16

    canvas.draw_rect(ox + 9, py_torso, 14, 13, shirt_c)
    canvas.draw_rect(ox + 11, py_torso + 2, 10, 18, overall_c, M_BLUE_SH if overall_c == M_BLUE else M_RED_SH)
    canvas.draw_rect(ox + 13, py_torso + 5, 2, 3, M_YELLOW)
    canvas.draw_rect(ox + 17, py_torso + 5, 2, 3, M_YELLOW)

    if pose in ["idle", "crouch", "damaged"]:
        canvas.draw_rect(ox + 5, py_torso + 2, 6, 11, shirt_c)
        canvas.draw_rect(ox + 4, py_torso + 10, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 21, py_torso + 2, 6, 11, shirt_c)
        canvas.draw_rect(ox + 22, py_torso + 10, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
    elif pose == "shoot":
        canvas.draw_rect(ox + 21, py_torso + 1, 9, 6, shirt_c)
        canvas.draw_rect(ox + 28, py_torso, 4, 7, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 28, py_torso - 4, 8, 8, M_RED_HI)
        canvas.draw_rect(ox + 30, py_torso - 2, 4, 4, M_YELLOW)
    elif pose in ["skid", "slide"]:
        canvas.draw_rect(ox + 4, py_torso + 1, 6, 11, shirt_c)
        canvas.draw_rect(ox + 3, py_torso + 9, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 22, py_torso + 3, 6, 11, shirt_c)
        canvas.draw_rect(ox + 24, py_torso + 11, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        if pose == "skid":
            canvas.draw_rect(ox + 2, oy + 54, 5, 4, M_DUST)
            canvas.draw_rect(ox + 5, oy + 57, 5, 4, M_DUST)
    elif pose in ["walk", "run", "glide", "spin"]:
        if step == 0:
            canvas.draw_rect(ox + 3, py_torso + 1, 6, 11, shirt_c)
            canvas.draw_rect(ox + 2, py_torso + 8, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 23, py_torso + 4, 6, 11, shirt_c)
            canvas.draw_rect(ox + 24, py_torso + 12, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        elif step == 1:
            canvas.draw_rect(ox + 5, py_torso + 3, 6, 11, shirt_c)
            canvas.draw_rect(ox + 4, py_torso + 11, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 21, py_torso + 3, 6, 11, shirt_c)
            canvas.draw_rect(ox + 22, py_torso + 11, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        else:
            canvas.draw_rect(ox + 23, py_torso + 1, 6, 11, shirt_c)
            canvas.draw_rect(ox + 24, py_torso + 8, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 3, py_torso + 4, 6, 11, shirt_c)
            canvas.draw_rect(ox + 2, py_torso + 12, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
    elif pose in ["jump", "flutter", "double_jump", "swoop"]:
        canvas.draw_rect(ox + 20, py_torso - 14, 6, 15, shirt_c)
        canvas.draw_rect(ox + 20, py_torso - 20, 6, 7, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 4, py_torso + 3, 6, 10, shirt_c)
        canvas.draw_rect(ox + 3, py_torso + 10, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
    elif pose in ["fall", "wall_slide", "ground_pound", "swim", "climb"]:
        canvas.draw_rect(ox + 2, py_torso - 4, 7, 10, shirt_c)
        canvas.draw_rect(ox + 1, py_torso - 9, 5, 6, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 23, py_torso - 4, 7, 10, shirt_c)
        canvas.draw_rect(ox + 25, py_torso - 9, 5, 6, M_GLOVE_HI, M_GLOVE_SH)

    py_legs = py_torso + (12 if pose == "crouch" else 18)

    if pose in ["idle", "crouch", "shoot", "damaged"]:
        canvas.draw_rect(ox + 10, py_legs, 5, 11, overall_c)
        canvas.draw_rect(ox + 17, py_legs, 5, 11, overall_c)
        canvas.draw_rect(ox + 7, py_legs + 8, 8, 10, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 17, py_legs + 8, 8, 10, M_BOOT, M_BOOT_SH)
    elif pose in ["walk", "run", "glide", "spin"]:
        if step == 0:
            canvas.draw_rect(ox + 12, py_legs, 7, 10, overall_c)
            canvas.draw_rect(ox + 10, py_legs + 8, 11, 10, M_BOOT, M_BOOT_SH)
        elif step == 1:
            canvas.draw_rect(ox + 8, py_legs, 6, 10, overall_c)
            canvas.draw_rect(ox + 18, py_legs, 6, 10, overall_c)
            canvas.draw_rect(ox + 6, py_legs + 8, 8, 10, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_legs + 8, 8, 10, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 13, py_legs, 7, 10, overall_c)
            canvas.draw_rect(ox + 14, py_legs + 8, 12, 10, M_BOOT, M_BOOT_SH)
    elif pose in ["skid", "slide"]:
        canvas.draw_rect(ox + 8, py_legs, 7, 10, overall_c)
        canvas.draw_rect(ox + 18, py_legs, 6, 10, overall_c)
        canvas.draw_rect(ox + 5, py_legs + 8, 11, 10, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 18, py_legs + 8, 8, 10, M_BOOT, M_BOOT_SH)
    elif pose in ["jump", "wall_slide", "swoop"]:
        canvas.draw_rect(ox + 7, py_legs - 2, 7, 10, overall_c)
        canvas.draw_rect(ox + 18, py_legs + 3, 6, 10, overall_c)
        canvas.draw_rect(ox + 5, py_legs + 6, 9, 9, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 18, py_legs + 11, 9, 8, M_BOOT, M_BOOT_SH)
    elif pose == "flutter":
        if step == 0:
            canvas.draw_rect(ox + 7, py_legs - 2, 7, 10, overall_c)
            canvas.draw_rect(ox + 5, py_legs + 6, 9, 9, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_legs + 6, 6, 10, overall_c)
            canvas.draw_rect(ox + 18, py_legs + 14, 9, 8, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 7, py_legs + 6, 7, 10, overall_c)
            canvas.draw_rect(ox + 5, py_legs + 14, 9, 9, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 18, py_legs - 2, 6, 10, overall_c)
            canvas.draw_rect(ox + 18, py_legs + 6, 9, 8, M_BOOT, M_BOOT_SH)
    elif pose == "double_jump":
        canvas.draw_rect(ox + 7, py_legs - 4, 18, 14, overall_c)
        canvas.draw_rect(ox + 5, py_legs + 6, 10, 10, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 17, py_legs + 6, 10, 10, M_BOOT, M_BOOT_SH)
    elif pose in ["fall", "ground_pound", "swim", "climb"]:
        canvas.draw_rect(ox + 6, py_legs - 3, 7, 10, overall_c)
        canvas.draw_rect(ox + 19, py_legs - 3, 7, 10, overall_c)
        canvas.draw_rect(ox + 4, py_legs + 5, 9, 9, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 19, py_legs + 5, 9, 9, M_BOOT, M_BOOT_SH)


def draw_mini_mario_detailed(canvas, ox, oy, pose="idle", step=0, shirt_c=M_RED, overall_c=M_BLUE):
    py_cap = oy + (3 if pose == "damaged" else 1)
    canvas.draw_rect(ox + 4, py_cap, 8, 3, shirt_c, M_RED_SH if shirt_c == M_RED else L_GREEN_SH)
    canvas.draw_rect(ox + 5, py_cap, 5, 1, M_RED_HI if shirt_c == M_RED else L_GREEN_HI)
    canvas.draw_rect(ox + 8, py_cap + 2, 4, 1, shirt_c)

    py_face = py_cap + 3
    canvas.draw_rect(ox + 4, py_face, 8, 4, M_SKIN)
    if pose == "damaged":
        canvas.set_pixel(ox + 8, py_face + 1, C_BLACK)
    elif pose == "death":
        canvas.draw_rect(ox + 5, py_face + 1, 2, 2, C_BLACK)
        canvas.draw_rect(ox + 9, py_face + 1, 2, 2, C_BLACK)
    else:
        canvas.draw_rect(ox + 7, py_face + 1, 2, 2, C_BLACK)
        canvas.set_pixel(ox + 7, py_face + 1, M_WHITE)
    canvas.draw_rect(ox + 6, py_face + 3, 4, 2, M_BOOT)

    py_torso = py_face + 4
    canvas.draw_rect(ox + 4, py_torso, 8, 4, overall_c)
    canvas.draw_rect(ox + 5, py_torso + 1, 1, 1, M_YELLOW)
    canvas.draw_rect(ox + 8, py_torso + 1, 1, 1, M_YELLOW)

    if pose == "jump":
        canvas.draw_rect(ox + 9, py_torso - 3, 2, 4, shirt_c)
        canvas.draw_rect(ox + 9, py_torso - 5, 2, 2, M_WHITE)
        canvas.draw_rect(ox + 2, py_torso, 2, 3, shirt_c)
    elif pose == "fall":
        canvas.draw_rect(ox + 1, py_torso, 3, 2, shirt_c)
        canvas.draw_rect(ox + 12, py_torso, 3, 2, shirt_c)
    elif pose == "walk":
        if step == 0:
            canvas.draw_rect(ox + 2, py_torso + 1, 2, 3, shirt_c)
            canvas.draw_rect(ox + 10, py_torso + 2, 2, 3, shirt_c)
        else:
            canvas.draw_rect(ox + 10, py_torso + 1, 2, 3, shirt_c)
            canvas.draw_rect(ox + 2, py_torso + 2, 2, 3, shirt_c)
    else:
        canvas.draw_rect(ox + 2, py_torso + 1, 2, 3, shirt_c)
        canvas.draw_rect(ox + 12, py_torso + 1, 2, 3, shirt_c)

    py_leg = py_torso + 3
    if pose == "walk":
        if step == 0:
            canvas.draw_rect(ox + 3, py_leg + 1, 4, 3, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 9, py_leg + 1, 3, 3, M_BOOT, M_BOOT_SH)
        else:
            canvas.draw_rect(ox + 4, py_leg + 1, 3, 3, M_BOOT, M_BOOT_SH)
            canvas.draw_rect(ox + 9, py_leg + 1, 4, 3, M_BOOT, M_BOOT_SH)
    else:
        canvas.draw_rect(ox + 3, py_leg + 1, 3, 3, M_BOOT, M_BOOT_SH)
        canvas.draw_rect(ox + 10, py_leg + 1, 3, 3, M_BOOT, M_BOOT_SH)


def draw_mega_mario_detailed(canvas, ox, oy, pose="idle", step=0, shirt_c=M_RED, overall_c=M_BLUE):
    py_cap = oy + 12
    canvas.draw_rect(ox + 36, py_cap, 56, 24, shirt_c, M_RED_SH if shirt_c == M_RED else L_GREEN_SH)
    canvas.draw_rect(ox + 44, py_cap + 4, 40, 6, M_RED_HI if shirt_c == M_RED else L_GREEN_HI)
    canvas.draw_rect(ox + 72, py_cap + 12, 32, 10, shirt_c)

    py_face = py_cap + 24
    canvas.draw_rect(ox + 28, py_face + 8, 16, 32, M_BOOT)
    canvas.draw_rect(ox + 40, py_face, 48, 44, M_SKIN)
    canvas.draw_rect(ox + 44, py_face, 36, 6, M_SKIN_HI)
    canvas.draw_rect(ox + 68, py_face + 8, 12, 16, C_BLACK)
    canvas.set_pixel(ox + 68, py_face + 8, M_WHITE)
    canvas.set_pixel(ox + 69, py_face + 9, M_WHITE)
    canvas.draw_rect(ox + 76, py_face + 12, 16, 12, M_SKIN)
    canvas.draw_rect(ox + 78, py_face + 12, 10, 4, M_SKIN_HI)
    canvas.draw_rect(ox + 60, py_face + 24, 36, 16, M_BOOT, M_BOOT_SH)

    py_torso = py_face + 40
    canvas.draw_rect(ox + 36, py_torso, 56, 32, shirt_c)
    canvas.draw_rect(ox + 44, py_torso + 4, 40, 32, overall_c)
    canvas.draw_rect(ox + 44, py_torso + 12, 8, 10, M_YELLOW)
    canvas.draw_rect(ox + 76, py_torso + 12, 8, 10, M_YELLOW)

    if pose in ["jump", "flutter"]:
        canvas.draw_rect(ox + 88, py_torso - 36, 24, 40, shirt_c)
        canvas.draw_rect(ox + 84, py_torso - 56, 28, 24, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 20, py_torso + 4, 20, 24, shirt_c)
        canvas.draw_rect(ox + 16, py_torso + 16, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
    elif pose == "walk":
        if step == 0:
            canvas.draw_rect(ox + 16, py_torso + 2, 24, 24, shirt_c)
            canvas.draw_rect(ox + 12, py_torso + 14, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 88, py_torso + 10, 24, 24, shirt_c)
            canvas.draw_rect(ox + 92, py_torso + 24, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
        else:
            canvas.draw_rect(ox + 88, py_torso + 2, 24, 24, shirt_c)
            canvas.draw_rect(ox + 92, py_torso + 14, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
            canvas.draw_rect(ox + 16, py_torso + 10, 24, 24, shirt_c)
            canvas.draw_rect(ox + 12, py_torso + 24, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
    else:
        canvas.draw_rect(ox + 20, py_torso + 4, 20, 24, shirt_c)
        canvas.draw_rect(ox + 16, py_torso + 20, 20, 20, M_GLOVE_HI, M_GLOVE_SH)
        canvas.draw_rect(ox + 88, py_torso + 4, 20, 24, shirt_c)
        canvas.draw_rect(ox + 92, py_torso + 20, 20, 20, M_GLOVE_HI, M_GLOVE_SH)

    py_legs = py_torso + 32
    canvas.draw_rect(ox + 40, py_legs, 16, 12, overall_c)
    canvas.draw_rect(ox + 72, py_legs, 16, 12, overall_c)
    canvas.draw_rect(ox + 28, py_legs + 4, 32, 16, M_BOOT, M_BOOT_SH)
    canvas.draw_rect(ox + 68, py_legs + 4, 32, 16, M_BOOT, M_BOOT_SH)
    canvas.draw_rect(ox + 32, py_legs + 4, 24, 3, M_BOOT_HI)
    canvas.draw_rect(ox + 72, py_legs + 4, 24, 3, M_BOOT_HI)


# =========================================================================
# SPRITE GENERATION PIPELINE
# =========================================================================
def generate_mario_sprites(canvas):
    # 1. SMALL MARIO (32x32 per frame) -> Row 0 (y = 0..31)
    small_poses = [
        ("idle", 0), ("walk", 0), ("walk", 1), ("walk", 2),
        ("run", 0), ("run", 1), ("run", 2), ("jump", 0),
        ("fall", 0), ("crouch", 0), ("slide", 0), ("wall_slide", 0),
        ("ground_pound", 0), ("ground_pound", 1), ("swim", 0), ("swim", 1),
        ("climb", 0), ("climb", 1), ("skid", 0), ("damaged", 0), ("death", 0)
    ]
    small_frames_map = {}
    for idx, (pose, step) in enumerate(small_poses):
        ox = idx * 32
        oy = 0
        draw_small_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_RED, overall_c=M_BLUE)
        frame_name = f"mario_small_{pose}_{step}" if pose in ["walk", "run", "ground_pound", "swim", "climb"] else f"mario_small_{pose}"
        small_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 32}

    # 2. SUPER MARIO (32x64 per frame) -> Row 1 (y = 32..95)
    super_poses = [
        ("idle", 0), ("walk", 0), ("walk", 1), ("walk", 2),
        ("run", 0), ("run", 1), ("run", 2), ("jump", 0),
        ("fall", 0), ("crouch", 0), ("slide", 0), ("wall_slide", 0),
        ("ground_pound", 0), ("swim", 0), ("climb", 0), ("skid", 0), ("damaged", 0)
    ]
    super_frames_map = {}
    for idx, (pose, step) in enumerate(super_poses):
        ox = idx * 32
        oy = 32
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_RED, overall_c=M_BLUE, has_cape=False)
        frame_name = f"mario_super_{pose}_{step}" if pose in ["walk", "run"] else f"mario_super_{pose}"
        super_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 3. FIRE MARIO (32x64 per frame) -> Row 2 (y = 96..159)
    fire_poses = super_poses + [("shoot", 0)]
    fire_frames_map = {}
    for idx, (pose, step) in enumerate(fire_poses):
        ox = idx * 32
        oy = 96
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_WHITE, overall_c=M_RED, has_cape=False)
        frame_name = f"mario_fire_{pose}_{step}" if pose in ["walk", "run"] else f"mario_fire_{pose}"
        fire_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 4. CAPE MARIO (32x64 per frame) -> Row 3 (y = 160..223)
    cape_poses = super_poses + [("glide", 0), ("glide", 1), ("swoop", 0), ("spin", 0), ("spin", 1), ("spin", 2)]
    cape_frames_map = {}
    for idx, (pose, step) in enumerate(cape_poses):
        ox = idx * 32
        oy = 160
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_RED, overall_c=M_BLUE, has_cape=True)
        frame_name = f"mario_cape_{pose}_{step}" if pose in ["walk", "run", "glide", "spin"] else f"mario_cape_{pose}"
        cape_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 5. MINI MARIO (16x16 per frame) -> Row 4 (y = 224..239)
    mini_poses = [("idle", 0), ("walk", 0), ("walk", 1), ("jump", 0), ("fall", 0), ("damaged", 0), ("death", 0)]
    mini_frames_map = {}
    for idx, (pose, step) in enumerate(mini_poses):
        ox = idx * 16
        oy = 224
        draw_mini_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_RED, overall_c=M_BLUE)
        frame_name = f"mario_mini_{pose}_{step}" if pose == "walk" else f"mario_mini_{pose}"
        mini_frames_map[frame_name] = {"x": ox, "y": oy, "w": 16, "h": 16}

    # 6. MEGA MARIO (128x128 per frame) -> Row 5 (y = 240..367)
    mega_poses = [("idle", 0), ("walk", 0), ("walk", 1), ("jump", 0)]
    mega_frames_map = {}
    for idx, (pose, step) in enumerate(mega_poses):
        ox = idx * 128
        oy = 240
        draw_mega_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_RED, overall_c=M_BLUE)
        frame_name = f"mario_mega_{pose}_{step}" if pose == "walk" else f"mario_mega_{pose}"
        mega_frames_map[frame_name] = {"x": ox, "y": oy, "w": 128, "h": 128}

    all_mario = {}
    all_mario.update(small_frames_map)
    all_mario.update(super_frames_map)
    all_mario.update(fire_frames_map)
    all_mario.update(cape_frames_map)
    all_mario.update(mini_frames_map)
    all_mario.update(mega_frames_map)
    return all_mario


def generate_luigi_sprites(canvas):
    # 1. SMALL LUIGI (32x32 per frame) -> Row 6 (y = 368..399)
    small_poses = [
        ("idle", 0), ("walk", 0), ("walk", 1), ("walk", 2),
        ("run", 0), ("run", 1), ("run", 2), ("jump", 0),
        ("flutter", 0), ("flutter", 1), ("double_jump", 0),
        ("fall", 0), ("crouch", 0), ("slide", 0), ("wall_slide", 0),
        ("ground_pound", 0), ("ground_pound", 1), ("swim", 0), ("swim", 1),
        ("climb", 0), ("climb", 1), ("skid", 0), ("damaged", 0), ("death", 0)
    ]
    small_frames_map = {}
    for idx, (pose, step) in enumerate(small_poses):
        ox = idx * 32
        oy = 368
        draw_small_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=L_GREEN, overall_c=L_BLUE)
        frame_name = f"luigi_small_{pose}_{step}" if pose in ["walk", "run", "flutter", "ground_pound", "swim", "climb"] else f"luigi_small_{pose}"
        small_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 32}

    # 2. SUPER LUIGI (32x64 per frame) -> Row 7 (y = 400..463)
    super_poses = [
        ("idle", 0), ("walk", 0), ("walk", 1), ("walk", 2),
        ("run", 0), ("run", 1), ("run", 2), ("jump", 0),
        ("flutter", 0), ("flutter", 1), ("double_jump", 0),
        ("fall", 0), ("crouch", 0), ("slide", 0), ("wall_slide", 0),
        ("ground_pound", 0), ("swim", 0), ("climb", 0), ("skid", 0), ("damaged", 0)
    ]
    super_frames_map = {}
    for idx, (pose, step) in enumerate(super_poses):
        ox = idx * 32
        oy = 400
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=L_GREEN, overall_c=L_BLUE, has_cape=False)
        frame_name = f"luigi_super_{pose}_{step}" if pose in ["walk", "run", "flutter"] else f"luigi_super_{pose}"
        super_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 3. FIRE LUIGI (32x64 per frame) -> Row 8 (y = 464..527)
    fire_poses = super_poses + [("shoot", 0)]
    fire_frames_map = {}
    for idx, (pose, step) in enumerate(fire_poses):
        ox = idx * 32
        oy = 464
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=M_WHITE, overall_c=L_GREEN, has_cape=False)
        frame_name = f"luigi_fire_{pose}_{step}" if pose in ["walk", "run", "flutter"] else f"luigi_fire_{pose}"
        fire_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 4. CAPE LUIGI (32x64 per frame) -> Row 9 (y = 528..591)
    cape_poses = super_poses + [("glide", 0), ("glide", 1), ("swoop", 0), ("spin", 0), ("spin", 1), ("spin", 2)]
    cape_frames_map = {}
    for idx, (pose, step) in enumerate(cape_poses):
        ox = idx * 32
        oy = 528
        draw_super_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=L_GREEN, overall_c=L_BLUE, has_cape=True)
        frame_name = f"luigi_cape_{pose}_{step}" if pose in ["walk", "run", "flutter", "glide", "spin"] else f"luigi_cape_{pose}"
        cape_frames_map[frame_name] = {"x": ox, "y": oy, "w": 32, "h": 64}

    # 5. MINI LUIGI (16x16 per frame) -> Row 10 (y = 592..607)
    mini_poses = [("idle", 0), ("walk", 0), ("walk", 1), ("jump", 0), ("flutter", 0), ("fall", 0), ("damaged", 0), ("death", 0)]
    mini_frames_map = {}
    for idx, (pose, step) in enumerate(mini_poses):
        ox = idx * 16
        oy = 592
        draw_mini_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=L_GREEN, overall_c=L_BLUE)
        frame_name = f"luigi_mini_{pose}_{step}" if pose == "walk" else f"luigi_mini_{pose}"
        mini_frames_map[frame_name] = {"x": ox, "y": oy, "w": 16, "h": 16}

    # 6. MEGA LUIGI (128x128 per frame) -> Row 11 (y = 608..735)
    mega_poses = [("idle", 0), ("walk", 0), ("walk", 1), ("jump", 0), ("flutter", 0)]
    mega_frames_map = {}
    for idx, (pose, step) in enumerate(mega_poses):
        ox = idx * 128
        oy = 608
        draw_mega_mario_detailed(canvas, ox, oy, pose=pose, step=step, shirt_c=L_GREEN, overall_c=L_BLUE)
        frame_name = f"luigi_mega_{pose}_{step}" if pose in ["walk", "flutter"] else f"luigi_mega_{pose}"
        mega_frames_map[frame_name] = {"x": ox, "y": oy, "w": 128, "h": 128}

    all_luigi = {}
    all_luigi.update(small_frames_map)
    all_luigi.update(super_frames_map)
    all_luigi.update(fire_frames_map)
    all_luigi.update(cape_frames_map)
    all_luigi.update(mini_frames_map)
    all_luigi.update(mega_frames_map)
    return all_luigi


def main():
    width, height = 1024, 1024
    canvas = PlayerCanvas(width, height)

    print("Generating complete Mario and Luigi animation sets...")
    mario_frames = generate_mario_sprites(canvas)
    luigi_frames = generate_luigi_sprites(canvas)

    all_frames = {}
    all_frames.update(mario_frames)
    all_frames.update(luigi_frames)

    frames_json = {}
    for frame_name, rect in all_frames.items():
        frames_json[frame_name] = {
            "frame": {"x": rect["x"], "y": rect["y"], "w": rect["w"], "h": rect["h"]},
            "rotated": False,
            "trimmed": False,
            "spriteSourceSize": {"x": 0, "y": 0, "w": rect["w"], "h": rect["h"]},
            "sourceSize": {"w": rect["w"], "h": rect["h"]}
        }

    players_meta = {
        "frames": frames_json,
        "meta": {
            "app": "Sprite Extractor Agent",
            "version": "2.0",
            "image": "players.png",
            "format": "RGBA8888",
            "size": {"w": width, "h": height},
            "scale": "1",
            "completedCharacters": ["Mario", "Luigi"]
        }
    }

    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.abspath(os.path.join(script_dir, ".."))
    
    png_path = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "player", "players.png")
    json_path = os.path.join(root_dir, "SuperMarioGame", "assets", "spriteSheet", "player", "players.json")

    canvas.save_png(png_path)
    
    os.makedirs(os.path.dirname(os.path.abspath(json_path)), exist_ok=True)
    with open(json_path, 'w') as f:
        json.dump(players_meta, f, indent=2)
    print(f"Saved complete players metadata JSON ({len(all_frames)} frames) to {json_path}")

if __name__ == "__main__":
    main()
