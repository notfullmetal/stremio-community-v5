# 🚀 GitHub Actions Automated Build Guide

## Overview
I've set up GitHub Actions to automatically build your modified Stremio version and create releases. This eliminates the need to install build tools locally!

## 📁 Files Created
- `.github/workflows/build-release.yml` - Main build workflow
- `vcpkg.json` - Dependency configuration for vcpkg
- `GITHUB_ACTIONS_GUIDE.md` - This guide

## 🎯 How to Use

### Method 1: Create a Release (Recommended)

1. **Push your code to GitHub**:
   ```bash
   git add .
   git commit -m "Add UI auto-hide fix for paused media"
   git push origin main
   ```

2. **Create and push a version tag**:
   ```bash
   # Create a new version tag (follows Stremio versioning)
   git tag v5.0.19
   git push origin v5.0.19
   ```

3. **Automatic build starts**:
   - GitHub Actions will automatically detect the new tag
   - It will build your modified Stremio version
   - Create a GitHub release with the built executable

### Method 2: Manual Trigger

1. **Go to your GitHub repository**
2. **Click "Actions" tab**
3. **Select "Build and Release Stremio UI Auto-Hide Fix"**
4. **Click "Run workflow"**
5. **Fill in the form**:
   - Version: `v5.0.19` (or any version you want)
   - Create GitHub release: ✅ (checked)
6. **Click "Run workflow"**

## 📦 What Gets Built

The workflow automatically:
- ✅ Sets up Windows build environment (VS 2022)
- ✅ Installs all dependencies via vcpkg
- ✅ Downloads MPV libraries
- ✅ Downloads server.js and other requirements
- ✅ Builds your modified stremio.exe
- ✅ Creates a GitHub release with:
  - `stremio.exe` (your modified version)
  - `libmpv-2.dll` (required library)
  - `README.md` (installation instructions)
  - `VERSION.txt` (build information)

## 📥 Using the Built Version

Once the build completes:

1. **Go to your repository's "Releases" page**
2. **Download the latest release**
3. **Follow the installation instructions in the release notes**:
   ```powershell
   # Backup original
   cd "$env:LOCALAPPDATA\Programs\LNV\Stremio-5"
   Copy-Item "stremio.exe" "stremio.exe.backup"
   
   # Replace with your modified version
   Copy-Item "path\to\downloaded\stremio.exe" "stremio.exe"
   ```

## 🔄 Build Status

Check the build status:
- **Green ✅**: Build successful, release created
- **Red ❌**: Build failed, check the logs
- **Yellow 🟡**: Build in progress

Click on the workflow run to see detailed logs if there are any issues.

## 🛠️ Troubleshooting

### Common Issues:

1. **Build fails to find MPV libraries**:
   - The workflow automatically downloads them
   - If it fails, check the MPV download step in the logs

2. **vcpkg dependency installation fails**:
   - Usually a temporary network issue
   - Re-run the workflow

3. **Missing server.js**:
   - The workflow tries to download it automatically
   - Creates a dummy file if download fails
   - Build will still succeed

### Debug Steps:

1. **Check the Actions tab** for detailed logs
2. **Look at the "Verify build output" step** to see if stremio.exe was created
3. **Check artifact uploads** - even if release creation fails, artifacts are uploaded

## 🎨 Customization

### Modify the Version Number:
Edit `CMakeLists.txt`:
```cmake
project(stremio VERSION "5.0.18")  # Change this version
```

### Change Release Notes:
Edit the `body:` section in `.github/workflows/build-release.yml`

### Add More Files to Release:
Add them to the `files:` section in the workflow

## 🔐 Security Notes

- The workflow uses GitHub's built-in `GITHUB_TOKEN`
- No external secrets or credentials needed
- All builds happen in GitHub's secure environment
- Source code and dependencies are downloaded fresh each time

## 🎉 Benefits of This Approach

✅ **No local setup required** - Everything builds in the cloud
✅ **Reproducible builds** - Same environment every time  
✅ **Automatic releases** - Just push a tag and get a release
✅ **Build artifacts** - Even test builds are saved
✅ **Cross-platform** - Can easily add Linux/macOS builds later
✅ **Version control** - Every release tied to a specific commit
✅ **Professional workflow** - Same approach used by major projects

## 🚀 Next Steps

1. **Push your changes to GitHub**
2. **Create your first release** using Method 1 or 2 above
3. **Download and test** the built executable
4. **Share with others** - they can download from your releases page!

Your UI auto-hide fix will now be easily accessible to anyone who wants it! 🎊
