# third_party

This repository uses CMake `FetchContent` to obtain dependencies at configure time.

- `GLFW`: fetched from the official GitHub repository.
- `stb_image`: fetched from the official `stb` repository.

## Why

The goal is to keep the Windows + Visual Studio setup minimal:

- No manual installation of GLFW.
- Shader compilation uses `glslangValidator` from the Vulkan SDK.

## Notes

- `FetchContent` downloads sources into your build directory. Nothing is committed under `third_party/`.
- If you need fully offline builds, vendor the dependencies and replace `FetchContent` with `add_subdirectory`.
