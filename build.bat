#!/bin/bash 2>/dev/null || goto :windows

# -------------------
# LINUX 
# -------------------

#!/bin/bash
set -e  # Exit on error

# Ensure a main file is provided
if [ -z "$1" ]; then
    echo "Error: No input file provided."
    echo "Usage: ./build <source_file.c> [debug]"
    exit 1
fi

MAIN_FILE="$1"
LIB_PATH="./libs"

# Choose build type
if [ "$2" == "debug" ]; then
    CFLAGS="-g -O0 -DDEBUG /DKIT_LOG_USE_COLOR"
    OUTFILE="${MAIN_FILE%.c}_debug"
    LDFLAGS= "-L$LIB_PATH -lbgfxDebug -lbimgDebug -lbxDebug -lglfw3 -ldl -lm -lpthread -lstdc++ -lX11 -lXcursor -lGL -lXi -lXrandr -lvulkan"
else
    CFLAGS="-O2 -DNDEBUG -s -ffunction-sections -fdata-sections -DKIT_LOG_USE_COLOR"
    OUTFILE="${MAIN_FILE%.c}"
    LDFLAGS= "-L$LIB_PATH -lbgfxRelease -lbimgRelease -lbxRelease -lglfw3 -ldl -lm -lpthread -lstdc++ -lX11 -lXcursor -lGL -lXi -lXrandr -lvulkan"
fi

echo "Building $OUTFILE with main file $MAIN_FILE..."

gcc $CFLAGS $MAIN_FILE kit/kit.c kit/zpl.c $LDFLAGS -o $OUTFILE

echo "Done."
exit 0

# -------------------
# WINDOWS 
# -------------------
:windows
@echo off
setlocal enabledelayedexpansion

REM Ensure a main file is provided
if "%1"=="" (
    echo Error: No input file provided.
    echo Usage: build.bat ^<source_file.c^> [debug]
    exit /b 1
)

set "MAIN_FILE=%1"

REM Choose build type
if "%2"=="debug" (
    set "CFLAGS=/Od /Zi /MDd /DDEBUG /DKIT_LOG_USE_COLOR"
    set "LDFLAGS=/link /LIBPATH:libs bxDebug.lib bimgDebug.lib bgfxDebug.lib"
    set "OUTFILE=%~n1_debug.exe"
) else (
    set "CFLAGS=/O2 /MD /DNDEBUG /DKIT_LOG_USE_COLOR"
    set "OUTFILE=%~n1.exe"
    set "LDFLAGS=/link /LIBPATH:libs bxRelease.lib bimgRelease.lib bgfxRelease.lib"
)

echo Building %OUTFILE% with main file %MAIN_FILE%...


cl %CFLAGS% %MAIN_FILE% kit/kit.c /Zc:preprocessor %LDFLAGS% /OUT:%OUTFILE%

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

REM echo Done.

