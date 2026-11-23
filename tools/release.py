#!/usr/bin/env python3
"""Verified original download, safe extraction and the combined portable release ZIP."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import tempfile
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def version():
    text=(ROOT/'VERSION').read_text(encoding='utf-8').strip()
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)',text):
        raise ValueError('VERSION must contain one stable SemVer, e.g. 0.1.0')
    return text

def verify_original(path):
    spec=json.loads((ROOT/'packaging/original.json').read_text())
    if path.stat().st_size!=spec['bytes'] or hashlib.sha256(path.read_bytes()).hexdigest()!=spec['sha256']:
        raise ValueError('Original archive checksum/size mismatch; refusing to package it')

def fetch_original(path):
    spec=json.loads((ROOT/'packaging/original.json').read_text())
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():
        verify_original(path);return
    errors=[]
    for url in [spec['url'],spec['fallback_url']]:
        try:
            request=urllib.request.Request(url,headers={'User-Agent':'ACT3-restoration-release/1'})
            with urllib.request.urlopen(request,timeout=90) as response:
                data=response.read(spec['bytes']+1)
            temporary=path.with_suffix('.part');temporary.write_bytes(data)
            verify_original(temporary);temporary.replace(path);return
        except Exception as exc:
            errors.append(str(exc))
    raise RuntimeError('Could not download verified original: '+'; '.join(errors))

def extract_original(archive,destination):
    verify_original(archive)
    destination.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            name=entry.filename.replace('\\','/')
            relative=PurePosixPath(name)
            if relative.is_absolute() or '..' in relative.parts or ':' in name:
                raise ValueError('Unsafe ZIP member path')
            if entry.is_dir() or relative.name.lower() in {'thumbs.db','desktop.ini','.ds_store'}:continue
            if stat.S_ISLNK(entry.external_attr>>16):raise ValueError('Symlink in original archive')
            if entry.file_size>64*1024*1024:raise ValueError('Unexpected original asset size')
            target=destination.joinpath(*relative.parts);target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(source.read(entry))
    roots=list(destination.glob('**/Act3.exe'))
    if len(roots)!=1:raise ValueError('Expected exactly one original Act3.exe')
    return roots[0].parent

def overlay(source,destination):
    """Merge canonical build paths without Windows case-folding collisions."""
    destination.mkdir(parents=True,exist_ok=True)
    for item in sorted(source.iterdir()):
        existing=next((p for p in destination.iterdir() if p.name.casefold()==item.name.casefold()),None)
        target=destination/item.name
        if existing and existing.name!=item.name:
            existing.rename(target)
        if item.is_dir():overlay(item,target)
        else:shutil.copyfile(item,target)

def check_manifest(root):
    manifest=root/'asset-manifest.txt'
    if not manifest.is_file():raise ValueError('Missing asset manifest')
    for line in manifest.read_text().splitlines():
        size,name=line.split('|',1)
        target=root.joinpath(*PurePosixPath(name).parts)
        if not target.is_file() or target.stat().st_size!=int(size):raise ValueError('Bad asset: '+name)

def assemble(archive,windows,linux,output):
    release_version=version()
    check_manifest(windows);check_manifest(linux)
    # Both platforms must be built from identical content before sharing assets.
    for item in windows.rglob('*'):
        if item.is_file() and item.relative_to(windows).parts[0] in {'data','extra'}:
            counterpart=linux/item.relative_to(windows)
            if not counterpart.is_file() or hashlib.sha256(item.read_bytes()).digest()!=hashlib.sha256(counterpart.read_bytes()).digest():
                raise ValueError('Platform asset mismatch: '+str(item.relative_to(windows)))
    with tempfile.TemporaryDirectory(prefix='act3-package-') as temp:
        temp=Path(temp);original=extract_original(archive,temp/'original');package=temp/'ACT3'
        shutil.copytree(original,package)
        old=package/'Act3.exe';old.rename(package/'ACT3-2004.exe')
        overlay(windows,package)
        built=package/'act3.exe'
        if not built.is_file():raise ValueError('Windows executable missing')
        built.rename(package/'ACT3.exe')
        binary=linux/'act3'
        if not binary.is_file() or binary.read_bytes()[:4]!=b'\x7fELF':raise ValueError('Linux ELF missing')
        shutil.copyfile(binary,package/'ACT3-linux-x86_64')
        if not (linux/'linux/lib').is_dir():raise ValueError('Portable Linux libraries missing')
        overlay(linux/'linux',package/'linux')
        shutil.copyfile(ROOT/'LICENSE',package/'LICENSE')
        (package/'VERSION').write_text(release_version+'\n',encoding='utf-8')
        (package/'README.txt').write_text(
            'ACT3 '+release_version+' / personal restoration by Michalis\n'
            'Original production: Reversed Engineers, React 2004.\n'
            'This restoration is not an official group release.\n\n'
            'Windows x64: double-click ACT3.exe\n'
            'Linux x86_64: ./ACT3-linux-x86_64 (Xlib settings launcher)\n'
            '  If an extractor removes executable permissions: chmod +x ACT3-linux-x86_64\n'
            '  Requires glibc 2.31+, desktop OpenGL and X11/XWayland. Audio uses the system backend.\n'
            '  Arguments start directly: --extra selects the 22nd anniversary edition.\n'
            '  --no-ui starts Party playback; --ui opens settings; --help lists options.\n'
            'Default: Party version / native fullscreen / music. Escape exits.\n\n'
            'Keep data/, extra/, linux/ and asset-manifest.txt beside the executables.\n'
            'ACT3-2004.exe is the unchanged historical Windows executable.\n'
            'The restored player does not need its legacy DirectX/audio runtime.\n\n'
            'Latest: https://github.com/mgeorgoulopoulos/act3/releases/latest/download/ACT3.zip\n',encoding='utf-8')
        licenses=package/'licenses';licenses.mkdir(exist_ok=True)
        header=(ROOT/'third_party/miniaudio/miniaudio.h').read_text(encoding='utf-8')
        start=header.rfind('This software is available as a choice of the following licenses')
        if start<0:start=header.rfind('ALTERNATIVE 1')
        (licenses/'miniaudio.txt').write_text(header[start:].rstrip('*/\n')+'\n',encoding='utf-8')
        check_manifest(package)
        output.parent.mkdir(parents=True,exist_ok=True)
        names=set()
        with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as zipped:
            for path in sorted(package.rglob('*')):
                if not path.is_file():continue
                relative=path.relative_to(temp).as_posix()
                folded=relative.casefold()
                if folded in names:raise ValueError('Case-folding duplicate in release: '+relative)
                names.add(folded)
                info=zipfile.ZipInfo(relative,date_time=(2026,1,1,0,0,0));info.create_system=3
                mode=0o755 if path.name=='ACT3-linux-x86_64' else 0o644
                info.external_attr=(stat.S_IFREG|mode)<<16;info.compress_type=zipfile.ZIP_DEFLATED
                zipped.writestr(info,path.read_bytes(),compresslevel=9)
        checksum=hashlib.sha256(output.read_bytes()).hexdigest()
        output.with_suffix(output.suffix+'.sha256').write_text(checksum+'  '+output.name+'\n',encoding='ascii')
        print('Packaged',output.name,output.stat().st_size,'bytes; SHA256',checksum)

def main():
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    fetch=commands.add_parser('fetch-original');fetch.add_argument('--output',type=Path,required=True)
    extract=commands.add_parser('extract-original');extract.add_argument('--archive',type=Path,required=True);extract.add_argument('--output',type=Path,required=True)
    commands.add_parser('version')
    package=commands.add_parser('assemble')
    for option in ['original','windows','linux','output']:package.add_argument('--'+option,type=Path,required=True)
    args=parser.parse_args()
    if args.command=='version':print(version())
    elif args.command=='fetch-original':fetch_original(args.output)
    elif args.command=='extract-original':print(extract_original(args.archive,args.output))
    else:assemble(args.original,args.windows,args.linux,args.output)
if __name__=='__main__':main()
