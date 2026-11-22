#!/usr/bin/env python3
"""Versioned main pushes publish; PRs and ordinary released-version commits only build."""
import json
import os
from pathlib import Path
import subprocess
from release import ROOT, version

def plan(current,event,ref,previous='',tagged=False,manual=False):
    if event=='pull_request':return False
    if event=='workflow_dispatch':
        if manual and ref!='refs/heads/main':raise ValueError('Publishing is restricted to main')
        return manual
    if event!='push' or ref!='refs/heads/main':return False
    changed=previous!=current
    if changed and previous and tuple(map(int,current.split('.')))<=tuple(map(int,previous.split('.'))):
        raise ValueError('A release bump must increase VERSION')
    return changed or not tagged

def main():
    current=version();event=os.environ.get('GITHUB_EVENT_NAME','workflow_dispatch')
    context=json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text()) if os.environ.get('GITHUB_EVENT_PATH') else {}
    previous='';before=context.get('before','0'*40)
    if event=='push' and before.strip('0'):
        previous=subprocess.run(['git','show',before+':VERSION'],cwd=ROOT,capture_output=True,text=True).stdout.strip()
    tagged=subprocess.run(['git','show-ref','--verify','--quiet','refs/tags/v'+current],cwd=ROOT).returncode==0
    manual=context.get('inputs',{}).get('publish',False) in [True,'true']
    publish=plan(current,event,os.environ.get('GITHUB_REF',''),previous,tagged,manual)
    result=f'version={current}\npublish={str(publish).lower()}\n';print(result,end='')
    if os.environ.get('GITHUB_OUTPUT'):
        with open(os.environ['GITHUB_OUTPUT'],'a',encoding='utf-8') as output:output.write(result)
if __name__=='__main__':main()
