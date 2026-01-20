add_executable(Step03_Texture
  main.cpp
)

target_link_libraries(Step03_Texture PRIVATE vgt_build_deps)

vgt_add_glsl_shaders(Step03_Texture
  SOURCES
    shaders/texture.vert
    shaders/texture.frag
)
