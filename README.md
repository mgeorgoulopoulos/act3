# Build ACT3

The current target opens an OpenGL window. Press Escape or close the window
to exit.

## Windows

Install Visual Studio 2022 with the Desktop development with C++ workload and
use CMake to generate the solution:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64

Open `build/ACT3.sln` and press F5.

## Linux

On Debian or Ubuntu, install `build-essential`, `cmake`, `libx11-dev` and
`libglvnd-dev`. Run from a graphical session with X11 or XWayland and GLX:

    cmake -S . -B build
    cmake --build build
    ./build/act3

If Windows and Linux share the same checkout, use a distinct build directory
for each platform, such as `build/linux` on Linux.
