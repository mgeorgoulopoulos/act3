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


import ctypes
import threading

def flood_window_events(stop, started, state):
    # Use a separate X connection. Client messages model sustained desktop traffic.
    x11 = ctypes.CDLL('libX11.so.6')
    x11.XOpenDisplay.restype = ctypes.c_void_p
    x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
    x11.XInternAtom.restype = ctypes.c_ulong
    x11.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    x11.XSendEvent.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_long, ctypes.c_void_p]
    x11.XFlush.argtypes = [ctypes.c_void_p]
    x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
    class Message(ctypes.Structure):
        _fields_ = [('type', ctypes.c_int), ('serial', ctypes.c_ulong),
                    ('send_event', ctypes.c_int), ('display', ctypes.c_void_p),
                    ('window', ctypes.c_ulong), ('message_type', ctypes.c_ulong),
                    ('format', ctypes.c_int), ('data', ctypes.c_long * 5)]
    class Event(ctypes.Union):
        _fields_ = [('message', Message), ('padding', ctypes.c_long * 24)]
    display = x11.XOpenDisplay(None)
    if not display:
        state['error'] = 'Could not open the X11 event injector connection'
        return
    try:
        while not stop.is_set() and time.monotonic() - started < 3:
            stop.wait(0.05)
        windows = subprocess.check_output(['xdotool', 'search', '--onlyvisible', '--name', 'ACT3'], timeout=5)
        window = int(windows.splitlines()[0])
        event = Event()
        event.message.type = 33  # ClientMessage
        event.message.display = display
        event.message.window = window
        event.message.message_type = x11.XInternAtom(display, b'ACT3_CI_EVENT', False)
        event.message.format = 32
        while not stop.is_set() and time.monotonic() - started < 19:
            x11.XSendEvent(display, window, False, 0, ctypes.byref(event))
            x11.XFlush(display)
            state['sent'] += 1
            stop.wait(0.001)
    except Exception as error:
        state['error'] = str(error)
    finally:
        x11.XCloseDisplay(display)

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
        stop = threading.Event()
        flood_state = {'sent': 0, 'error': None}
        flood = threading.Thread(target=flood_window_events, args=(stop, started, flood_state), daemon=True)
        flood.start()
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
                if elapsed >= 20:
                    if flood_state['error'] or flood_state['sent'] < 1000:
                        raise RuntimeError(f'X11 event injection failed: {flood_state}')
                    during_events = [s[2] for s in samples if 5 <= s[0] <= 18]
                    if len(set(during_events)) < 2:
                        raise RuntimeError('Rendering froze while X11 events were arriving')
            elapsed = time.monotonic() - started
            if process.returncode != 0 or not 210 <= elapsed <= 300:
                raise RuntimeError(f'Unexpected exit {process.returncode} after {elapsed:.1f}s')
            for name, (begin, end) in PARTS.items():
                frames = [s for s in samples if begin <= s[0] <= end]
                visible = [s for s in frames if s[3] > 2 and s[4] > 1]
                if len({s[2] for s in visible}) < 2:
                    raise RuntimeError(f'{name}: missing, blank or frozen frames')
        finally:
            stop.set()
            flood.join(timeout=6)
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
