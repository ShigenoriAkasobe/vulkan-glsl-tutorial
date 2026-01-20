include(VgtCommon)
include(VgtVulkanConfig)

function(vgt_add_step_executable)
  set(options)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(VGT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT VGT_NAME)
    message(FATAL_ERROR "vgt_add_step_executable requires NAME")
  endif()

  add_executable(${VGT_NAME} ${VGT_SOURCES})

  vgt_set_default_warnings(${VGT_NAME})
  target_compile_features(${VGT_NAME} PRIVATE cxx_std_20)

  vgt_target_setup_vulkan(${VGT_NAME})
  target_link_libraries(${VGT_NAME} PRIVATE vgt::glfw)

  if(WIN32)
    target_compile_definitions(${VGT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  endif()

  vgt_copy_runtime_dlls(${VGT_NAME})
endfunction()
