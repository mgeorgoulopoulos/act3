#!/usr/bin/env python3
"""Audit the committed tree and every reachable historical blob before publication."""
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
def git(*args, **kwargs):
    return subprocess.check_output(['git', *args], cwd=ROOT, **kwargs)

def audit():
    entries = git('rev-list', '--objects', '--all').decode().splitlines()
    names = {}
    for entry in entries:
        oid, _, name = entry.partition(' ')
        names.setdefault(oid, name)
    ids = list(names)
    checks = git('cat-file', '--batch-check', input=('\n'.join(ids)+'\n').encode()).decode().splitlines()
    blobs = [(oid, int(size)) for oid, kind, size in (line.split() for line in checks) if kind == 'blob']
    raw = git('cat-file', '--batch', input=('\n'.join(oid for oid, _ in blobs)+'\n').encode())
    patterns = {
        'private_home_path': rb'(?:[A-Za-z]:[\\/]+Users[\\/]+[^\s\x00<>"\']+|/home/[A-Za-z0-9_.-]+/|/Users/[A-Za-z0-9_.-]+/)',
        'private_key': rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
        'credential_shape': rb'(?:ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,}|AKIA[A-Z0-9]{16}|sk-proj-[A-Za-z0-9_-]{40,})',
    }
    findings = []; offset = 0
    emails = set()
    for oid, size in blobs:
        end = raw.index(b'\n', offset); data = raw[end+1:end+1+size]; offset = end+size+2
        for label, expression in patterns.items():
            if re.search(expression, data): findings.append({'kind':label, 'path':names[oid], 'blob':oid})
        if b'\0' not in data and not names[oid].startswith('third_party/'):
            for match in re.findall(rb'[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}', data):
                if match != b'act3@users.noreply.github.com': emails.add(match.decode('ascii'))
    messages = git('log', '--all', '--format=%B')
    for label, expression in patterns.items():
        if re.search(expression, messages): findings.append({'kind':label, 'path':'commit messages'})
    for match in re.findall(rb'[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}', messages):
        if match != b'act3@users.noreply.github.com': emails.add(match.decode('ascii'))
    authors = sorted(set(git('log' , '--all', '--format=%an <%ae> | %cn <%ce>').decode().splitlines()))
    metadata_clean = all(line == 'Reversed Engineers <act3@users.noreply.github.com> | Reversed Engineers <act3@users.noreply.github.com>' for line in authors)
    tracked = git('ls-files').decode().splitlines()
    unwanted = [p for p in tracked if Path(p).suffix.lower() in {'.exe','.dll','.pdb','.obj','.ilk','.mp4','.avi','.zip','.7z','.tar','.db','.log','.bundle'}
                or Path(p).name in {'.env','desktop.ini','.DS_Store'} or p.startswith(('build/','.vs/'))]
    biggest = sorted(((size,names[oid]) for oid,size in blobs),reverse=True)[:10]
    current_sizes = [(int(line.split()[3]),line.split('\t',1)[1]) for line in git('ls-tree','-rl','HEAD').decode().splitlines()]
    return dict(reachable_commits=int(git('rev-list','--count','HEAD')),
        reachable_unique_blob_bytes=sum(size for _,size in blobs),
        current_tree_bytes=sum(size for size,_ in current_sizes),
        largest_historical_blobs=[{'bytes':s,'path':p} for s,p in biggest],
        personal_commit_metadata_removed=metadata_clean,
        sensitive_pattern_findings=findings, non_vendor_text_emails=sorted(emails),
        unwanted_tracked_files=unwanted, files_over_10_MiB=[{'bytes':s,'path':p} for s,p in biggest if s>10*1024*1024])

if __name__ == '__main__':
    result=audit();print(json.dumps(result,indent=2))
    raise SystemExit(bool(result['sensitive_pattern_findings'] or result['unwanted_tracked_files'] or result['non_vendor_text_emails'] or not result['personal_commit_metadata_removed']))
