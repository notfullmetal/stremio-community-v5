@echo off
echo ============================================
echo   STREMIO UI AUTO-HIDE FIX - QUICK START
echo ============================================
echo.
echo This script will help you install the required tools.
echo Please run these commands manually for better control:
echo.
echo STEP 1: Install build tools
echo winget install Microsoft.VisualStudio.2022.Community
echo winget install Kitware.CMake
echo winget install OpenJS.NodeJS
echo.
echo STEP 2: Set up vcpkg (Run as Administrator)
echo New-Item -ItemType Directory -Path "C:\vcpkg" -Force
echo cd C:\vcpkg
echo git clone https://github.com/Microsoft/vcpkg.git .
echo .\bootstrap-vcpkg.bat
echo.
echo STEP 3: Install dependencies (in x64 Native Tools Command Prompt)
echo cd C:\vcpkg
echo vcpkg install openssl:x64-windows-static
echo vcpkg install nlohmann-json:x64-windows-static
echo vcpkg install webview2:x64-windows-static
echo vcpkg install curl:x64-windows-static
echo.
echo STEP 4: Build project (see BUILD_INSTRUCTIONS.md)
echo.
echo ============================================
echo   For detailed instructions, see BUILD_INSTRUCTIONS.md
echo ============================================
pause
