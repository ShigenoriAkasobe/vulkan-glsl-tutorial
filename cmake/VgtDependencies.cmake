include(FetchContent)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW (build from source to keep setup minimal on Windows)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

add_library(vgt::glfw ALIAS glfw)

# stb_image (header-only)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb)

add_library(vgt_stb_image INTERFACE)
target_include_directories(vgt_stb_image INTERFACE "${stb_SOURCE_DIR}")
add_library(vgt::stb_image ALIAS vgt_stb_image)
