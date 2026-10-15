# ![WARNING! AI SLOP](https://img.shields.io/badge/-WARNING!_AI_SLOP-red?style=for-the-badge)

> [!WARNING]
> This repository was largely AI-generated with minimal human review. Verify before relying on it.


# sdl3dviewer

A small first-person 3D environment in C using SDL3 and OpenGL 3.3 core (loaded with GLAD).
Runs on Windows, Linux and macOS, built with a plain GNU Makefile.

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

`SDL3.dll` and every MSYS2 runtime DLL it depends on (e.g. `libiconv-2.dll`, found automatically
by scanning the DLL import tables) are copied next to the `.exe` during the build, so the
contents of `build/release/` can be run directly (double-click) or copied elsewhere.

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

### macOS

Install the dependencies with [Homebrew](https://brew.sh/):

```sh
brew install sdl3 pkg-config
```

Then:

```sh
make run
```

Notes:

- Apple's default `clang` (what `gcc` resolves to) works fine; no other
  toolchain is needed.
- The debug build enables ASan/UBSan, which clang on macOS supports.
- The binary runs from the terminal; there is no `.app` bundle packaging,
  so it is not launchable from Finder by double-clicking.

### Make targets

- `make` - debug build in `build/debug/` (with ASan/UBSan on Linux and macOS)
- `make release` - optimised build in `build/release/` (no console window on Windows)
- `make run` - build and run (use `make run BUILD=release` for the release build)
- `make clean` - remove `build/`

GLAD was generated with `glad2`:
`glad --api gl:core=3.3 --extensions= --out-path external/glad c`

## Level persistence

The world (built-in level + everything placed in build mode) is saved automatically
when the game closes, and any time with `F5`. The file lives in SDL's per-user
preferences directory: `%APPDATA%\sdl3dviewer\level.txt` on Windows, `~/Library/Application Support/sdl3dviewer/level.txt`
on macOS, and on Linux `~/.local/share/sdl3dviewer/level.txt`. The exact path is printed to the console
(debug build) at startup.

- If the file exists it **replaces** the built-in level at startup; without it you
  get the built-in level again (so deleting the file resets the game).
- The format is plain text, one shape per line, easy to edit by hand:

  ```
  # sdl3dviewer level v1
  box      x0 y0 z0 x1 y1 z1  r g b
  cylinder cx cy cz radius height  r g b
  sphere   cx cy cz radius  r g b
  ```

Settings (mouse sensitivity, move speed and sprint multiplier, changed in the
pause menu) are saved to `settings.txt` in the same directory.

## Controls

| Input | Action |
|---|---|
| Mouse | Look |
| W A S D / arrow keys | Move |
| Shift | Sprint |
| Space | Jump |
| Esc | Open pause menu (Return to Game / Settings / Exit) / exit build mode, close menu |
| E | Build menu (choose shape and color, type dimensions, Enter to place); ghosts snap to the ground and to the sides of shapes; in remove mode: switch to build mode |
| R | Toggle remove mode; in build mode: switch to remove mode; in remove mode, left click removes the highlighted shape |
| Left click | Recapture mouse / in the menu: click buttons and fields / place in build mode |
| Right click | Cancel menu / build mode |
| F5 | Save the level to disk |
