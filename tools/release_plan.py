#!/usr/bin/env python3
"""Publish only a VERSION bump on main; ordinary commits and PRs remain CI builds."""
import json
import os
from pathlib import Path
import subprocess
from release import ROOT, version

current=version();event=os.environ.get('GITHUB_EVENT_NAME','workflow_dispatch')
context=json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text()) if os.environ.get('GITHUB_EVENT_PATH') else {}
publish=False
if event=='push' and os.environ.get('GITHUB_REF')=='refs/heads/main':
    before=context.get('before','0'*40)
    previous=subprocess.run(['git','show',before+':VERSION'],cwd=ROOT,capture_output=True,text=True).stdout.strip() if before.strip('0') else ''
    publish=previous!=current
    if publish and previous and tuple(map(int,current.split('.')))<=tuple(map(int,previous.split('.'))):
        raise SystemExit('A release bump must increase VERSION')
elif event=='workflow_dispatch':
    publish=context.get('inputs',{}).get('publish',False) in [True,'true']
    if publish and os.environ.get('GITHUB_REF')!='refs/heads/main':raise SystemExit('Publishing is restricted to main')
result=f'version={current}\npublish={str(publish).lower()}\n'
print(result,end='')
if os.environ.get('GITHUB_OUTPUT'):
    with open(os.environ['GITHUB_OUTPUT'],'a',encoding='utf-8') as output:output.write(result)
