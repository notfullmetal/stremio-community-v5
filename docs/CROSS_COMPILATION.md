# Cross-Compilation Guide for Stremio Desktop

This guide explains how to cross-compile Stremio Desktop for Windows on Ubuntu/Linux using mingw-w64.

---

## 🚀 Quick Overview

Cross-compilation allows you to build Windows `.exe` files on Linux systems without needing a Windows machine or VM. This is particularly useful for:

- **CI/CD Pipelines**: Build Windows binaries on Ubuntu runners
- **Development**: Linux developers can build Windows binaries locally
- **Consistency**: Reproducible builds across different environments
- **Cost Efficiency**: No need for Windows build agents

---

## 🛠️ Prerequisites

### System Requirements

- **Ubuntu 20.04+** or compatible Linux distribution
- **At least 4GB RAM** for compilation
- **10GB free disk space** for dependencies and build artifacts

### Required Packages

```bash
sudo apt-get update
sudo apt-get install -y \
  gcc-mingw-w64-x86-64 \
  g++-mingw-w64-x86-64 \
  mingw-w64-tools \
  mingw-w64-common \
  wine64 \
  p7zip-full \
  curl \
  git \
  build-essential \
  pkg-config \
  cmake \
  ninja-build \
  unzip \
  tar \
  zip
```

---

## 📦 Setup Guide

### 1️⃣ **Install vcpkg**

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh

# Add to PATH (optional but recommended)
echo 'export PATH="$HOME/vcpkg:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### 2️⃣ **Install Dependencies**

```bash
cd ~/vcpkg

# Install Windows cross-compilation dependencies
./vcpkg install openssl:x64-mingw-static
./vcpkg install nlohmann-json:x64-mingw-static
./vcpkg install webview2:x64-mingw-static
./vcpkg install curl:x64-mingw-static

# Verify installation
./vcpkg list
```

### 3️⃣ **Prepare the Repository**

```bash
# Clone the repository
git clone https://github.com/Zaarrg/stremio-desktop-v5.git
cd stremio-desktop-v5

# Initialize submodules
git submodule update --init --recursive

# Create toolchain directory
mkdir -p cmake/toolchains
```

### 4️⃣ **Download MPV Library**

```bash
# Create MPV directory
mkdir -p deps/libmpv/x86_64

# Download MPV library for Windows
curl -L -o mpv-dev.7z "https://sourceforge.net/projects/mpv-player-windows/files/libmpv/mpv-dev-x86_64-20241229-git-d1e4dce.7z/download"

# Extract MPV library
7z x mpv-dev.7z -ompv-temp
cp -r mpv-temp/* deps/libmpv/x86_64/

# Clean up
rm -rf mpv-temp mpv-dev.7z
```

### 5️⃣ **Download Additional Dependencies**

```bash
# Create utils directory
mkdir -p utils/windows

# Download server.js (replace VERSION with actual version)
VERSION="5.0.19"
curl -o utils/windows/server.js "https://s3-eu-west-1.amazonaws.com/stremio-artifacts/four/v$VERSION/server.js"

# Verify stremio-runtime.exe exists in repository
ls -la utils/windows/stremio-runtime.exe
```

---

## 🔨 Building

### Option 1: Using CMake Presets (Recommended)

```bash
# Configure the build
cmake --preset mingw-w64-cross-compile \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$PWD/cmake/toolchains/mingw-w64-x86_64.cmake"

# Build the project
cmake --build --preset mingw-w64-cross-compile --parallel $(nproc)
```

### Option 2: Manual CMake Configuration

```bash
# Create build directory
mkdir -p build-mingw
cd build-mingw

# Configure CMake
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="../cmake/toolchains/mingw-w64-x86_64.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
  -G Ninja

# Build the project
ninja
```

### Option 3: Using Helper Script

```bash
# Make the script executable
chmod +x scripts/cross-compile.sh

# Run cross-compilation
./scripts/cross-compile.sh
```

---

## 🔍 Verification

### Check Build Output

```bash
# Verify the executable was created
ls -la build-mingw/stremio.exe

# Check file type
file build-mingw/stremio.exe
# Should output: PE32+ executable (GUI) x86-64, for MS Windows

# Check dependencies
x86_64-w64-mingw32-objdump -p build-mingw/stremio.exe | grep "DLL Name:"
```

