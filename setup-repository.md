# 🎯 Repository Setup for Automated Builds

## Quick Setup Commands

If you're setting up this repository for the first time, run these commands:

### 1. Initialize Git Repository (if not already done)
```bash
git init
git add .
git commit -m "Initial commit with UI auto-hide fix"
```

### 2. Connect to GitHub
```bash
# Replace YOUR_USERNAME and YOUR_REPO_NAME with your actual values
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
git branch -M main
git push -u origin main
```

### 3. Create Your First Release
```bash
# Create and push a version tag to trigger the first build (follows Stremio versioning)
git tag v5.0.19 -m "First release with community modifications"
git push origin v5.0.19
```

## What Happens Next

1. **GitHub Actions starts building** - Check the "Actions" tab in your repository
2. **Build takes 5-10 minutes** - It downloads dependencies and compiles everything
3. **Release is created automatically** - Check the "Releases" section
4. **Download and use** - Get your modified stremio.exe from the release!

## Repository Structure

After setup, your repository will have:
```
├── .github/
│   └── workflows/
│       ├── build-release.yml    # Main build and release workflow
│       └── test-build.yml       # Test builds on every push
├── src/
│   └── mpv/
│       └── player.cpp           # Your modified file with UI fix
├── vcpkg.json                   # Dependency configuration
├── CMakeLists.txt              # Build configuration
├── .gitignore                  # Excludes build artifacts
└── GITHUB_ACTIONS_GUIDE.md     # Usage instructions
```

## Testing Your Setup

Push any change to test the build system:
```bash
echo "# UI Auto-Hide Fix Test" >> README.md
git add README.md
git commit -m "Test automated build"
git push
```

This will trigger a test build (no release created) so you can verify everything works.

## 🎉 You're All Set!

Your repository now has professional automated builds that will:
- ✅ Build on every code change
- ✅ Create releases when you push tags
- ✅ Handle all dependencies automatically
- ✅ Provide easy installation for users
