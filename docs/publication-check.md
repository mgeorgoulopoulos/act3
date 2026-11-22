# Publication check

The repository was prepared for publication with the following checks.

## Build and contents

- A fresh local clone, transferred using Git's normal object transport, configured
  and built successfully with Visual Studio 2022 in Release mode.
- All original runtime assets are inside the repository. The clean build does not
  need a sibling archive or an uncommitted historical-header shim.
- Windows Debug and Release builds passed. Aspect-ratio presentation was checked
  with 4:3, 16:9 and inherited multi-texture/cubemap state; real Space frames at
  800x600 and native display size were compared after the fix.
- Linux remains untested; the README states that boundary explicitly.
- The current tracked tree is approximately **43.3 MiB**. The packed Git object
  database is approximately **42.3 MiB** at this check. Sizes will grow with later commits.
- The largest reachable historical blob is **3.91 MiB**, the vendored miniaudio
  source header. No reachable blob exceeds 10 MiB.
- Large required content consists of source, music, geometry and original/HD art.
  Documentation screenshots total less than 1 MiB.
- No executables, DLLs, videos, build outputs, debug symbols, archives or thumbnail
  databases are tracked. Windows `Thumbs.db` caches were excluded from the imported assets.

## Privacy and presentation

- Author and committer metadata use the public project identity. Personal names
  and email addresses were removed from the commit metadata throughout history.
- That metadata-only history rewrite preserved file trees and commit messages.
  The old history was backed up locally outside the repository; it is not published.
- Old unreachable objects and reflogs were pruned after verifying that backup.
- Every reachable historical blob and commit message was scanned for private home
  paths, email addresses, private-key material and common credential shapes.
  No findings remained in the final scan.
- Screenshots were visually reviewed for desktop content and personal information.
  The launcher shot shows only its application window. Scene images are renderer captures.
- Image metadata was inspected. Generated assets and screenshots do not contain
  personal EXIF/text metadata; empty original PNG comment tags were retained.
- [Restoration notes](restoration.md) are a curated, respectful technical account.
  Private session transcripts and the local chronological working log are not tracked.
- [MIT licensing](../LICENSE), original public scene credits and the miniaudio
  license notice are included.

These checks describe the reviewed snapshot; they should be repeated after new
files or history are added. The automated scan is a targeted pre-publication check,
not a substitute for reviewing future content.

```sh
python tools/publication_audit.py
python tools/source_metrics.py
```
