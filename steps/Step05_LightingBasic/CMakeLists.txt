add_executable(Step05_LightingBasic
  main.cpp
)

target_link_libraries(Step05_LightingBasic PRIVATE vgt_build_deps)

vgt_add_glsl_shaders(Step05_LightingBasic
  SOURCES
    shaders/lighting.vert
    shaders/lighting.frag
)
