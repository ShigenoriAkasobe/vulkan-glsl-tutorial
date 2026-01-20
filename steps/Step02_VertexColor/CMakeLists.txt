add_executable(Step02_VertexColor
  main.cpp
)

target_link_libraries(Step02_VertexColor PRIVATE vgt_build_deps)

vgt_add_glsl_shaders(Step02_VertexColor
  SOURCES
    shaders/vertex_color.vert
    shaders/vertex_color.frag
)
