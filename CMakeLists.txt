cmake_minimum_required(VERSION 3.20)

project(vulkan_glsl_tutorial LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VGT_ENABLE_VALIDATION "Enable Vulkan validation layers (debug builds recommended)" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)

find_package(glfw3 CONFIG QUIET)
if (NOT glfw3_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
endif()

find_program(GLSLANG_VALIDATOR glslangValidator)
if (NOT GLSLANG_VALIDATOR)
  message(WARNING "glslangValidator not found. Shader compilation targets will be skipped.")
endif()

add_library(vgt_build_deps INTERFACE)
target_link_libraries(vgt_build_deps INTERFACE Vulkan::Vulkan)
if (glfw3_FOUND)
  target_link_libraries(vgt_build_deps INTERFACE glfw)
else()
  target_link_libraries(vgt_build_deps INTERFACE PkgConfig::GLFW3)
endif()

target_compile_definitions(vgt_build_deps INTERFACE
  $<$<BOOL:${VGT_ENABLE_VALIDATION}>:VGT_ENABLE_VALIDATION=1>
)

include(ShaderCompile)

add_subdirectory(steps/Step00_ClearScreen)
add_subdirectory(steps/Step01_MinimalTriangle)
add_subdirectory(steps/Step02_VertexColor)
add_subdirectory(steps/Step03_Texture)
add_subdirectory(steps/Step04_Transform)
add_subdirectory(steps/Step05_LightingBasic)
