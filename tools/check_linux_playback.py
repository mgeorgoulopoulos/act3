#!/usr/bin/env python3
"""Exercise the real player with audio and validate changing frames in every part.

Run inside xvfb-run with a working PulseAudio sink. This is an integration smoke
check, not a pixel-fidelity or physical GPU/audio hardware test.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import time
from PIL import Image, ImageDraw, ImageStat

PARTS = {'space': (15, 60), 'shadow': (80, 120), 'eggplants': (135, 160),
         'greetz': (172, 185), 'credits': (195, 210)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--edition', choices=['original', 'extra'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    samples = []
    started = time.monotonic()
    with (args.output / 'player.log').open('w') as log:
        process = subprocess.Popen([str(args.binary.resolve()), '--' + args.edition,
                                    '--fullscreen'], cwd='/tmp', stdout=log, stderr=log)
        try:
            while process.poll() is None:
                if time.monotonic() - started > 300:
                    raise RuntimeError('Full playback did not finish within five minutes')
                time.sleep(5)
                if process.poll() is not None:
                    break
                elapsed = time.monotonic() - started
                capture = args.output / 'frame.png'
                subprocess.run(['import', '-window', 'root', str(capture)],
                               check=True, timeout=15)
                with Image.open(capture) as source:
                    image = source.convert('RGB')
                image.thumbnail((320, 240))
                stats = ImageStat.Stat(image)
                samples.append((elapsed, image, hashlib.sha256(image.tobytes()).digest(),
                                max(stats.stddev), max(stats.mean)))
                image.save(args.output / f'{elapsed:06.1f}.jpg', quality=85)
                capture.unlink()
            elapsed = time.monotonic() - started
            if process.returncode != 0 or not 210 <= elapsed <= 300:
                raise RuntimeError(f'Unexpected exit {process.returncode} after {elapsed:.1f}s')
            for name, (begin, end) in PARTS.items():
                frames = [s for s in samples if begin <= s[0] <= end]
                visible = [s for s in frames if s[3] > 2 and s[4] > 1]
                if len({s[2] for s in visible}) < 2:
                    raise RuntimeError(f'{name}: missing, blank or frozen frames')
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            if samples:
                sheet = Image.new('RGB', (1280, 260 * ((len(samples) + 3) // 4)), '#161616')
                draw = ImageDraw.Draw(sheet)
                for index, (elapsed, image, *_) in enumerate(samples):
                    x, y = index % 4 * 320, index // 4 * 260
                    sheet.paste(image, (x, y + 20))
                    draw.text((x + 5, y + 3), f'{args.edition}: {elapsed:.1f}s', fill='white')
                sheet.save(args.output / 'contact.jpg', quality=85)
    print(f'Full {args.edition} playback passed: all five parts, {elapsed:.1f}s, natural exit.')

if __name__ == '__main__':
    main()
