function(vgt_set_default_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

function(vgt_copy_runtime_dlls target)
  if(WIN32)
    # Disabled: some VS-bundled CMake builds fail when `cmake -E copy_if_different`
    # is invoked with directory arguments in POST_BUILD rules.
  endif()
endfunction()
