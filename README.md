# sdl3dviewer

A small first-person 3D environment in C using SDL3 and OpenGL 3.3 core (loaded with GLAD).
Runs on Windows and Linux, built with a plain GNU Makefile.

## Controls

| Input | Action |
|---|---|
| Mouse | Look |
| W A S D / arrow keys | Move |
| Shift | Sprint |
| Space | Jump |
| Esc | Release mouse (press again to quit) |
| Left click | Recapture mouse |

## Building

### Windows (MSYS2 UCRT64)

1. Install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** shell and install the toolchain:
   ```sh
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-pkgconf make
   ```
3. Build and run from the repo directory, from any normal prompt (cmd, PowerShell, VS Code terminal):
   ```bat
   build run
   ```
   `build.cmd` adds `C:\msys64\ucrt64\bin` and `C:\msys64\usr\bin` to `PATH` for that command only
   and then runs `make` with whatever arguments you pass (`build`, `build release`, `build clean`, ...).
   If MSYS2 is installed elsewhere, set `MSYS2_ROOT` (e.g. `set MSYS2_ROOT=D:\msys64`).
   In PowerShell use `.\build run`.

   Inside the MSYS2 UCRT64 shell, plain `make run` works too.

`SDL3.dll` is copied next to the `.exe` during the build, so the contents of `build/release/`
can be run directly (double-click) or copied elsewhere; no other MSYS2 DLLs are needed.

### Linux

Install a compiler, make, pkg-config and the SDL3 development package, e.g.:

```sh
sudo apt install build-essential pkg-config libsdl3-dev   # Debian/Ubuntu (24.10+)
sudo dnf install gcc make pkgconf SDL3-devel               # Fedora
sudo pacman -S base-devel sdl3                             # Arch
```

Then:

```sh
make run
```

### Make targets

- `make` - debug build in `build/debug/` (with ASan/UBSan on Linux)
- `make release` - optimised build in `build/release/` (no console window on Windows)
- `make run` - build and run (use `make run BUILD=release` for the release build)
- `make clean` - remove `build/`

## Layout

```
src/main.c      SDL/GL setup, event loop, input
src/player.c    first-person movement, gravity, jumping, AABB collision
src/render.c    shaders, cube mesh, drawing
src/scene.c     hardcoded level (boxes)
src/mathx.h     vec3 / mat4 helpers
external/glad/  generated OpenGL 3.3 core loader
```

GLAD was generated with `glad2`:
`glad --api gl:core=3.3 --extensions= --out-path external/glad c`
