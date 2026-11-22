# How much of ACT3 is still the original?

Measured from the first recovered-source commit to the current committed tree.
Run `python tools/source_metrics.py` to reproduce the numbers.

| Measure | Result |
| --- | ---: |
| Original source/resource-script files | 38 |
| Files with source-line changes | 10 |
| Original physical source lines | 12,783 |
| Original lines changed or removed | 63 |
| **Historical source lines changed/removed** | **0.49%** |
| Added lines inside historical files | 71 |
| **Edit volume including those additions, relative to original lines** | **1.05%** |
| Unchanged historical lines retained | 12,720 |
| Final first-party source lines | 18,693 |
| **Unchanged historical share of the first-party source tree** | **68.05%** |

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
