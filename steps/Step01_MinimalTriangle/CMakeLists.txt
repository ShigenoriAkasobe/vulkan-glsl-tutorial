cmake_minimum_required(VERSION 3.26)

include(VgtSample)
include(VgtShaders)

vgt_add_step_executable(
  NAME Step01_MinimalTriangle
  SOURCES
    main.cpp
)

target_link_libraries(Step01_MinimalTriangle PRIVATE vgt::config)

vgt_add_glsl_shaders(Step01_MinimalTriangle
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/triangle.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/triangle.frag"
)
