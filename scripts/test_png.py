import os
import sys
import zlib
import struct

def create_png(width, height, rgba_data):
    """
    Creates a PNG file in memory using Python standard library (zlib, struct).
    rgba_data: bytes of length width * height * 4
    Returns bytes of complete PNG file.
    """
    # PNG signature
    png_sig = b'\x89PNG\r\n\x1a\n'
    
    # IHDR chunk
    # Width (4 bytes), Height (4 bytes), Bit depth (1 byte: 8), Color type (1 byte: 6 = RGBA),
    # Compression (1 byte: 0), Filter (1 byte: 0), Interlace (1 byte: 0)
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    ihdr_chunk = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)
    
    # IDAT chunk
    # Build raw scanlines (each row starts with filter type byte 0x00)
    raw_bytes = bytearray()
    row_bytes_len = width * 4
    for y in range(height):
        raw_bytes.append(0) # Filter type 0 (None)
        start = y * row_bytes_len
        raw_bytes.extend(rgba_data[start : start + row_bytes_len])
        
    compressed_data = zlib.compress(raw_bytes, level=9)
    idat_crc = zlib.crc32(b'IDAT' + compressed_data)
    idat_chunk = struct.pack('>I', len(compressed_data)) + b'IDAT' + compressed_data + struct.pack('>I', idat_crc)
    
    # IEND chunk
    iend_crc = zlib.crc32(b'IEND')
    iend_chunk = struct.pack('>I', 0) + b'IEND' + struct.pack('>I', iend_crc)
    
    return png_sig + ihdr_chunk + idat_chunk + iend_chunk


# Test PNG generation
if __name__ == '__main__':
    w, h = 32, 32
    pixels = bytearray(w * h * 4)
    # Fill with semi-transparent red
    for i in range(w * h):
        pixels[i*4] = 255     # R
        pixels[i*4+1] = 0     # G
        pixels[i*4+2] = 0     # B
        pixels[i*4+3] = 200   # A
    png_data = create_png(w, h, pixels)
    print(f"Generated PNG test of size {len(png_data)} bytes.")
