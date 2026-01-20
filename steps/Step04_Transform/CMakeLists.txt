cmake_minimum_required(VERSION 3.26)

include(VgtSample)
include(VgtShaders)

vgt_add_step_executable(
  NAME Step04_Transform
  SOURCES
    main.cpp
)

target_link_libraries(Step04_Transform PRIVATE vgt::config)

vgt_add_glsl_shaders(Step04_Transform
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/transform.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/transform.frag"
)
