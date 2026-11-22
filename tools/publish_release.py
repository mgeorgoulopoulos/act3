#!/usr/bin/env python3
"""Publish only complete, checksum-verified releases; drafts can be safely retried."""
import hashlib
import json
import os
from pathlib import Path
import urllib.parse
import urllib.request

def api(path,method='GET',data=None,raw=None,content_type='application/json'):
    url=path if path.startswith('https://') else 'https://api.github.com/'+path
    if urllib.parse.urlsplit(url).hostname not in {'api.github.com','uploads.github.com'}:raise ValueError('Unexpected GitHub API host')
    headers={'Authorization':'Bearer '+os.environ['GH_TOKEN'],'Accept':'application/vnd.github+json',
             'User-Agent':'ACT3-release','X-GitHub-Api-Version':'2022-11-28','Content-Type':content_type}
    body=raw if raw is not None else json.dumps(data).encode() if data is not None else None
    request=urllib.request.Request(url,headers=headers,method=method,data=body)
    with urllib.request.urlopen(request,timeout=120) as response:
        result=response.read();return json.loads(result) if result else None

def find_release(repo,tag):
    page=1
    while True:
        releases=api(f'repos/{repo}/releases?per_page=100&page={page}')
        for release in releases:
            if release['tag_name']==tag:return release
        if len(releases)<100:return None
        page+=1

def main():
    repo=os.environ['GITHUB_REPOSITORY'];version=os.environ['RELEASE_VERSION'];tag='v'+version;commit=os.environ['GITHUB_SHA']
    binary=Path('dist/ACT3.zip');digest=Path('dist/ACT3.zip.sha256')
    if hashlib.sha256(binary.read_bytes()).hexdigest()!=digest.read_text().split()[0]:raise ValueError('Release checksum mismatch')
    notes=('ACT3 '+version+' — personal restoration by Michalis, not an official group release.\n\n'
        'One ZIP contains Windows x64 and Linux x86_64 players, shared assets and the original 2004 executable.\n'
        'Default: Party version, native fullscreen, original soundtrack.\n\n'
        'Download ACT3.zip, extract it, run ACT3.exe or ./ACT3-linux-x86_64.\n'
        'Linux targets glibc 2.31+ with system OpenGL/X11; helper libraries are included.\n\n'
        'Permanent download: https://github.com/'+repo+'/releases/latest/download/ACT3.zip\n')
    release=find_release(repo,tag)
    if release and not release['draft']:
        ref=api(f'repos/{repo}/git/ref/tags/{tag}')['object']
        while ref['type']=='tag':ref=api(f'repos/{repo}/git/tags/{ref["sha"]}')['object']
        if ref['sha']!=commit:raise RuntimeError('VERSION is already published from another commit; bump VERSION')
        print('This exact version/commit is already published.');return
    payload={'tag_name':tag,'target_commitish':commit,'name':'ACT3 '+version,'body':notes,'draft':True,'prerelease':False}
    if release:release=api(f'repos/{repo}/releases/{release["id"]}','PATCH',payload)
    else:release=api(f'repos/{repo}/releases','POST',payload)
    upload=release['upload_url'].split('{',1)[0]
    for file in [binary,digest]:
        for asset in release['assets']:
            if asset['name']==file.name:api(f'repos/{repo}/releases/assets/{asset["id"]}','DELETE')
        api(upload+'?'+urllib.parse.urlencode({'name':file.name}),'POST',raw=file.read_bytes(),content_type='application/octet-stream')
    # GitHub's legacy selection considers semantic version ordering.
    result=api(f'repos/{repo}/releases/{release["id"]}','PATCH',{'draft':False,'prerelease':False,'make_latest':'legacy'})
    print('Published',result['html_url'])
if __name__=='__main__':main()
