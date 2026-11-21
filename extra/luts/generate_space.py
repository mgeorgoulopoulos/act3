"""Cosmos-inspired Space LUT; standard library only, RGB16 strip layout."""
from pathlib import Path
import struct
import zlib


def grade(r, g, b):
    luminance = 0.2126*r + 0.7152*g + 0.0722*b
    # Richer film contrast and chroma; black and stellar white stay anchored.
    contrast = lambda x: x + 0.28*x*(1-x)*(2*x-1)
    saturation = 1.12
    rgb = [contrast(max(0, min(1, luminance+(c-luminance)*saturation)))
           for c in (r, g, b)]
    shadow = luminance * (1-luminance)**3
    warm = max(0, min(r, g)-b) * (1-luminance)
    # Indigo shadows against amber metal evoke the original Cosmos palette.
    highlight = luminance**2 * (1-luminance)
    rgb[0] += -0.16*shadow + 0.22*warm + 0.055*highlight
    rgb[1] += 0.005*shadow + 0.035*warm + 0.015*highlight
    rgb[2] += 0.40*shadow - 0.14*warm - 0.025*highlight
    return bytes(round(255*max(0, min(1, c))) for c in rgb)


def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag+data))


pixels = b''.join(b'\0' + b''.join(grade(r/15, g/15, b/15) for b in range(16))
                  for r in range(16) for g in range(16))
png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 16, 256, 8, 2, 0, 0, 0))
       + chunk(b'IDAT', zlib.compress(pixels, 9)) + chunk(b'IEND', b''))
Path(__file__).with_name('space.png').write_bytes(png)
