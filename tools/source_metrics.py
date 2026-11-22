#!/usr/bin/env python3
"""Reproduce the source-tree preservation figures; uses Git and Python's stdlib."""
import argparse
import difflib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)
def source(name):
    return (not name.startswith(('third_party/', 'tools/', 'docs/')) and
            (Path(name).suffix in {'.cpp', '.h', '.rc', '.in', '.py', '.ps1'} or name == 'CMakeLists.txt'))
def lines(data):
    return [line.rstrip() for line in data.decode('utf-8', errors='replace').splitlines()]
def measure():
    baseline = git('rev-list', '--max-parents=0', 'HEAD').decode().splitlines()[0]
    original = [p for p in git('ls-tree', '-r', '--name-only', baseline).decode().splitlines() if source(p)]
    current = [p for p in git('ls-tree', '-r', '--name-only', 'HEAD').decode().splitlines() if source(p)]
    old_count = changed = added = surviving = touched = 0
    for name in original:
        old = lines(git('show', f'{baseline}:{name}'))
        new = lines(git('show', f'HEAD:{name}')) if name in current else []
        removed = inserted = 0
        for tag, a, b, c, d in difflib.SequenceMatcher(None, old, new, autojunk=False).get_opcodes():
            if tag != 'equal':
                removed += b-a
                inserted += d-c
        old_count += len(old); changed += removed; added += inserted
        surviving += len(old)-removed; touched += old != new
    total = sum(len(lines(git('show', f'HEAD:{p}'))) for p in current)
    return dict(baseline_source_files=len(original), changed_source_files=touched,
                baseline_lines=old_count, changed_or_removed_baseline_lines=changed,
                added_lines_in_historical_files=added, unchanged_historical_lines=surviving,
                final_first_party_source_lines=total,
                historical_lines_changed_percent=round(100*changed/old_count, 2),
                unchanged_historical_share_percent=round(100*surviving/total, 2))
def markdown(m):
    return f'''# How much of ACT3 is still the original?

Measured from the first recovered-source commit to the current committed tree.
Run `python tools/source_metrics.py` to reproduce the numbers.

| Measure | Result |
| --- | ---: |
| Original source/resource-script files | {m['baseline_source_files']:,} |
| Files with source-line changes | {m['changed_source_files']:,} |
| Original physical source lines | {m['baseline_lines']:,} |
| Original lines changed or removed | {m['changed_or_removed_baseline_lines']:,} |
| **Historical source lines changed/removed** | **{m['historical_lines_changed_percent']}%** |
| Added lines inside historical files | {m['added_lines_in_historical_files']:,} |
| Unchanged historical lines retained | {m['unchanged_historical_lines']:,} |
| Final first-party source lines | {m['final_first_party_source_lines']:,} |
| **Unchanged historical share of the first-party source tree** | **{m['unchanged_historical_share_percent']}%** |

## What these figures mean

- Physical lines include comments and blank lines. CRLF/LF and trailing whitespace
  are normalized. A replacement counts as a changed original line, not as both an
  original deletion and an extra original insertion. Added lines are reported separately.
- The denominator for the first percentage is the original source snapshot. The
  denominator for the second is the final first-party source tree.
- C/C++ headers and sources, resource scripts, CMake, source templates and asset
  generation scripts count. Documentation, images, music, meshes, build outputs,
  audit tools and vendored miniaudio do not.
- Historical source files are counted once. Generated build-tree copies are not
  counted again. The tree still includes the old entry point, engine shell and
  camera-recording code, even where the new host does not compile those paths.
- These are **source preservation figures**, not percentages of executable bytes,
  frames, runtime work, creative authorship, or only the code compiled into the player.
  They must not be presented as such.
- Configure-time adapters alter generated translation units and insert optional
  enhancement hooks. A small on-disk historical diff does not imply zero adaptation
  or identical rendering behavior. See [restoration notes](restoration.md).
- Uncommitted local changes are deliberately excluded, so published figures describe
  what a clone receives.
'''
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true', help='Update docs/code-provenance.md')
    args = parser.parse_args(); result = measure()
    print(json.dumps(result, indent=2))
    if args.write:
        (ROOT/'docs/code-provenance.md').write_text(markdown(result), encoding='utf-8')
