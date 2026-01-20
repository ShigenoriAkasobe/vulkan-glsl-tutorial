cmake_minimum_required(VERSION 3.26)

include(VgtSample)
include(VgtShaders)

vgt_add_step_executable(
  NAME Step05_LightingBasic
  SOURCES
    main.cpp
)

target_link_libraries(Step05_LightingBasic PRIVATE vgt::config)

vgt_add_glsl_shaders(Step05_LightingBasic
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/lighting.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/lighting.frag"
)
