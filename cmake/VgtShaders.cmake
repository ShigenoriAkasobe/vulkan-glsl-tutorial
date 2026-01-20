function(vgt_add_glsl_shaders target)
  set(options)
  set(oneValueArgs OUTPUT_DIR)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(VGT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT VGT_SOURCES)
    return()
  endif()

  find_program(GLSLANG_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/Bin32")
  if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found. Install Vulkan SDK and ensure it is in PATH (or VULKAN_SDK is set).")
  endif()

  if(NOT VGT_OUTPUT_DIR)
    set(VGT_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  endif()

  file(MAKE_DIRECTORY "${VGT_OUTPUT_DIR}")

  set(spv_outputs "")
  foreach(src IN LISTS VGT_SOURCES)
    get_filename_component(src_name "${src}" NAME)
    set(out_spv "${VGT_OUTPUT_DIR}/${src_name}.spv")

    add_custom_command(
      OUTPUT "${out_spv}"
      COMMAND "${GLSLANG_VALIDATOR}" -V "${src}" -o "${out_spv}"
      DEPENDS "${src}"
      VERBATIM
    )

    list(APPEND spv_outputs "${out_spv}")
  endforeach()

  add_custom_target(${target}_shaders DEPENDS ${spv_outputs})
  add_dependencies(${target} ${target}_shaders)
endfunction()
