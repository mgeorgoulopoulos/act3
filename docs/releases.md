# Versioned portable releases

This is Michalis's personal restoration, not an official Reversed Engineers release.

## Publish a version

1. Increase `VERSION` using stable `major.minor.patch` notation.
2. Commit and push to `main`.
3. Watch [Build and release ACT3](https://github.com/mgeorgoulopoulos/act3/actions/workflows/release.yml).

The workflow publishes only after both platform builds, asset checks and packaging
succeed. It creates a versioned GitHub release and uploads `ACT3.zip` plus
`ACT3.zip.sha256`. Failed builds leave the last good download available.
Ordinary commits after a published version, and pull requests, build without publishing.
An unreleased version is retried on the next main push if its first build fails. A manual workflow run
can build without release; its explicit `publish` switch permits a release from
`main`, useful for a first release or retry. An existing published version is never
silently replaced with a different source commit.

Permanent URL:

**https://github.com/mgeorgoulopoulos/act3/releases/latest/download/ACT3.zip**

## Package recipe

The pipeline first downloads the 2004 release from scene.org. Its exact size and
SHA-256 are pinned in [original.json](../packaging/original.json). Downloads with
unexpected bytes are rejected. The original supplies the runtime assets for both
builds, not an unverified local folder.

CMake installs the restored players and assets. Packaging preserves the original
release contents, removes thumbnail caches, renames the historical executable,
merges the new assets and adds the two restored executables. Shared assets are
checked for byte equality between platform builds before they are combined.
Archive paths are checked for traversal and case-insensitive duplicates. Linux
executable permissions are encoded in the ZIP.

```text
ACT3/
  ACT3.exe                   restored Windows x64 player / native launcher
  ACT3-linux-x86_64          restored portable Linux player / CLI
  ACT3-2004.exe              unchanged historical Windows executable
  data/                     original geometry, cameras, textures and soundtrack + LUTs
  extra/                    anniversary HD textures
  linux/lib/                portable Linux helper libraries
  linux/licenses/           bundled-library notices
  licenses/miniaudio.txt
  asset-manifest.txt
  LICENSE
  VERSION
  README.txt
  ...                       original NFO / file_id.diz
```

The player locates data relative to its own executable, independent of the current
working directory or the machine that built it. CMake build-tree execution also
works, with `assets/` beside or one directory above the executable.

## Windows

Visual Studio 2022 x64, Release, statically linked MSVC runtime. The GUI, image
loading and windowing use native Windows APIs. No UI toolkit/audio DLL or separate
VC++ runtime installation is added to the distribution. The old executable may
still need its historical platform dependencies; that does not apply to the new player.

## Linux portability

- x86_64, built in Debian 11 with a **glibc 2.31** ABI ceiling.
- GCC C++ and support runtimes linked statically.
- libpng/libjpeg/zlib and stable XRandR helper libraries shipped in `linux/lib`, with
  origin-relative runtime search paths and corresponding licenses.
- libc, the loader, X11/XCB and graphics-driver libraries come from the desktop.
  Bundling those can conflict with the installed Mesa/NVIDIA stack.
- Requires desktop OpenGL compatibility support, X11 or XWayland and a working
  system audio backend. Pure musl/Alpine, NixOS's nonstandard loader layout,
  ARM CPUs and native Wayland-only setups are outside this binary target.
- No FUSE or AppImage runtime requirement.

CI checks the binary's required glibc symbols, missing dependencies, its version,
and assets after relocation. Both editions also load all scenes under Xvfb/Mesa
without audio or continuous playback, catching case-sensitive paths and resource failures. The assembled archive is extracted and checked again
on a current Ubuntu runner. These checks do not substitute for visual/audio testing
on real Linux hardware.

## Local packaging

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix dist/local
```

For the combined release, the workflow uses the same public Python helpers:

```sh
python tools/release.py fetch-original --output build/original.zip
python tools/release.py assemble --original build/original.zip --windows stage/windows --linux stage/linux --output dist/ACT3.zip
python -m unittest discover -s tests -v
```

No personal access token is stored in the source. Only the final release job has
`contents: write`, using GitHub's automatically issued `GITHUB_TOKEN`. Official
actions are pinned to commit IDs. Release binaries, downloaded archives, staging
folders and ZIPs are build outputs and stay outside Git.

### Full Linux playback smoke check

Before packaging, both editions play to their natural end under Xvfb/Mesa and
PulseAudio on Ubuntu 24.04, using the portable Linux build. The test uses the real
soundtrack clock, fullscreen at the virtual display's native resolution, and an
unrelated working directory. It also injects sustained X11 client messages during the intro to catch render-loop
starvation. It checks for visible, changing frames in all five
parts and a normal exit after the full production. Small screenshot contact sheets
and player output are retained as short-lived CI artifacts; the player contains
no temporary diagnostic instrumentation. This catches stalls and gross rendering
failures in this software-rendered environment, not vendor-specific GPU problems
or audible output on physical hardware.

Texture regression checks also load the intro R/E glyphs after deliberately
raising unrelated OpenGL errors, then inspect their uploaded pixels. The chrome
cubemap check compares all adjacent face edges after upload, catching reversed
Z faces rather than relying on screenshots alone. These checks run in the build
container before installation; the test executable is not distributed.
