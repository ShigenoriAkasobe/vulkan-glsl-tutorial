add_executable(Step01_MinimalTriangle
  main.cpp
)

target_link_libraries(Step01_MinimalTriangle PRIVATE vgt_build_deps)

vgt_add_glsl_shaders(Step01_MinimalTriangle
  SOURCES
    shaders/triangle.vert
    shaders/triangle.frag
)
