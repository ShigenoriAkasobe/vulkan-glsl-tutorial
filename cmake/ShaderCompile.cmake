include_guard(GLOBAL)

function(vgt_add_glsl_shaders target)
  if (NOT GLSLANG_VALIDATOR)
    message(STATUS "glslangValidator not found: skipping shader compilation for ${target}")
    return()
  endif()

  set(options)
  set(oneValueArgs OUTPUT_DIR)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(VGT_SHADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT VGT_SHADERS_OUTPUT_DIR)
    set(VGT_SHADERS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  endif()

  file(MAKE_DIRECTORY "${VGT_SHADERS_OUTPUT_DIR}")

  set(outputs "")
  foreach(src IN LISTS VGT_SHADERS_SOURCES)
    if (NOT IS_ABSOLUTE "${src}")
      set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
    endif()

    get_filename_component(src_name "${src}" NAME)
    set(out "${VGT_SHADERS_OUTPUT_DIR}/${src_name}.spv")

    add_custom_command(
      OUTPUT "${out}"
      COMMAND "${GLSLANG_VALIDATOR}" -V "${src}" -o "${out}"
      DEPENDS "${src}"
      COMMENT "glslangValidator: ${src_name} -> ${src_name}.spv"
      VERBATIM
    )

    list(APPEND outputs "${out}")
  endforeach()

  if (outputs)
    add_custom_target(${target}_shaders DEPENDS ${outputs})
    add_dependencies(${target} ${target}_shaders)

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/shaders"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${VGT_SHADERS_OUTPUT_DIR}" "$<TARGET_FILE_DIR:${target}>/shaders"
      COMMENT "Copy compiled shaders next to executable"
    )
  endif()
endfunction()
