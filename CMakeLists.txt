cmake_minimum_required(VERSION 3.26)

project(vulkan_glsl_tutorial LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep samples Windows-oriented (project_plan_windows.md).
if(NOT WIN32)
  message(WARNING "This repository is primarily intended for Windows + MSVC.")
endif()

option(VGT_ENABLE_VALIDATION "Enable Vulkan validation layers in samples" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(VgtCommon)
include(VgtDependencies)
include(VgtShaders)
include(VgtConfig)

# Steps
add_subdirectory(steps/Step00_ClearScreen)
add_subdirectory(steps/Step01_MinimalTriangle)
add_subdirectory(steps/Step02_VertexColor)
add_subdirectory(steps/Step03_Texture)
add_subdirectory(steps/Step04_Transform)
add_subdirectory(steps/Step05_LightingBasic)
