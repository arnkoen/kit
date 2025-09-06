#!/bin/bash 2>/dev/null || goto :windows

# -------------------
# LINUX / MACOS PART
# -------------------

#!/bin/bash
set -e  # Exit on error

echo "Building shaders for Linux..."


mkdir -p vk  # Ensure output folder exists

shaderc -f model.vs -o vk/model.vs.bin --varyingdef model_varying.def --type v --platform linux --profile spirv
shaderc -f model.fs -o vk/model.fs.bin --varyingdef model_varying.def --type f --platform linux --profile spirv
shaderc -f skinned.vs -o vk/skinned.vs.bin --varyingdef skinned_varying.def --type v --platform linux --profile spirv
shaderc -f skinned.fs -o vk/skinned.fs.bin --varyingdef skinned_varying.def --type f --platform linux --profile spirv

echo "Done."
exit 0

# -------------------
# WINDOWS PART
# -------------------
:windows
@echo off
setlocal enabledelayedexpansion

echo Building shaders for Windows...

REM Ensure shaderc is available
where shaderc >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: shaderc not found. Ensure it is installed and in PATH.
    exit /b 1
)


REM Vulkan
if not exist vk mkdir vk
shaderc -f model.vs -o vk/model.vs.bin --varyingdef model_varying.def --type v --platform windows --profile spirv
shaderc -f model.fs -o vk/model.fs.bin --varyingdef model_varying.def --type f --platform windows --profile spirv
shaderc -f skinned.vs -o vk/skinned.vs.bin --varyingdef skinned_varying.def --type v --platform windows --profile spirv
shaderc -f skinned.fs -o vk/skinned.fs.bin --varyingdef skinned_varying.def --type f --platform windows --profile spirv

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

echo Done.
