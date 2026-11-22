#!/usr/bin/env python3
"""Create a draft, upload both verified artifacts, then publish and update latest."""
import json
import os
from pathlib import Path
import subprocess

def gh(*args):return subprocess.check_output(['gh',*args],text=True)
def api(method,path,payload=None):
    args=['api','--method',method,path]
    if payload is not None:args+=['--input','-']
    data=subprocess.check_output(['gh',*args],input=json.dumps(payload) if payload is not None else None,text=True)
    return json.loads(data) if data.strip() else None

def main():
    repo=os.environ['GITHUB_REPOSITORY'];version=os.environ['RELEASE_VERSION'];tag='v'+version;commit=os.environ['GITHUB_SHA']
    found=subprocess.run(['gh','release','view',tag,'--repo',repo,'--json','isDraft,tagName'],capture_output=True,text=True)
    if found.returncode==0:
        previous=json.loads(gh('api',f'repos/{repo}/releases/tags/{tag}'))
        if previous['target_commitish']!=commit or not json.loads(found.stdout)['isDraft']:
            ref=json.loads(gh('api',f'repos/{repo}/git/ref/tags/{tag}'))['object']
            while ref['type']=='tag':ref=json.loads(gh('api',f'repos/{repo}/git/tags/{ref["sha"]}'))['object']
            if ref['sha']!=commit:raise RuntimeError('VERSION tag already belongs to a different commit')
        if not json.loads(found.stdout)['isDraft']:
            print('This exact version/commit is already published; leaving its artifacts unchanged.');return
    else:
        notes=('ACT3 '+version+' — personal restoration by Michalis, not an official group release.\n\n'
            'One ZIP contains Windows x64 and Linux x86_64 players, shared assets and the original 2004 executable.\n'
            'Default: Party version, native fullscreen, original soundtrack.\n\n'
            'Download ACT3.zip, extract it, run ACT3.exe or ./ACT3-linux-x86_64.\n'
            'Linux targets glibc 2.31+ with system OpenGL/X11; helper libraries are included.\n\n'
            'Permanent download: https://github.com/'+repo+'/releases/latest/download/ACT3.zip\n')
        Path('build').mkdir(exist_ok=True);Path('build/release-notes.md').write_text(notes,encoding='utf-8')
        gh('release','create',tag,'--repo',repo,'--target',commit,'--title','ACT3 '+version,'--draft','--notes-file','build/release-notes.md')
    gh('release','upload',tag,'dist/ACT3.zip','dist/ACT3.zip.sha256','--repo',repo,'--clobber')
    release=json.loads(gh('api',f'repos/{repo}/releases/tags/{tag}'))
    # Legacy selection uses semantic version ordering, avoiding an older queued bump
    # stealing latest merely because its build finishes later.
    api('PATCH',f'repos/{repo}/releases/{release["id"]}',{'draft':False,'prerelease':False,'make_latest':'legacy'})
    print('Published',release['html_url'])
if __name__=='__main__':main()
