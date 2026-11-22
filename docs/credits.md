# Credits and third-party notices

ACT3 was presented by **Reversed Engineers at React 2004**.
The original in-demo credits are preserved:

| Contribution | Original credit |
| --- | --- |
| Code | msamurai |
| 3D graphics | quiesce |
| 2D graphics | sybeX |
| Music | cat (styled with a cent-sign glyph in the original artwork) |

The soundtrack is **Universus**. The restored player uses the original meshes,
textures, recorded camera tracks and soundtrack from the original release.
The historical source is the surviving recovered snapshot, not a claim to have
reconstructed a byte-identical 2004 release-source tree.

The restoration and 22nd anniversary edition were developed with AI assistance,
under the original author's direction and visual review. Selected replacement
textures and the launcher emblem use newly generated artwork. The original
models and composition remain the basis of the production. The anniversary
edition is an alternate artistic treatment, separate from the Party version.

## miniaudio

[miniaudio 0.11.23](https://github.com/mackron/miniaudio/tree/0.11.23), by David Reid,
is vendored in `third_party/miniaudio/`. It provides cross-platform playback and
MP3 decoding without a separately distributed audio DLL.

It is available under the public-domain dedication or MIT No Attribution license
printed in the header. Those notices remain intact. See its
[version and checksum](../third_party/miniaudio/VERSION) and
[full source/license text](../third_party/miniaudio/miniaudio.h).

## Project license

This repository is published under the [MIT license](../LICENSE).
The third-party license above remains applicable to its respective component.
Windows, OpenGL, X11, XRandR and libpng are platform/system dependencies;
this repository does not redistribute those libraries or font files.
