# Using cmake-js with MinGW

## Quick Reference

### Local Build (on your Windows machine)

```powershell
# Simple build
npm run build:addon

# Clean rebuild
npm run build:addon:rebuild

# Or use npx directly
npx cmake-js compile -d native -G "MinGW Makefiles"
```

### Docker Build

```powershell
# Using cmake-js in Docker
.\docker\build-native-cmakejs.ps1 -Pull

# Clean build
.\docker\build-native-cmakejs.ps1 -CleanBuild
```

## Configuration

The `.cmake-js` file in the `native/` directory tells cmake-js to use MinGW:

```json
{
  "generator": "MinGW Makefiles",
  "preferMake": true
}
```

## Common Commands

```powershell
# Install Node headers (usually automatic)
npx cmake-js install -d native

# Configure only (no build)
npx cmake-js configure -d native -G "MinGW Makefiles"

# Build only (after configure)
npx cmake-js build -d native

# Full compile (configure + build)
npx cmake-js compile -d native -G "MinGW Makefiles"

# Clean
npx cmake-js clean -d native

# Rebuild (clean + compile)
npx cmake-js rebuild -d native -G "MinGW Makefiles"
```

## Why cmake-js vs raw cmake?

**cmake-js advantages:**
- Automatically downloads Node headers
- Handles Node.js/Electron version matching
- Simpler commands
- Better integration with npm ecosystem

**Raw cmake advantages:**
- More control over build process
- Faster (no Node.js process overhead)
- Better for CI/CD with cached headers

**Recommendation:** Use cmake-js for development, raw cmake for production builds in Docker.

## Troubleshooting

### "MSVC compiler not found"

**Problem:** cmake-js defaults to MSVC on Windows.

**Solution:** Always specify `-G "MinGW Makefiles"` or use the `.cmake-js` config file.

### "node-addon-api not found"

**Problem:** npm dependencies not installed.

**Solution:**
```powershell
npm install  # Install project deps
```

### "CMake not found"

**Problem:** CMake not in PATH.

**Solution:** Install CMake and ensure it's in PATH:
```powershell
# Check if cmake is available
cmake --version

# If not, install via Chocolatey
choco install cmake
```

### MinGW not found

**Problem:** MinGW not installed or not in PATH.

**Solution:**
```powershell
# Check if MinGW is available
g++ --version

# If not, install via Chocolatey
choco install mingw
```