### Test with Wine (Optional)

```bash
# Install Wine if not already installed
sudo apt-get install wine64

# Configure Wine
winecfg

# Test the executable (may fail due to missing Windows UI components)
wine build-mingw/stremio.exe --version
```

---

## 🐛 Troubleshooting

### Common Issues

#### **1. Missing mingw-w64 Packages**
```bash
# Error: x86_64-w64-mingw32-gcc not found
sudo apt-get install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

#### **2. vcpkg Package Installation Fails**
```bash
# Clean vcpkg and reinstall
cd ~/vcpkg
./vcpkg remove --triplet x64-mingw-static --recurse
./vcpkg install openssl:x64-mingw-static --clean-after-build
```

#### **3. CMake Configuration Errors**
```bash
# Verify toolchain file exists
ls -la cmake/toolchains/mingw-w64-x86_64.cmake

# Check vcpkg integration
~/vcpkg/vcpkg integrate install
```

#### **4. Linking Errors**
```bash
# Check library paths
x86_64-w64-mingw32-ld --verbose | grep SEARCH_DIR

# Verify vcpkg packages
~/vcpkg/vcpkg list | grep x64-mingw
```

#### **5. MPV Library Issues**
```bash
# Verify MPV files exist
ls -la deps/libmpv/x86_64/
# Should contain: libmpv-2.dll, mpv.lib, include/

# Re-download if necessary
rm -rf deps/libmpv/x86_64/*
# (repeat download steps from setup)
```

### Debug Build Information

```bash
# Enable verbose CMake output
cmake --build build-mingw --verbose

# Check compiler version
x86_64-w64-mingw32-gcc --version
x86_64-w64-mingw32-g++ --version

# Verify vcpkg toolchain
cat ~/vcpkg/scripts/buildsystems/vcpkg.cmake | head -20
```

---

## 🚀 GitHub Actions Integration

The cross-compilation setup is automatically integrated with GitHub Actions. See `.github/workflows/cross-compile-windows.yml` for the complete workflow.

### Triggering Builds

- **Push to main/master**: Automatic build
- **Create tag**: Build + Release
- **Manual trigger**: Use GitHub Actions UI

### Artifacts

- **Executable**: `stremio.exe`
- **Dependencies**: `libmpv-2.dll` (if available)
- **Documentation**: `README.md`, `BUILD_INFO.txt`

---

## 📚 Technical Details

### Cross-Compilation Toolchain

- **Compiler**: gcc-mingw-w64 (GCC cross-compiler for Windows)
- **Target Architecture**: x86_64 (64-bit Windows)
- **C Runtime**: Static linking to minimize dependencies
- **Resource Compiler**: windres for .rc files

### Dependencies

- **vcpkg Triplet**: `x64-mingw-static`
- **Static Linking**: Reduces DLL dependencies
- **WebView2**: Windows runtime component
- **MPV**: Media player library for video playback

### Build Differences

| Aspect | Native Windows | Cross-Compilation |
|--------|----------------|-------------------|
| Compiler | MSVC | mingw-w64-gcc |
| Runtime | Dynamic MSVC | Static mingw |
| Libraries | .lib files | .a files |
| Linking | Dynamic | Static preferred |
| Build Time | Slower | Faster |

---

## 🤝 Contributing

### Improving Cross-Compilation

1. **Test on different Linux distributions**
2. **Optimize dependency management**
3. **Add support for 32-bit Windows targets**
4. **Improve Wine testing integration**

### Reporting Issues

When reporting cross-compilation issues, please include:

- **Host OS**: Linux distribution and version
- **mingw-w64 version**: `x86_64-w64-mingw32-gcc --version`
- **CMake version**: `cmake --version`
- **vcpkg status**: `~/vcpkg/vcpkg list`
- **Error logs**: Full CMake and build output

---

## 🔗 References

- [mingw-w64 Documentation](http://mingw-w64.org/)
- [vcpkg Documentation](https://vcpkg.io/)
- [CMake Cross-Compilation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
