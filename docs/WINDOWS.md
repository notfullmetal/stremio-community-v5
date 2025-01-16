
# Building on Windows

---

## 🚀 Quick Overview

This guide walks you through the process of building Stremio on Windows. Follow the steps carefully to set up the environment, build dependencies, and compile the project.

---

## 🛠️ Requirements

Ensure the following are installed on your system:

- **Operating System**: Windows 7 or newer
- **Utilities**: [7zip](https://www.7-zip.org/) or similar
- **Tools**:
    - [Git](https://git-scm.com/download/win)
    - [Microsoft Visual Studio](https://visualstudio.microsoft.com/)
    - [CMake](https://cmake.org/)
    - [WebView2](https://developer.microsoft.com/de-de/microsoft-edge/webview2/?ch=1&form=MA13LH)
    - [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)
    - [Node.js](https://nodejs.org/)
    - [FFmpeg](https://ffmpeg.org/download.html)
    - [MPV](https://sourceforge.net/projects/mpv-player-windows/)

---

## 📂 Setup Guide

### 1️⃣ **Install Essential Tools**
- **Git**: [Download](https://git-scm.com/download/win) and install.
- **Visual Studio**: [Download Community 2022](https://visualstudio.microsoft.com/de/downloads/).
- **Vcpkg**: [Download](https://github.com/microsoft/vcpkg)

---

### 2️⃣ **Install dependency**


1. Download vcpkg [here](https://github.com/microsoft/vcpkg).

2. Install using vcpkg ``openssl:x64-windows-static``, ``nlohmann-json:x64-windows-static``, ``webview2:x64-windows-static`` and ``curl:x64-windows-static``

> **⏳ Note:** To deploy x86 install the x86 versions of the dependencies above.


---

### 3️⃣ **Prepare the MPV Library**

- **Optional step as mpv is included as submodule:**
  - Download the MPV library: [MPV libmpv](https://sourceforge.net/projects/mpv-player-windows/files/libmpv/).
  - Use the `mpv-x86_64` version.
> **⏳ Note:** The submodule https://github.com/Zaarrg/libmpv already includes .lib, just make sure to unzip the actual .dll for x64 systems.
---

### 4️⃣ **Clone and Configure the Repository**

1. Clone the repository:
   ```cmd
   git clone --recursive git@github.com:Zaarrg/stremio-desktop-v5.git
   cd stremio-dekstop-v5
   ```
2. Update CMake Options:
   ```cmd
    -DCMAKE_TOOLCHAIN_FILE=C:/bin/vcpkg/scripts/buildsystems/vcpkg.cmake 
    -DVCPKG_TARGET_TRIPLET=x64-windows-static
   ```
   As Oneliner:
    ```cmd
   -DCMAKE_TOOLCHAIN_FILE=C:\bin\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
   ```
3. Download the `server.js` file:
   ```cmd
   powershell -Command Start-BitsTransfer -Source "https://s3-eu-west-1.amazonaws.com/stremio-artifacts/four/v%package_version%/server.js" -Destination server.js
   ```

> **⏳ Note:** To run the cmake project make sure to add `stremio-runtime.exe, server.js` beside the `stremio.exe`

---

### 5️⃣ **Build / Deploying the Shell**

1. Make sure to run the following in the `x64 Native Tools Command Prompt for VS 2022` or `x86 Native Tools Command Prompt for VS 2022`

   2. Run the deployment script in the ``build`` folder. By default builds ``x64``
      ```cmd
      node deploy_windows.js --installer
      ```
   - For Portable (Needs ``7zip`` and ``/utils/WebviewRuntime/x64/EdgeWebView`` )
     ```cmd
      node deploy_windows.js --portable
      ```
   
   - For x86
     ```cmd
     node deploy_windows.js --x86 --installer
     ```
   - For Portable x86 (Needs ``7zip`` and ``/utils/WebviewRuntime/x86/EdgeWebView`` )
     ```cmd
      node deploy_windows.js --x86 --portable
      ```

2. **Requirements**
   - To run the deploy_window.js script successfully the following is always required:
     1. ``libmpv-2.dll`` in ``deps/libmpv/i686`` for ``x86`` or ``deps/libmpv/x86_64`` for ``x64``
     2. ``server.js`` and ``stremio-runtime.exe`` in ``utils/windows``
     3. ``ffmpeg`` folder with ffmpeg and .dlls in ``utils/windows/ffmpeg``
     4. ``anime4k``  in ``utils/mpv/anime4k/portable_config``
     5. ``stremio-settings.ini`` as default settings in ``utils/stremio``
     6. ```vcpkg toolchain``` in `C:/bin/vcpkg/scripts/buildsystems/vcpkg.cmake` or change the `VCPKG_CMAKE` path in the script.
     7. ``cmake-build-release-x64`` or ``cmake-build-release-x86`` folders in project root. Run cmake once before deployment for this.
   - For the installer
     1. ``nsis`` in ``C:\Program Files (x86)\NSIS\makensis.exe`` or change path manually
     2. ``nsprocess`` nsis plugin. Can be found in ``utils/window/NsProcess``. Drop the contents into your nsis install folder.
   - For Portable
     1. ``7zip`` in ``C:\Program Files\7-Zip\7z.exe`` or ``C:\Program Files (x86)\7-Zip\7z.exe``
     2. ``WebView2 Runtime`` in ``utils/windows/WebviewRuntime/x64/EdgeWebView`` or ``../x86/EdgeWebView``. The Runtime can be found at ``C:\Program Files (x86)\Microsoft\EdgeWebView\Application\131.0.2903.146``. Copy the contents of this to our ``utils/windows/WebviewRuntime/x64/EdgeWebView``. For ``x86`` install ``WebView2 Runtime x86`` and do the same.


> **⏳ Note:** This script uses common path for ``DCMAKE_TOOLCHAIN_FILE`` of vcpkg, make sure the script has the correct one. If running with ``--installer`` make sure u installed ``nsis`` with the needed ``nsprocess`` plugin at least once.

3. Done. This will build the `installer` and ``dist/win`` folder.

> **⏳ Note:** This will create `dist/win` with all necessary files like `node.exe`, `ffmpeg.exe`. Also make sure to have `stremio-runtime.exe, server.js` are in `utils\windows\` folder
---

## 📦 Installer (Optional)

1. Download and install [NSIS](https://nsis.sourceforge.io/Download).  
   Default path: `C:\Program Files (x86)\NSIS`.

2. Generate the installer:
   ```cmd
    FOR /F "tokens=4 delims=() " %i IN ('findstr /C:"project(stremio VERSION" CMakeLists.txt') DO @set "package_version=%~i"
   set arch = "x64";
   "C:\Program Files (x86)\NSIS\makensis.exe" utils\windows\installer\windows-installer.nsi
   ```
    - Result: `Stremio %package_version%.exe`.

---

## 🔧 Silent Installation

Run the installer with `/S` (silent mode) and configure via these options:

- `/notorrentassoc`: Skip `.torrent` association.
- `/nodesktopicon`: Skip desktop shortcut.

Silent uninstall:
```cmd
"%LOCALAPPDATA%\Programs\LNV\Stremio-5\Uninstall.exe" /S /keepdata
```

---

✨ **Happy Building!**
