@echo off
rem Build from a normal cmd/PowerShell prompt using the MSYS2 UCRT64 toolchain.
rem Usage: build [make targets/options], e.g.  build   build run   build release   build clean
rem Set MSYS2_ROOT if MSYS2 is not installed in C:\msys64.
setlocal
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"

if not exist "%MSYS2_ROOT%\ucrt64\bin\gcc.exe" (
    echo error: gcc not found in "%MSYS2_ROOT%\ucrt64\bin".
    echo Install MSYS2 and the UCRT64 packages listed in README.md, or set MSYS2_ROOT.
    exit /b 1
)
if not exist "%MSYS2_ROOT%\usr\bin\make.exe" (
    echo error: make not found in "%MSYS2_ROOT%\usr\bin". Run: pacman -S make
    exit /b 1
)

set "PATH=%MSYS2_ROOT%\ucrt64\bin;%MSYS2_ROOT%\usr\bin;%PATH%"
cd /d "%~dp0"
make %*
exit /b %ERRORLEVEL%
