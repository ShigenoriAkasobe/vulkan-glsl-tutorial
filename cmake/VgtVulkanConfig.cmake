function(vgt_target_setup_vulkan target)
  target_include_directories(${target} PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE Vulkan::Vulkan)

  # Use volk-like manual loading via vkGetInstanceProcAddr, but without extra dependency.
  # We still link Vulkan::Vulkan for loader import library on Windows.
endfunction()
