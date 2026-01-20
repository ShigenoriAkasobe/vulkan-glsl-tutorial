add_executable(Step04_Transform
  main.cpp
)

target_link_libraries(Step04_Transform PRIVATE vgt_build_deps)

vgt_add_glsl_shaders(Step04_Transform
  SOURCES
    shaders/transform.vert
    shaders/transform.frag
)
