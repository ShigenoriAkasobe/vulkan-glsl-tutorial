cmake_minimum_required(VERSION 3.26)

include(VgtSample)
include(VgtShaders)

vgt_add_step_executable(
  NAME Step02_VertexColor
  SOURCES
    main.cpp
)

target_link_libraries(Step02_VertexColor PRIVATE vgt::config)

vgt_add_glsl_shaders(Step02_VertexColor
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/vertex_color.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/vertex_color.frag"
)
