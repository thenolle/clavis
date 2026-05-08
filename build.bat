@echo off
setlocal enabledelayedexpansion

set NAME=Clavis
set OUT=bin
set SRC=src\main.c src\audio.c src\hooks.c src\hotkey.c src\soundpack.c src\tray.c src\ui.c src\theme.c src\embedded_packs.c
set INC=-Iinclude
set RES=%OUT%\res.o

if not exist %OUT% mkdir %OUT%
if not exist %OUT%\Soundpacks mkdir %OUT%\Soundpacks

echo [*] Embedding soundpacks...
python tools\embed_packs.py
if errorlevel 1 (
    echo [FAIL] embed_packs.py failed.
    exit /b 1
)

echo [*] Compiling resources...
windres resources\resource.rc -O coff -o %RES%
if errorlevel 1 (
    echo [WARN] windres failed, skipping icon.
    set RES=
)

set CFLAGS=-Wall -Wextra -Wno-unused-parameter -municode -mwindows
set DEFS=-D_WIN32_WINNT=0x0601 -DWINVER=0x0601
set LIBS=-lwinmm -lshell32 -lcomctl32 -lshlwapi -lgdi32 -luser32 -lcomdlg32 -ldwmapi -luxtheme

if /i "%1"=="debug" (
    set OPT=-g -O0
    echo [*] Mode: Debug
) else (
    set OPT=-O2 -s
    echo [*] Mode: Release
)

echo [*] Compiling...
gcc %CFLAGS% %DEFS% %OPT% %INC% %SRC% %RES% %LIBS% -o %OUT%\%NAME%.exe

if errorlevel 1 (
    echo.
    echo [FAIL] Build failed.
    exit /b 1
)

echo.
echo [OK]  Built: %OUT%\%NAME%.exe
echo       Embedded packs are baked in.
echo       Drop extra packs into: %OUT%\Soundpacks\
endlocal