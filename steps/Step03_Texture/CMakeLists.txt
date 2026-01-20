cmake_minimum_required(VERSION 3.26)

include(VgtSample)
include(VgtShaders)

vgt_add_step_executable(
  NAME Step03_Texture
  SOURCES
    main.cpp
)

target_link_libraries(Step03_Texture PRIVATE vgt::config vgt::stb_image)

vgt_add_glsl_shaders(Step03_Texture
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/shaders/texture.vert"
    "${CMAKE_CURRENT_LIST_DIR}/shaders/texture.frag"
)

# Copy assets folder to build directory
add_custom_command(TARGET Step03_Texture POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_LIST_DIR}/assets"
    "$<TARGET_FILE_DIR:Step03_Texture>/assets"
  COMMENT "Copying assets to build directory"
)


