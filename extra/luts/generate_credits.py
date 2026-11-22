"""Electric violet / cyan Credits grade. Black and white remain anchored."""
from pathlib import Path
import struct
import zlib


def grade(r, g, b):
    y = .2126*r + .7152*g + .0722*b
    rgb = [max(0, min(1, y+1.16*(c-y))) for c in (r, g, b)]
    rgb = [c+.32*c*(1-c)*(2*c-1) for c in rgb]
    shadow = y*(1-y)**3
    green = max(0, g-max(r, b))
    blue = max(0, b-max(r, g))
    rgb[0] += .22*shadow + .20*blue - .12*green
    rgb[1] += -.07*shadow + .03*green
    rgb[2] += .46*shadow + .32*green
    return bytes(round(255*max(0, min(1, c))) for c in rgb)


def chunk(tag, data):
    return struct.pack('>I', len(data))+tag+data+struct.pack('>I', zlib.crc32(tag+data))


pixels = b''.join(b'\0'+b''.join(grade(r/15, g/15, b/15) for b in range(16))
                  for r in range(16) for g in range(16))
Path(__file__).with_name('credits.png').write_bytes(
    b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB', 16, 256, 8, 2, 0, 0, 0))
    +chunk(b'IDAT', zlib.compress(pixels, 9))+chunk(b'IEND', b''))
