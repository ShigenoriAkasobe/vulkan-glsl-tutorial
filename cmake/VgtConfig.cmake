configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/VgtConfig.h.in"
  "${CMAKE_BINARY_DIR}/generated/VgtConfig.h"
  @ONLY
)

add_library(vgt_config INTERFACE)
target_include_directories(vgt_config INTERFACE "${CMAKE_BINARY_DIR}/generated")
add_library(vgt::config ALIAS vgt_config)
