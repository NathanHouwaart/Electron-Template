Conan profile for building Node.js native addons

This profile is intended to be used when building the project with Conan for Windows/MSVC
and ensures the correct runtime linkage so Node.js and the native addon share the C runtime
(/MD or /MDd). This prevents issues such as `std::cout`/`std::cerr` output not appearing
because of multiple CRT copies.

Usage (project-local):

powershell
```
# from project root
conan install . --output-folder=build --profile=conan_nodejs.profile --build=missing
# Use the generated toolchain file when configuring CMake
npx cmake-js build -- -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake
```

Notes:
- Verify `compiler.version` in `conan_nodejs.profile` matches your MSVC toolset (e.g., 193 for VS2022).
- If you prefer a named profile, copy this file into your Conan profiles directory (e.g., `%USERPROFILE%/.conan2/profiles/nodejs`) and use `--profile=nodejs`.
- If you target Debug builds, set `build_type=Debug` in the profile and Conan will use `/MDd` for the runtime.

CI suggestion:
- Add the profile to your repo as above and use the same `conan install` command in CI. It ensures reproduciable builds.

Troubleshooting:
- If `std::cout` is still not visible, ensure both the addon and all Conan dependencies are built with dynamic runtime (`/MD`).
- Inspect the generated `build/generators/conan_toolchain.cmake` to confirm runtime flags.
