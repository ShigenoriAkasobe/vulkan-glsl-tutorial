cmake_minimum_required(VERSION 3.26)

include(VgtSample)

vgt_add_step_executable(
  NAME Step00_ClearScreen
  SOURCES
    main.cpp
)

target_link_libraries(Step00_ClearScreen PRIVATE vgt::config)
