#!/usr/bin/env python3
"""Bundle stable helper libraries, keeping libc/X11/XCB and GPU drivers on the host."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess

BUNDLED={'libpng16.so.16','libz.so.1','libXrandr.so.2','libXrender.so.1','libXext.so.6'}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('prefix',type=Path);args=p.parse_args()
    binary=args.prefix/'act3';libraries=args.prefix/'linux/lib';licenses=args.prefix/'linux/licenses'
    libraries.mkdir(parents=True,exist_ok=True);licenses.mkdir(parents=True,exist_ok=True)
    output=subprocess.check_output(['ldd',str(binary)],text=True)
    if 'not found' in output:raise RuntimeError(output)
    copied=set()
    for soname,path in re.findall(r'^\s*(\S+)\s+=>\s+(\/\S+)',output,re.M):
        if soname not in BUNDLED:continue
        target=libraries/soname;shutil.copyfile(Path(path).resolve(),target)
        subprocess.run(['patchelf','--set-rpath','$ORIGIN',str(target)],check=True)
        copied.add(soname)
    if not {'libpng16.so.16','libz.so.1','libXrandr.so.2'}.issubset(copied):raise RuntimeError('Missing expected helper libraries')
    for package in ['libpng16-16','zlib1g','libxrandr2','libxrender1','libxext6','gcc-10-base']:
        source=Path('/usr/share/doc')/package/'copyright'
        if not source.exists():raise RuntimeError('Missing license: '+str(source))
        shutil.copyfile(source,licenses/(package+'.txt'))
    # Confirm the portability floor and absence of a dynamically linked GCC runtime.
    versions=subprocess.check_output(['objdump','-T',str(binary)],text=True)
    required={tuple(map(int,v.split('.'))) for v in re.findall(r'GLIBC_([0-9.]+)',versions)}
    if any(v>(2,31) for v in required):raise RuntimeError('Binary exceeds the glibc 2.31 baseline')
    if 'libstdc++.so' in output or 'libgcc_s.so' in output:raise RuntimeError('GCC runtime was not linked statically')
    print('Portable libraries:',', '.join(sorted(copied)))
if __name__=='__main__':main()
