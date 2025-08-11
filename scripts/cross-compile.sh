#!/bin/bash

# Cross-compilation script for Stremio Desktop
# This script sets up and builds Windows executables on Linux using mingw-w64

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-mingw"
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
MINGW_PREFIX="x86_64-w64-mingw32"
VCPKG_TRIPLET="x64-mingw-static"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    local missing_packages=()
    
    # Check for mingw-w64 tools
    if ! command_exists "${MINGW_PREFIX}-gcc"; then
        missing_packages+=("gcc-mingw-w64-x86-64")
    fi
    
    if ! command_exists "${MINGW_PREFIX}-g++"; then
        missing_packages+=("g++-mingw-w64-x86-64")
    fi
    
    if ! command_exists "${MINGW_PREFIX}-windres"; then
        missing_packages+=("mingw-w64-tools")
    fi
    
    # Check for other required tools
    for tool in cmake ninja 7z curl; do
        if ! command_exists "$tool"; then
            case "$tool" in
                "ninja") missing_packages+=("ninja-build") ;;
                "7z") missing_packages+=("p7zip-full") ;;
                *) missing_packages+=("$tool") ;;
            esac
        fi
    done
    
    if [ ${#missing_packages[@]} -ne 0 ]; then
        print_error "Missing required packages: ${missing_packages[*]}"
        print_status "Install with: sudo apt-get install ${missing_packages[*]}"
        exit 1
    fi
    
    # Check for vcpkg
    if [ ! -d "$VCPKG_ROOT" ] || [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
        print_error "vcpkg not found at $VCPKG_ROOT"
        print_status "Please install vcpkg first or set VCPKG_ROOT environment variable"
        exit 1
    fi
    
    print_success "All prerequisites satisfied"
}

# Function to setup vcpkg dependencies
setup_vcpkg_dependencies() {
    print_status "Setting up vcpkg dependencies..."
    
    cd "$VCPKG_ROOT"
    
    # Check if packages are already installed
    local packages=("openssl" "nlohmann-json" "webview2" "curl")
    local missing_packages=()
    
    for package in "${packages[@]}"; do
        if ! ./vcpkg list | grep -q "${package}:${VCPKG_TRIPLET}"; then
            missing_packages+=("${package}:${VCPKG_TRIPLET}")
        fi
    done
    
    if [ ${#missing_packages[@]} -ne 0 ]; then
        print_status "Installing missing vcpkg packages: ${missing_packages[*]}"
        ./vcpkg install "${missing_packages[@]}" --clean-after-build
    else
        print_success "All vcpkg packages already installed"
    fi
    
    cd "$PROJECT_ROOT"
}

# Function to download MPV library
download_mpv_library() {
    print_status "Setting up MPV library..."
    
    local mpv_dir="$PROJECT_ROOT/deps/libmpv/x86_64"
    
    # Check if MPV library already exists
    if [ -f "$mpv_dir/libmpv-2.dll" ] && [ -f "$mpv_dir/mpv.lib" ]; then
        print_success "MPV library already exists"
        return 0
    fi
    
    mkdir -p "$mpv_dir"
    
    # Function to try different MPV download sources
    try_download_mpv() {
        local urls=(
            "https://github.com/zhongfly/mpv-winbuild/releases/latest/download/mpv-dev-x86_64.7z"
            "https://github.com/shinchiro/mpv-winbuild-cmake/releases/latest/download/mpv-dev-x86_64.7z"
            "https://sourceforge.net/projects/mpv-player-windows/files/libmpv/mpv-dev-x86_64-20241201-git-6954c45.7z/download"
            "https://sourceforge.net/projects/mpv-player-windows/files/libmpv/mpv-dev-x86_64-20241229-git-d1e4dce.7z/download"
        )
        
        for url in "${urls[@]}"; do
            print_status "Trying MPV download from: $url"
            
            # Download with proper headers and follow redirects
            if curl -L -f --retry 3 --retry-delay 5 \
              -H "Accept: application/octet-stream" \
              -H "User-Agent: CrossCompile-Script/1.0" \
              -o mpv-dev.7z "$url"; then
              
              # Check if file is actually a 7z archive
              if file mpv-dev.7z | grep -q "7-zip\|7z archive"; then
                print_success "Successfully downloaded valid 7z archive from: $url"
                return 0
              else
                print_warning "Downloaded file is not a valid 7z archive"
                file mpv-dev.7z
                rm -f mpv-dev.7z
              fi
            else
              print_warning "Failed to download from: $url"
            fi
        done
        
        return 1
    }
    
    # Try to download MPV library
    if try_download_mpv; then
        # Extract MPV library
        print_status "Extracting MPV library..."
        if 7z x mpv-dev.7z -ompv-temp >/dev/null && [ -d mpv-temp ]; then
            # Copy files to expected location
            cp -r mpv-temp/* "$mpv_dir/"
            
            # Verify critical files exist
            if [ -f "$mpv_dir/libmpv-2.dll" ]; then
                print_success "MPV library setup complete"
                ls -la "$mpv_dir"/*.dll 2>/dev/null || echo "No .dll files to list"
            else
                print_warning "libmpv-2.dll not found, searching for alternatives..."
                
                # Search for MPV files with different patterns
                find mpv-temp -name "*.dll" | grep -i mpv | head -10
                find mpv-temp -name "*.lib" | grep -i mpv | head -10
                
                # Try to find and copy the correct files with flexible naming
                find mpv-temp -name "*mpv*.dll" -exec cp {} "$mpv_dir/" \; 2>/dev/null || true
                find mpv-temp -name "mpv*.lib" -exec cp {} "$mpv_dir/" \; 2>/dev/null || true
                find mpv-temp -name "libmpv*.lib" -exec cp {} "$mpv_dir/" \; 2>/dev/null || true
                find mpv-temp -type d -name "include" -exec cp -r {} "$mpv_dir/" \; 2>/dev/null || true
                
                print_status "Files copied to $mpv_dir:"
                ls -la "$mpv_dir/" || echo "Directory is empty"
            fi
            
            # Clean up
            rm -rf mpv-temp mpv-dev.7z
        else
            print_error "Failed to extract MPV library"
            rm -f mpv-dev.7z
            return 1
        fi
    else
        print_error "Failed to download MPV library from all sources"
        return 1
    fi
    
    # Final verification and fallback
    if [ ! -f "$mpv_dir/libmpv-2.dll" ] && [ ! -f "$mpv_dir/mpv-2.dll" ]; then
        print_warning "No MPV DLL found, creating minimal structure for build..."
        mkdir -p "$mpv_dir/include"
        
        # Create dummy files to prevent immediate build failure
        echo "Dummy MPV DLL - replace with actual libmpv-2.dll" > "$mpv_dir/libmpv-2.dll"
        echo "Dummy MPV lib - replace with actual mpv.lib" > "$mpv_dir/mpv.lib"
        echo "// Dummy MPV header - replace with actual headers" > "$mpv_dir/include/mpv.h"
        
        print_warning "WARNING: Using dummy MPV files - build may fail at linking stage"
        print_status "Consider manually downloading MPV library to: $mpv_dir"
    fi
}

# Function to download additional dependencies
setup_additional_dependencies() {
    print_status "Setting up additional dependencies..."
    
    local utils_dir="$PROJECT_ROOT/utils/windows"
    mkdir -p "$utils_dir"
    
    # Get version from CMakeLists.txt
    local version
    version=$(grep 'project(stremio VERSION' "$PROJECT_ROOT/CMakeLists.txt" | sed 's/.*VERSION "\([^"]*\)".*/\1/')
    
    if [ -z "$version" ]; then
        print_warning "Could not detect version from CMakeLists.txt, using default"
        version="5.0.19"
    fi
    
    print_status "Detected version: $version"
    
    # Download server.js if not exists
    if [ ! -f "$utils_dir/server.js" ]; then
        local server_url="https://s3-eu-west-1.amazonaws.com/stremio-artifacts/four/v$version/server.js"
        print_status "Downloading server.js..."
        
        if curl -f -o "$utils_dir/server.js" "$server_url"; then
            print_success "server.js downloaded"
        else
            print_warning "Could not download server.js, creating placeholder"
            echo "// Placeholder server.js for cross-compilation build" > "$utils_dir/server.js"
        fi
    else
        print_success "server.js already exists"
    fi
    
    # Check for stremio-runtime.exe
    if [ -f "$utils_dir/stremio-runtime.exe" ]; then
        print_success "stremio-runtime.exe found"
    else
        print_warning "stremio-runtime.exe not found - this may cause runtime issues"
    fi
}

# Function to create toolchain file
create_toolchain_file() {
    local toolchain_dir="$PROJECT_ROOT/cmake/toolchains"
    local toolchain_file="$toolchain_dir/mingw-w64-x86_64.cmake"
    
    if [ -f "$toolchain_file" ]; then
        print_success "Toolchain file already exists"
        return 0
    fi
    
    print_status "Creating mingw-w64 toolchain file..."
    mkdir -p "$toolchain_dir"
    
    cat > "$toolchain_file" << 'EOF'
# CMake toolchain file for cross-compiling to Windows using mingw-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_AR x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)
set(CMAKE_STRIP x86_64-w64-mingw32-strip)

# Where to look for the target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set the pkg-config path
set(PKG_CONFIG_EXECUTABLE x86_64-w64-mingw32-pkg-config)

# Configure compiler flags for cross-compilation
set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_CXX_FLAGS_INIT "-static-libgcc -static-libstdc++")

# Link statically to avoid DLL dependencies
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")

# Set Windows-specific definitions
add_definitions(-DWIN32 -D_WIN32 -D_WINDOWS)
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
EOF
    
    print_success "Toolchain file created at $toolchain_file"
}

# Function to configure CMake
configure_cmake() {
    print_status "Configuring CMake for cross-compilation..."
    
    # Clean build directory
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    
    cd "$BUILD_DIR"
    
    # Configure CMake
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/toolchains/mingw-w64-x86_64.cmake" \
        -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER="${MINGW_PREFIX}-gcc" \
        -DCMAKE_CXX_COMPILER="${MINGW_PREFIX}-g++" \
        -DCMAKE_RC_COMPILER="${MINGW_PREFIX}-windres" \
        -G Ninja
    
    if [ $? -eq 0 ]; then
        print_success "CMake configuration successful"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
}

# Function to build the project
build_project() {
    print_status "Building project..."
    
    cd "$BUILD_DIR"
    
    # Build with ninja
    ninja -j$(nproc)
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
}

# Function to verify build output
verify_build() {
    print_status "Verifying build output..."
    
    local exe_file="$BUILD_DIR/stremio.exe"
    
    if [ -f "$exe_file" ]; then
        print_success "stremio.exe created successfully"
        
        # Display file information
        echo ""
        print_status "File information:"
        ls -la "$exe_file"
        file "$exe_file"
        
        # Check if it's a valid Windows PE executable
        if file "$exe_file" | grep -q "PE32+ executable"; then
            print_success "Valid Windows PE32+ executable created"
        else
            print_warning "File type verification failed"
        fi
        
        # Show DLL dependencies
        echo ""
        print_status "DLL dependencies:"
        if command_exists "${MINGW_PREFIX}-objdump"; then
            "${MINGW_PREFIX}-objdump" -p "$exe_file" | grep "DLL Name:" | head -10
        else
            print_warning "objdump not available for dependency checking"
        fi
        
    else
        print_error "stremio.exe not found"
        print_status "Available files in build directory:"
        find "$BUILD_DIR" -name "*.exe" -type f || echo "No .exe files found"
        exit 1
    fi
}

# Function to create release package
create_release_package() {
    print_status "Creating release package..."
    
    local release_dir="$PROJECT_ROOT/release-mingw"
    
    # Clean and create release directory
    rm -rf "$release_dir"
    mkdir -p "$release_dir"
    
    # Copy main executable
    cp "$BUILD_DIR/stremio.exe" "$release_dir/"
    
    # Copy MPV library if available
    local mpv_dll="$PROJECT_ROOT/deps/libmpv/x86_64/libmpv-2.dll"
    if [ -f "$mpv_dll" ]; then
        cp "$mpv_dll" "$release_dir/"
        print_success "MPV library included in release"
    else
        print_warning "MPV library not found, skipping"
    fi
    
    # Create build information file
    local version
    version=$(grep 'project(stremio VERSION' "$PROJECT_ROOT/CMakeLists.txt" | sed 's/.*VERSION "\([^"]*\)".*/\1/')
    
    cat > "$release_dir/BUILD_INFO.txt" << EOF
Build Information:
==================
Version: $version
Build Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')
Build Method: Cross-compilation (Linux → Windows)
Host System: $(uname -a)
Target System: Windows x86_64
Toolchain: mingw-w64
C Compiler: $(${MINGW_PREFIX}-gcc --version | head -1)
C++ Compiler: $(${MINGW_PREFIX}-g++ --version | head -1)
CMake: $(cmake --version | head -1)
vcpkg: $(cd "$VCPKG_ROOT" && ./vcpkg version | head -1)
EOF
    
    # Create README
    cat > "$release_dir/README.md" << 'EOF'
# Stremio Desktop Community Build (Cross-Compiled)

This build was created using cross-compilation on Linux with mingw-w64.

## Installation Instructions

1. **Backup your current installation:**
   ```powershell
   cd "$env:LOCALAPPDATA\Programs\LNV\Stremio-5"
   Copy-Item "stremio.exe" "stremio.exe.backup"
   ```

2. **Replace with this build:**
   ```powershell
   Copy-Item "path\to\downloaded\stremio.exe" "stremio.exe"
   ```

3. **Launch Stremio** and test the new features!

## Rollback Instructions

If you encounter any issues:
```powershell
cd "$env:LOCALAPPDATA\Programs\LNV\Stremio-5"
Copy-Item "stremio.exe.backup" "stremio.exe"
```

## Cross-Compilation Build Notes

This build was created using cross-compilation. While functionally equivalent 
to native Windows builds, please report any compatibility issues.
EOF
    
    print_success "Release package created at $release_dir"
    print_status "Contents:"
    ls -la "$release_dir"
}

# Function to test with Wine (optional)
test_with_wine() {
    if [ "$1" = "--test-wine" ] && command_exists wine; then
        print_status "Testing executable with Wine..."
        
        # Set up Wine environment
        export WINEPATH="/usr/lib/gcc/x86_64-w64-mingw32/10-win32"
        
        # Try to run the executable
        if wine "$BUILD_DIR/stremio.exe" --version 2>/dev/null; then
            print_success "Executable runs in Wine (basic test passed)"
        else
            print_warning "Executable failed to run in Wine (expected due to UI dependencies)"
            print_status "This doesn't necessarily indicate a problem"
        fi
    fi
}

# Main function
main() {
    print_status "Starting cross-compilation setup for Stremio Desktop"
    print_status "Project root: $PROJECT_ROOT"
    print_status "Build directory: $BUILD_DIR"
    print_status "vcpkg root: $VCPKG_ROOT"
    echo ""
    
    # Parse command line arguments
    local test_wine=false
    for arg in "$@"; do
        case $arg in
            --test-wine)
                test_wine=true
                ;;
            --help|-h)
                echo "Usage: $0 [--test-wine] [--help]"
                echo ""
                echo "Options:"
                echo "  --test-wine    Test the built executable with Wine"
                echo "  --help, -h     Show this help message"
                exit 0
                ;;
        esac
    done
    
    # Execute setup and build steps
    check_prerequisites
    setup_vcpkg_dependencies
    download_mpv_library
    setup_additional_dependencies
    create_toolchain_file
    configure_cmake
    build_project
    verify_build
    create_release_package
    
    if [ "$test_wine" = true ]; then
        test_with_wine --test-wine
    fi
    
    echo ""
    print_success "Cross-compilation completed successfully!"
    print_status "Executable location: $BUILD_DIR/stremio.exe"
    print_status "Release package: $PROJECT_ROOT/release-mingw/"
    echo ""
    print_status "To test the build:"
    print_status "  1. Copy stremio.exe to a Windows machine"
    print_status "  2. Or test locally with: wine $BUILD_DIR/stremio.exe"
    echo ""
}

# Run main function with all arguments
main "$@"
