# 🚀 Building Modified Stremio

## ⚠️ Recommended: Use GitHub Actions Instead

**For the easiest experience, use the automated GitHub Actions build system instead of manual building.**
- See `GITHUB_ACTIONS_GUIDE.md` for instructions
- No local setup required - builds happen in the cloud
- Automatic releases and distribution

## Overview
This guide covers manual local building if you prefer to build yourself. Your modifications are ready to be compiled.

## 🎯 Option 1: Simple Replacement (Recommended)

Since you already have Stremio Desktop v5 installed at:
`C:\Users\dubey\AppData\Local\Programs\LNV\Stremio-5\`

We'll build only the modified executable and replace the existing one.

### Step 1: Install Build Tools

#### 1.1 Install Visual Studio Community 2022
```powershell
winget install Microsoft.VisualStudio.2022.Community
```
- During installation, make sure to select "Desktop development with C++" workload
- This includes MSVC compiler and Windows SDK

#### 1.2 Install CMake
```powershell
winget install Kitware.CMake
```

#### 1.3 Install Git (if not already installed)
```powershell
winget install Git.Git
```

#### 1.4 Install vcpkg (C++ Package Manager)
Open a new **Administrator PowerShell** and run:
```powershell
# Create vcpkg directory
New-Item -ItemType Directory -Path "C:\vcpkg" -Force
cd C:\vcpkg

# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git .

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integrate vcpkg globally (optional but recommended)
.\vcpkg integrate install
```

### Step 2: Install Project Dependencies

Open **x64 Native Tools Command Prompt for VS 2022** (search for it in Start Menu):

```cmd
cd C:\vcpkg

# Install required dependencies for x64
vcpkg install openssl:x64-windows-static
vcpkg install nlohmann-json:x64-windows-static
vcpkg install webview2:x64-windows-static
vcpkg install curl:x64-windows-static
```

### Step 3: Prepare Project Dependencies

#### 3.1 Download MPV Library
The project needs MPV libraries. Since the `deps/libmpv` folder is empty, download them:

```powershell
# Go to your project directory
cd "C:\Users\dubey\Downloads\Projects\stremio-community-v5"

# Create the required directory structure
New-Item -ItemType Directory -Path "deps\libmpv\x86_64" -Force

# Download MPV x64 library (you'll need to do this manually)
# Go to: https://sourceforge.net/projects/mpv-player-windows/files/libmpv/
# Download: mpv-dev-x86_64-*.7z
# Extract the contents to deps\libmpv\x86_64\
```

#### 3.2 Download Additional Required Files
```powershell
# Download server.js (you'll need to replace %package_version% with 5.0.18)
Invoke-WebRequest -Uri "https://s3-eu-west-1.amazonaws.com/stremio-artifacts/four/v5.0.18/server.js" -OutFile "utils\windows\server.js"

# Copy stremio-runtime.exe if not present
# This should already be in utils\windows\ based on your file structure
```

### Step 4: Build the Project

#### 4.1 Configure with CMake
Open **x64 Native Tools Command Prompt for VS 2022** and navigate to your project:

```cmd
cd "C:\Users\dubey\Downloads\Projects\stremio-community-v5"

# Create build directory
mkdir cmake-build-release-x64
cd cmake-build-release-x64

# Configure CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release
```

#### 4.2 Build the Project
```cmd
cmake --build . --config Release
```

### Step 5: Replace the Executable

#### 5.1 Backup Original
```powershell
# Navigate to Stremio installation
cd "$env:LOCALAPPDATA\Programs\LNV\Stremio-5"

# Backup original executable
Copy-Item "stremio.exe" "stremio.exe.backup"
```

#### 5.2 Replace with Modified Version
```powershell
# Copy your newly built executable
Copy-Item "C:\Users\dubey\Downloads\Projects\stremio-community-v5\cmake-build-release-x64\Release\stremio.exe" "stremio.exe"
```

### Step 6: Test Your Changes

1. **Launch Stremio**: Run `stremio.exe` from the installation directory
2. **Play any media**: Load and play any video content
3. **Test UI Auto-Hide**: 
   - During playback: UI should auto-hide after a few seconds ✅ (existing behavior)
   - When paused: UI should now also auto-hide after a few seconds ✅ (your new feature!)

---

## 🎯 Option 2: Full Build and Installation

If you prefer a complete fresh installation or the simple replacement doesn't work:

### Use the Deploy Script

The project includes a deployment script that handles everything:

```cmd
# Open x64 Native Tools Command Prompt for VS 2022
cd "C:\Users\dubey\Downloads\Projects\stremio-community-v5"

# Make sure you have Node.js installed
winget install OpenJS.NodeJS

# Run the deployment script
cd build
node deploy_windows.js --installer
```

This will create a complete installer with your modifications.

---

## 🛠️ Troubleshooting

### Common Issues:

1. **CMake not found**: Restart your command prompt after installing CMake
2. **MSVC not found**: Make sure you use "x64 Native Tools Command Prompt for VS 2022"
3. **vcpkg packages not found**: Verify vcpkg integration with `vcpkg integrate list`
4. **MPV library missing**: Ensure libmpv-2.dll is in deps\libmpv\x86_64\
5. **Build fails**: Check that all dependencies are installed for x64-windows-static

### Verification Commands:

```cmd
# Check CMake
cmake --version

# Check MSVC
cl

# Check vcpkg integration
vcpkg integrate list

# Check if libmpv exists
dir deps\libmpv\x86_64\libmpv-2.dll
```

---

## 🎉 Success!

Once completed, your modified Stremio will have the UI auto-hide behavior you requested:
- ✅ UI auto-hides during playback (existing)
- ✅ UI auto-hides when paused (new feature)
- ✅ All other functionality remains intact

The change you made in `src/mpv/player.cpp` ensures the frontend always receives `pause=false` for UI behavior while maintaining actual pause functionality internally.
