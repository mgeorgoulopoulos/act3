#!/usr/bin/env python3
"""Exercise the real Xlib launcher under Xvfb and capture its application window."""
import argparse
import os
from pathlib import Path
import subprocess
import time
from PIL import Image


def command(*args):
    return subprocess.check_output(['xdotool', *map(str,args)], text=True, timeout=10).strip()

def wait_window(pattern, process):
    deadline=time.monotonic()+45
    while time.monotonic()<deadline:
        if process.poll() is not None:
            raise RuntimeError(f'Player exited before window appeared: {process.returncode}')
        result=subprocess.run(['xdotool','search','--onlyvisible','--name',pattern],capture_output=True,text=True,timeout=5)
        if result.returncode==0:return result.stdout.splitlines()[0]
        time.sleep(.1)
    raise RuntimeError('Timed out waiting for '+pattern)

def stop(process):
    if process.poll() is None:
        process.terminate()
        try:process.wait(timeout=10)
        except subprocess.TimeoutExpired:process.kill();process.wait()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();binary=str(args.binary.resolve());args.output.mkdir(parents=True,exist_ok=True)
    # Help and diagnostics remain available without an X server.
    env=dict(os.environ);env.pop('DISPLAY',None)
    for option in ['--help','--check-assets']:
        subprocess.run([binary,option],env=env,cwd='/tmp',check=True,timeout=15,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    with (args.output/'accepted.log').open('w') as log:
        process=subprocess.Popen([binary],cwd='/tmp',stdout=log,stderr=log)
        try:
            window=wait_window('^ACT3 / Reversed Engineers$',process)
            command('windowfocus',window)
            geometry=command('getwindowgeometry','--shell',window)
            values=dict(line.split('=',1) for line in geometry.splitlines())
            scale=int(values['WIDTH'])/760
            def click(x,y):
                command('mousemove','--window',window,int(x*scale),int(y*scale));command('click',1);time.sleep(.15)
            def shot(name):
                time.sleep(.15)
                subprocess.run(['import','-window',window,str(args.output/name)],check=True,timeout=15)
            shot('launcher-default.png')
            with Image.open(args.output/'launcher-default.png') as image:
                rgb=image.convert('RGB')
                if rgb.getpixel((int(32*scale),int(240*scale)))!=(223,54,74):
                    raise RuntimeError('Bare launch did not select Party version')
            click(555,282)
            shot('launcher-anniversary.png')
            click(555,402)
            shot('launcher-display-menu.png')
            command('key','Home','Return');time.sleep(.1)  # Windowed.
            click(200,402)
            shot('launcher-resolution-menu.png')
            command('key','Home','Return');time.sleep(.1)  # 800 x 600.
            shot('launcher-windowed.png')
            # Resolution -> Display -> Exit -> Go. Exercise keyboard traversal.
            command('key','Tab','Tab','Tab','Return')
            player=wait_window('Full Demo',process)
            deadline=time.monotonic()+45
            while time.monotonic()<deadline:
                if process.poll() is not None:raise RuntimeError('Player exited during launcher handoff')
                protocols=subprocess.run(['xprop','-id',player,'WM_PROTOCOLS'],capture_output=True,text=True,timeout=5)
                if 'WM_DELETE_WINDOW' in protocols.stdout:break  # Installed after scene/audio loading.
                time.sleep(.1)
            else:raise RuntimeError('Player did not finish initialization')
            time.sleep(2)
            if process.poll() is not None:raise RuntimeError('Retired launcher events stopped the player')
            subprocess.run(['import','-window',player,str(args.output/'launcher-started-player.png')],check=True,timeout=15)
            command('windowfocus',player);command('key','Escape')
            process.wait(timeout=45)
            if process.returncode!=0:raise RuntimeError('Player failed after launcher acceptance')
        finally:stop(process)
    accepted=(args.output/'accepted.log').read_text(errors='replace')
    if '22nd anniversary edition / 16:9' not in accepted or '800 x 600 / windowed' not in accepted:
        raise RuntimeError('Launcher did not pass the chosen edition/resolution/display mode to the player:\n'+accepted)
    with (args.output/'cancelled.log').open('w') as log:
        process=subprocess.Popen([binary,'--ui','--extra','--windowed','--resolution=800x600'],cwd='/tmp',stdout=log,stderr=log)
        try:
            window=wait_window('^ACT3 / Reversed Engineers$',process)
            subprocess.run(['import','-window',window,str(args.output/'launcher-preset.png')],check=True,timeout=15)
            command('windowfocus',window);command('key','Escape');process.wait(timeout=10)
            if process.returncode!=0:raise RuntimeError('Cancel did not exit cleanly')
        finally:stop(process)
    if 'miniaudio' in (args.output/'cancelled.log').read_text(errors='replace'):
        raise RuntimeError('Cancelling the launcher initialized audio')
    print('PASS Xlib launcher: default Party/native/fullscreen capture; mouse and keyboard selection; windowed 800x600 anniversary playback; clean cancellation; headless CLI diagnostics.')

if __name__=='__main__':main()
